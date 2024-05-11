/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.c
 *
 *    Description:  Assignment 6
 *
 *        Version:  2.0
 *        Created:  March 17, 2024.
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Youssef Essam 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "aesdsocket.h"
#include "manipulate_file.h" 

#define USE_AESD_CHAR_DEVICE    1

#if (USE_AESD_CHAR_DEVICE == 1)
#define FILENAME "/dev/aesdchar"
#else   
#define FILENAME "/var/tmp/aesdsocketdata"
#endif

/* Global variable */
int signal_sign = 0;
// define mutex pointer 
pthread_mutex_t *mutex = NULL;
// Iterate over the list and print elements
struct thread_list *current_thread;
// define timer ID
//timer_t timerid;
// define file descriptor variable 
FILE *datafd;
/* function prototype */
int signal_setup(int, ...);

// Define the list head structure
SLIST_HEAD(listhead, thread_list) head = SLIST_HEAD_INITIALIZER(head);

int main(int argc, char *argv[]) {
    pid_t id = 0;
    socklen_t addrlen;
    int sockfd, acceptfd, rc;
    struct sockaddr_in serv, client;
    struct itimerval timer;

    SLIST_INIT(&head);

    #if (USE_AESD_CHAR_DEVICE == 0)
        struct sigaction sa;
        int timer_start = 0;
        timer_setup(10, &timer, &sa);
    #endif

    DEBUG_MSG("Welcom to socket Testing program");
    
    // initialize the serv struct of my socket
    memset(&serv, 0 ,sizeof(struct sockaddr));
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(9000);
    serv.sin_family = AF_INET;
    //tcpfd = (struct fdset *) malloc(sizeof(struct fdset));
    
    // initializing the mutex
    mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL) {
        ERROR_HANDLER(pthread_mutex_init);
    }
    
    //DEBUG_MSG("mutex init\n");
    if (pthread_mutex_init(mutex, NULL) != 0)
    {
        ERROR_HANDLER(pthread_mutex_init);
    }
    // Set syslog configuration up.
    openlog(NULL, LOG_PID, LOG_USER);

    sockfd = tcp_socket_setup(AF_INET, SOCK_STREAM, IPPROTO_TCP, serv, true);
    if (sockfd == -1) {
        process_kill(sockfd, &timer, mutex);
        ERROR_HANDLER(socket);
    }
    tcp_set_nonblock(sockfd, 0);

    rc = tcp_getopt(argc, argv);
    if (rc == false) {
        DEBUG_MSG("No any options.\n");
    }

    if (id != 0) {
        perror("daemon ");
        exit(EXIT_SUCCESS);
    }

    rc = signal_setup(2, SIGINT, SIGTERM);
    
    while (1) {
        addrlen = sizeof(struct sockaddr);
        acceptfd = tcp_incoming_check(sockfd, &client, addrlen);
        if (acceptfd > 0) {
            // Create/open a file to write the data received from a client.
            if(datafd == NULL) {
                datafd = fopen(FILENAME, "a+");
                if(datafd == NULL) {
                    perror("Error Can't open /dev/aesdchar\n");
                    return EXIT_FAILURE;
                }
            }
            /* start timer stampe in file /var/tmp/aesdsocketdata*/
            #if (USE_AESD_CHAR_DEVICE == 0)
                if(! timer_start) {
                    start_timer(&timer);
                    timer_start = 1;
                } 
            #endif
            //printf("Accepted connection from %s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            SOCKET_LOGGING("Accepted connection from %d", acceptfd);
            // define variable to receive thread data parameters in 
            struct thread_data *param = NULL;

            // initialize the thread data 
            SOCKET_LOGGING("initialize new thread data for %d", acceptfd);
            if ((param = thread_data_init(param, acceptfd, datafd, mutex)) == NULL)
            {
                LOGGING("thread data initialize fail");
                //thread_data_deinit(param, threadid, acceptfd);
            } else {
                // add new thread to thread linked list 
                LOGGING("add new thread to thread linked list for");
                if(add_thread_to_list(param) == 0) {
                    LOGGING("Can't add new thread data pinter to the linked list!");
                    //thread_data_deinit(param, threadid, acceptfd);
                } else {
                    // launch the new thread 
                    if (pthread_create(param->ptid, NULL, tcp_echoback, (void *) param) != 0)
                    {
                        LOGGING("thread create fail");
                        // set the complete variable true to clear the thread data parameter and deallocate the memory 
                        thread_data_deinit(param, param->ptid, param->sockfd);
                    }
    
                }

            }
        }
        // check for signal
        if(signal_sign)
        {
            // if the signal set to one so the process received SIGINT or SIGTERM
            process_kill(sockfd, &timer, mutex);
            break;
        }

        if(acceptfd == 0)
        {
            LOGGING("Waiting for new connection request or SIGINT or SIGTERM to kill the process.\n");
            if(datafd) {
                // close the file in the ideal time when no data recived
                fclose(datafd);
            }
            
            // loop over threads to check for 
            check_for_completed_threads();
        }        
    }
    
    return 0;
}

int tcp_socket_setup(int domain, int type, int protocol, struct sockaddr_in addr, int reuse) {

    int sockfd, rc, on = 1;

    DEBUG_MSG("Initializing the socket ....");

    sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        ERROR_HANDLER(socket);
    }

    if (reuse == true) {
        // Set socket option to enable reuse the local address
        rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &on, (socklen_t) sizeof(on));
        if (rc == -1) {
            ERROR_HANDLER(setsockopt);
        }
    }

    rc = bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
    if (rc == -1) {
        ERROR_HANDLER(bind);
    }

    return sockfd;
}

int tcp_incoming_check(int sockfd, struct sockaddr_in *client, socklen_t addrlen) {

    int rc, acceptfd;
    DEBUG_MSG("LISTENING...");
    rc = listen(sockfd, LISTEN_BACKLOG);
    if (rc == -1) {
        ERROR_HANDLER(listen);
    }
  
    rc = tcp_select(sockfd);
    if (rc == -1) {
        DEBUG_MSG("tcp_select no pick up.");
        return -1;
    } else if (rc == 0) {
        printf("select timeout.\n");
        return 0;
    }

    acceptfd = accept(sockfd, (struct sockaddr *) client, &addrlen);
    if (acceptfd == -1) {
        ERROR_HANDLER(accept);
        //DEBUG_MSG("Failed to accept connection");
    }
    return acceptfd;
}

void tcp_shutdown(int sockfd, int how) {

    if (shutdown(sockfd, how)) {
        ERROR_HANDLER(shutdown);
    }
    return;
}

void tcp_close (int sockfd) {

    if (sockfd > 0) {
        tcp_shutdown(sockfd, SHUT_RDWR);
    }
    if (close(sockfd)) {
        ERROR_HANDLER(close);
    }
    return;
}

int tcp_receive(int acceptfd, char *buffer, int size) {
    int rc;
    
    rc = tcp_select(acceptfd);
    if(rc > 0) {
        rc = recv(acceptfd, buffer, size, 0); 
        if (rc == 0) {
            SOCKET_LOGGING("The client-side might was closed, rc: %d\n", rc);
        }
    }
    
    return rc;
}

int tcp_send(int acceptfd, char *buffer, int size) {
    int rc;
   
    rc = send(acceptfd, buffer, size, 0);
    /*if (rc != size) {
        ERROR_HANDLER(send);
    }*/
    return rc;
}

void *tcp_echoback (void *thread_param) {
    struct thread_data* param = (struct thread_data *) thread_param;
    int rc = 0;
    char sbuffer[BUFFER_SIZE] = {0}, rbuffer[BUFFER_SIZE] = {0};
    param->thread_complete_status = false;
    struct aesd_seekto pair;

    do {
        // obtain the mutex
        pthread_mutex_lock(param->mutex);

        rc = tcp_receive(param->sockfd, rbuffer, sizeof(rbuffer));
        if (rc > 0) {
            // wait for a packet which ends with '\n' character
            if (strchr(rbuffer, '\n')) {

                // check if the recived is command 
                if(strstr(rbuffer, "AESDCHAR_IOCSEEKTO") == rbuffer)
                {
                    /* the size of file calculation here will affect the correct 
                    * start offset as we will not read from file beginning so calculate it earlier */
                    int size_read =  file_size(fileno(param->datafd));
                    
                    sscanf(rbuffer,"AESDCHAR_IOCSEEKTO:%u,%u", &pair.write_cmd, &pair.write_cmd_offset);
                    printf("Found command %u, %u\n",pair.write_cmd, pair.write_cmd_offset);
                    ioctl(fileno(param->datafd), AESDCHAR_IOCSEEKTO, &pair);
                    
                    // read the written data to the file /var/tmp/aesdsocketdata or /dev/aesdchar
                    if(file_read(fileno(param->datafd), sbuffer, size_read) < 0) {
                        SOCKET_LOGGING("[%d] failed to read from file", param->sockfd);
                        break;
                    }
                     
                } else {

                    if(file_write(fileno(param->datafd), rbuffer, rc) <= 0){
                        //perror("failed to write to /var/tmp/aesdsocketdata file\n");
                        SOCKET_LOGGING("[%d] failed to write to file", param->sockfd);
                        break;
                    }
                    fflush(param->datafd);

                    /* the size of file calculation here will not change the correct 
                    * start offset as we need to read from file beginning */
                    int size_read =  file_size(fileno(param->datafd));
                    // read the written data to the file /var/tmp/aesdsocketdata or /dev/aesdchar
                    if(file_read(fileno(param->datafd), sbuffer, size_read) < 0) {
                        SOCKET_LOGGING("[%d] failed to read from file", param->sockfd);
                        break;
                    }
        
                }
                
                // send the received data back to the file /var/tmp/aesdsocketdata 
                if(tcp_send(param->sockfd, sbuffer, strlen(sbuffer)) <= 0) {
                    SOCKET_LOGGING("[%d] failed to send data back", param->sockfd);
                    break;
                }
                // clear the buffers 
                memset(rbuffer, 0, sizeof(rbuffer));
                memset(sbuffer, 0, sizeof(sbuffer));

                pthread_mutex_unlock(param->mutex);

            }
        } else if (rc == 0) {
            pthread_mutex_unlock(param->mutex);
            break; // connection closed 
        } else {
            pthread_mutex_unlock(param->mutex);
            // Error receiving data
            perror("Error receiving data");
            break;
        }

    } while (1);
    // in case the reciveing and sending operations ended witout problems 
    param->thread_complete_status = true;

    return NULL;
}

void tcp_set_nonblock(int sockfd, int invert) {

    int flag;

    flag = fcntl(sockfd, F_GETFL);
    if (!invert) {
        fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
    } else {
        fcntl(sockfd, F_SETFL, flag ^ O_NONBLOCK);
    }

    return;
}

int tcp_select(int sockfd) {
    int rc;
    fd_set readfds;
    struct timeval time = {0, 0};
    time.tv_sec = 15;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    do {
        rc = select(sockfd + 1, &readfds, NULL, NULL, &time);
    } while (rc == -1 && errno == EINTR); // Retry if interrupted by a signal

    if (rc > 0 && FD_ISSET(sockfd, &readfds)) {
        return sockfd;
    } else if (rc == 0) {
        return 0; // Timeout
    } else {
        // Handle other errors
        perror("select");
        return -1;
    }
}

int tcp_getopt(int argc, char *argv[]) {

    int ch;

    while ((ch = getopt(argc, argv, "d")) != -1) {
        switch(ch) {
            case 'd':
                printf("Convert %s into daemon.\n", __FILE__);
                daemon(1, 0);
                LOGGING("Running as daemon");
                return true;
        }
    }
    return false;
} 

void process_kill(int sockfd, struct itimerval *timer, pthread_mutex_t *mutex) {
    
    #if (USE_AESD_CHAR_DEVICE == 0)
        stop_timer(timer);
    #endif

    //check_for_completed_threads();

    // Remove elements from the list after close and free all file descriptors
    //printf("loop over the list to terminate\n");
    struct thread_list *current_thread, *tmp;

    SLIST_FOREACH_SAFE(current_thread, &head, thread_entries, tmp) {
        if (current_thread->thread_data->thread_complete_status) {
            if (pthread_join(*(current_thread->thread_data->ptid), NULL) != 0) {
                perror("pthread_join");
                // Handle error, e.g., continue, break, or return
            }

            // Clean up resources associated with the thread
            tcp_close(current_thread->thread_data->sockfd);
            free(current_thread->thread_data->ptid);
            free(current_thread->thread_data);
            
            // Remove the thread entry from the list and free memory
            SLIST_REMOVE(&head, current_thread, thread_list, thread_entries);
            free(current_thread);
        }
    }

    free(current_thread);
    if(datafd) {
        fclose(datafd);
    }
    // close the server socket
    tcp_close(sockfd);
    // destroy mutex
    pthread_mutex_destroy(mutex);
    free(mutex);
    #if (USE_AESD_CHAR_DEVICE == 0)
        file_delete(FILENAME);
    #endif
    closelog();
}

int add_thread_to_list(void * thread_param)
{

    // addd the thread struct pointer to the thread list
    //printf("addd thread struct pointer to thread list\n");  
    struct thread_list *new_thread = malloc(sizeof(struct thread_list));
    if (new_thread == NULL) {
    perror("New thread memory allocation failed");
    return 0;
    //exit(EXIT_FAILURE);
    }
        
    // assign the new thread_data to the linked list 
    new_thread->thread_data = (struct thread_data*) thread_param;
    SLIST_INSERT_HEAD(&head, new_thread, thread_entries);
    return 1;
}

void check_for_completed_threads()
{
    //printf("check for completed threads \n");
    
    SLIST_FOREACH(current_thread, &head, thread_entries) {
        // if the current thread completed 
        if((current_thread->thread_data->thread_complete_status) == true)
        {
            // terminating the current thread
            if (pthread_join(*(current_thread->thread_data->ptid), NULL) == -1) {
                //process_kill(current_thread->thread_data->sockfd, param);
                ERROR_HANDLER(pthread_join);
            }
            SOCKET_LOGGING("Closed Connection from %d", (current_thread->thread_data->sockfd));
            current_thread->thread_data->thread_complete_status = false;
        }
    }
}

void timer_handler(int signum) {
    time_t now;
    struct tm *timeinfo;
    //char stamp[36] = {0};
    char timestamp[40] = {0};
    //FILE *file;

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S\n", timeinfo);
    //strcat(timestamp, stamp);
    //printf("set timestamp!\n");

    pthread_mutex_lock(mutex);
    // write the timestamp to the file /var/tmp/aesdsocketdata 
    if(file_write(fileno(datafd), timestamp, strlen(timestamp)) <= 0){
        perror("failed to write timestamp to file\n");
    }

    pthread_mutex_unlock(mutex);
}