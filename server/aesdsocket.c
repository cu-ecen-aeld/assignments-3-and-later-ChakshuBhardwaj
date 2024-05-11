
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "aesdsocket.h"

#define USE_AESD_CHAR_DEVICE 1

#if (USE_AESD_CHAR_DEVICE == 1)
#define FILENAME "/dev/aesdchar"
#else
#define FILENAME "/var/tmp/aesdsocketdata"
#endif

int file_create(char *pathname, ...);

int file_write(int fd, const char *buffer, int size);

int file_size(int fd);

int file_read(int fd, char *buffer, int size);

void file_delete(char *file);

struct thread_data
{
    pthread_t *ptid;
    pthread_mutex_t *mutex;
    int sockfd;
    FILE *datafd;
    bool thread_complete_status;
};

struct thread_data *thread_data_init(struct thread_data *param, int sockfd, FILE *datafd, pthread_mutex_t *mutex);
void thread_data_deinit(struct thread_data *param, pthread_t *ptid, int sockfd);

int signal_sign = 0;
pthread_mutex_t *mutex = NULL;
struct thread_list *current_thread;
FILE *datafd;
int signal_setup(int, ...);
SLIST_HEAD(listhead, thread_list)
head = SLIST_HEAD_INITIALIZER(head);

int main(int argc, char *argv[])
{
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
    memset(&serv, 0, sizeof(struct sockaddr));
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(9000);
    serv.sin_family = AF_INET;
    mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (mutex == NULL)
    {
        ERROR_HANDLER(pthread_mutex_init);
    }

    if (pthread_mutex_init(mutex, NULL) != 0)
    {
        ERROR_HANDLER(pthread_mutex_init);
    }

    openlog(NULL, LOG_PID, LOG_USER);

    sockfd = tcp_socket_setup(AF_INET, SOCK_STREAM, IPPROTO_TCP, serv, true);
    if (sockfd == -1)
    {
        process_kill(sockfd, &timer, mutex);
        ERROR_HANDLER(socket);
    }
    tcp_set_nonblock(sockfd, 0);

    rc = tcp_getopt(argc, argv);
    if (rc == false)
    {
        DEBUG_MSG("No any options.\n");
    }

    if (id != 0)
    {
        perror("daemon ");
        exit(EXIT_SUCCESS);
    }

    rc = signal_setup(2, SIGINT, SIGTERM);

    while (1)
    {
        addrlen = sizeof(struct sockaddr);
        acceptfd = tcp_incoming_check(sockfd, &client, addrlen);
        if (acceptfd > 0)
        {

            if (datafd == NULL)
            {
                datafd = fopen(FILENAME, "a+");
                if (datafd == NULL)
                {
                    perror("Error Can't open /dev/aesdchar\n");
                    return EXIT_FAILURE;
                }
            }
#if (USE_AESD_CHAR_DEVICE == 0)
            if (!timer_start)
            {
                start_timer(&timer);
                timer_start = 1;
            }
#endif

            SOCKET_LOGGING("Accepted connection from %d", acceptfd);
            struct thread_data *param = NULL;

            SOCKET_LOGGING("initialize new thread data for %d", acceptfd);
            // if ((param = thread_data_init(param, acceptfd, datafd, mutex)) == NULL)
            if (0)
            {
                LOGGING("thread data initialize fail");
            }
            else
            {
                LOGGING("add new thread to thread linked list for");
                if (add_thread_to_list(param) == 0)
                {
                    LOGGING("Can't add new thread data pinter to the linked list!");
                }
                else
                {
                    if (pthread_create(param->ptid, NULL, tcp_echoback, (void *)param) != 0)
                    {
                        LOGGING("thread create fail");
                    }
                }
            }
        }
        if (signal_sign)
        {
            process_kill(sockfd, &timer, mutex);
            break;
        }

        if (acceptfd == 0)
        {
            LOGGING("Waiting for new connection request or SIGINT or SIGTERM to kill the process.\n");
            if (datafd)
            {
                fclose(datafd);
            }

            check_for_completed_threads();
        }
    }

    return 0;
}

int tcp_socket_setup(int domain, int type, int protocol, struct sockaddr_in addr, int reuse)
{

    int sockfd, rc, on = 1;

    DEBUG_MSG("Initializing the socket ....");

    sockfd = socket(domain, type, protocol);
    if (sockfd == -1)
    {
        ERROR_HANDLER(socket);
    }

    if (reuse == true)
    {
        rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&on, (socklen_t)sizeof(on));
        if (rc == -1)
        {
            ERROR_HANDLER(setsockopt);
        }
    }

    rc = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (rc == -1)
    {
        ERROR_HANDLER(bind);
    }

    return sockfd;
}

int tcp_incoming_check(int sockfd, struct sockaddr_in *client, socklen_t addrlen)
{

    int rc, acceptfd;
    DEBUG_MSG("LISTENING...");
    rc = listen(sockfd, LISTEN_BACKLOG);
    if (rc == -1)
    {
        ERROR_HANDLER(listen);
    }

    rc = tcp_select(sockfd);
    if (rc == -1)
    {
        DEBUG_MSG("tcp_select no pick up.");
        return -1;
    }
    else if (rc == 0)
    {
        printf("select timeout.\n");
        return 0;
    }

    acceptfd = accept(sockfd, (struct sockaddr *)client, &addrlen);
    if (acceptfd == -1)
    {
        ERROR_HANDLER(accept);
    }
    return acceptfd;
}

void tcp_shutdown(int sockfd, int how)
{

    if (shutdown(sockfd, how))
    {
        ERROR_HANDLER(shutdown);
    }
    return;
}

void tcp_close(int sockfd)
{

    if (sockfd > 0)
    {
        tcp_shutdown(sockfd, SHUT_RDWR);
    }
    if (close(sockfd))
    {
        ERROR_HANDLER(close);
    }
    return;
}

int tcp_receive(int acceptfd, char *buffer, int size)
{
    int rc;

    rc = tcp_select(acceptfd);
    if (rc > 0)
    {
        rc = recv(acceptfd, buffer, size, 0);
        if (rc == 0)
        {
            SOCKET_LOGGING("The client-side might was closed, rc: %d\n", rc);
        }
    }

    return rc;
}

int tcp_send(int acceptfd, char *buffer, int size)
{
    int rc;

    rc = send(acceptfd, buffer, size, 0);
    return rc;
}

void *tcp_echoback(void *thread_param)
{
    struct thread_data *param = (struct thread_data *)thread_param;
    int rc = 0;
    char sbuffer[BUFFER_SIZE] = {0}, rbuffer[BUFFER_SIZE] = {0};
    param->thread_complete_status = false;
    struct aesd_seekto pair;

    do
    {
        pthread_mutex_lock(param->mutex);

        rc = tcp_receive(param->sockfd, rbuffer, sizeof(rbuffer));
        if (rc > 0)
        {
            if (strchr(rbuffer, '\n'))
            {

                if (strstr(rbuffer, "AESDCHAR_IOCSEEKTO") == rbuffer)
                {
                    int size_read = file_size(fileno(param->datafd));

                    sscanf(rbuffer, "AESDCHAR_IOCSEEKTO:%u,%u", &pair.write_cmd, &pair.write_cmd_offset);
                    printf("Found command %u, %u\n", pair.write_cmd, pair.write_cmd_offset);
                    ioctl(fileno(param->datafd), AESDCHAR_IOCSEEKTO, &pair);
                    if (file_read(fileno(param->datafd), sbuffer, size_read) < 0)
                    {
                        SOCKET_LOGGING("[%d] failed to read from file", param->sockfd);
                        break;
                    }
                }
                else
                {

                    if (file_write(fileno(param->datafd), rbuffer, rc) <= 0)
                    {
                        SOCKET_LOGGING("[%d] failed to write to file", param->sockfd);
                        break;
                    }
                    fflush(param->datafd);
                    int size_read = file_size(fileno(param->datafd));
                    if (file_read(fileno(param->datafd), sbuffer, size_read) < 0)
                    {
                        SOCKET_LOGGING("[%d] failed to read from file", param->sockfd);
                        break;
                    }
                }
                if (tcp_send(param->sockfd, sbuffer, strlen(sbuffer)) <= 0)
                {
                    SOCKET_LOGGING("[%d] failed to send data back", param->sockfd);
                    break;
                }
                memset(rbuffer, 0, sizeof(rbuffer));
                memset(sbuffer, 0, sizeof(sbuffer));

                pthread_mutex_unlock(param->mutex);
            }
        }
        else if (rc == 0)
        {
            pthread_mutex_unlock(param->mutex);
            break;
        }
        else
        {
            pthread_mutex_unlock(param->mutex);
            perror("Error receiving data");
            break;
        }

    } while (1);
    param->thread_complete_status = true;

    return NULL;
}

void tcp_set_nonblock(int sockfd, int invert)
{

    int flag;

    flag = fcntl(sockfd, F_GETFL);
    if (!invert)
    {
        fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
    }
    else
    {
        fcntl(sockfd, F_SETFL, flag ^ O_NONBLOCK);
    }

    return;
}

int tcp_select(int sockfd)
{
    int rc;
    fd_set readfds;
    struct timeval time = {0, 0};
    time.tv_sec = 15;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    do
    {
        rc = select(sockfd + 1, &readfds, NULL, NULL, &time);
    } while (rc == -1 && errno == EINTR);

    if (rc > 0 && FD_ISSET(sockfd, &readfds))
    {
        return sockfd;
    }
    else if (rc == 0)
    {
        return 0;
    }
    else
    {
        perror("select");
        return -1;
    }
}

int tcp_getopt(int argc, char *argv[])
{

    int ch, discard;

    while ((ch = getopt(argc, argv, "d")) != -1)
    {
        switch (ch)
        {
        case 'd':
            printf("Convert %s into daemon.\n", __FILE__);
            discard = daemon(1, 0);
            LOGGING("Running as daemon");
            return true;
        }
    }
    return false;
}

void process_kill(int sockfd, struct itimerval *timer, pthread_mutex_t *mutex)
{

#if (USE_AESD_CHAR_DEVICE == 0)
    stop_timer(timer);
#endif
    struct thread_list *current_thread, *tmp;

    SLIST_FOREACH_SAFE(current_thread, &head, thread_entries, tmp)
    {
        if (current_thread->thread_data->thread_complete_status)
        {
            if (pthread_join(*(current_thread->thread_data->ptid), NULL) != 0)
            {
                perror("pthread_join");
            }

            tcp_close(current_thread->thread_data->sockfd);
            free(current_thread->thread_data->ptid);
            free(current_thread->thread_data);

            SLIST_REMOVE(&head, current_thread, thread_list, thread_entries);
            free(current_thread);
        }
    }

    free(current_thread);
    if (datafd)
    {
        fclose(datafd);
    }
    tcp_close(sockfd);
    pthread_mutex_destroy(mutex);
    free(mutex);
#if (USE_AESD_CHAR_DEVICE == 0)
    file_delete(FILENAME);
#endif
    closelog();
}

int add_thread_to_list(void *thread_param)
{

    struct thread_list *new_thread = malloc(sizeof(struct thread_list));
    if (new_thread == NULL)
    {
        perror("New thread memory allocation failed");
        return 0;
    }
    new_thread->thread_data = (struct thread_data *)thread_param;
    SLIST_INSERT_HEAD(&head, new_thread, thread_entries);
    return 1;
}

void check_for_completed_threads()
{

    SLIST_FOREACH(current_thread, &head, thread_entries)
    {
        if ((current_thread->thread_data->thread_complete_status) == true)
        {
            if (pthread_join(*(current_thread->thread_data->ptid), NULL) == -1)
            {
                ERROR_HANDLER(pthread_join);
            }
            SOCKET_LOGGING("Closed Connection from %d", (current_thread->thread_data->sockfd));
            current_thread->thread_data->thread_complete_status = false;
        }
    }
}

void timer_handler(int signum)
{
    time_t now;
    struct tm *timeinfo;
    char timestamp[40] = {0};

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S\n", timeinfo);

    pthread_mutex_lock(mutex);
    if (file_write(fileno(datafd), timestamp, strlen(timestamp)) <= 0)
    {
        perror("failed to write timestamp to file\n");
    }

    pthread_mutex_unlock(mutex);
}

int file_write(int fd, const char *buffer, int size)
{
    if (fd < 0)
    {
        fprintf(stderr, "Invalid file descriptor\n");
        return -1;
    }

    ssize_t written = write(fd, buffer, size);
    if (written == -1)
    {
        perror("Failed to write to file");
        return -1;
    }

    return written;
}

int file_size(int fd)
{
    int front, end;

    end = lseek(fd, 0, SEEK_END);
    front = lseek(fd, 0, SEEK_SET);

    return end - front;
}

int file_read(int fd, char *buffer, int size)
{
    char buf[1024] = {0};

    if (fd < 0)
    {
        fprintf(stderr, "Invalid file descriptor\n");
        return -1;
    }

    if (!buffer || size <= 0)
    {
        fprintf(stderr, "Invalid buffer or size\n");
        return -1;
    }
    ssize_t bytes_read = 0;
    do
    {
        bytes_read = read(fd, buf, size);
        if (bytes_read == -1)
        {
            perror("Failed to read from file");
            return -1;
        }
        else if (bytes_read > 0)
        {
            strcat(buffer, buf);
            size -= bytes_read;
            memset(buf, 0, sizeof(buf));
        }

    } while (bytes_read);

    return (int)bytes_read;
}

void file_delete(char *file)
{
    int rc = 0;
    struct stat status;
    memset(&status, 0, sizeof(struct stat));

    rc = stat(file, &status);
    if (rc != -1)
    {
        remove(file);
    }
}

static void signal_handler(int signal_number)
{
    LOGGING("Caught signal, exiting");

    signal_sign = 1;
}

int signal_setup(int number, ...)
{
    int signal, rc;
    va_list ap;
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));

    new_action.sa_handler = signal_handler;
    va_start(ap, number);
    while (number > 0)
    {
        signal = va_arg(ap, int);
        rc = sigaction(signal, &new_action, NULL);
        if (rc != 0)
        {
            break;
        }
        number--;
    }

    va_end(ap);

    return rc;
}

void timer_setup(int period, struct itimerval *timer, struct sigaction *sa)
{
    memset(sa, 0, sizeof(*sa));
    sa->sa_handler = timer_handler;
    sigaction(SIGALRM, sa, NULL);
    sa->sa_handler = timer_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    sigaction(SIGALRM, sa, NULL);
    timer->it_value.tv_sec = 10;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = 10;
    timer->it_interval.tv_usec = 0;
}

int start_timer(struct itimerval *timer)
{
    if (setitimer(ITIMER_REAL, timer, NULL) == -1)
    {
        perror("Error calling setitimer");
        return EXIT_FAILURE;
    }
    return 1;
}

void stop_timer(struct itimerval *timer)
{
    timer->it_value.tv_sec = 0;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = 0;
    timer->it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, timer, NULL);
}

struct thread_data *thread_data_init(struct thread_data *param, int sockfd, FILE *datafd, pthread_mutex_t *mutex)
{

    DEBUG_MSG("thread_data_init\n");
    param = !param ? (struct thread_data *)malloc(sizeof(struct thread_data)) : param;
    if (param == NULL)
    {
        return param;
    }

    pthread_t *ptid = (pthread_t *)malloc(sizeof(pthread_t));
    if (ptid == NULL)
    {
        LOGGING("thread initialize fail");
        thread_data_deinit(param, ptid, sockfd);
        return NULL;
    }
    param->ptid = ptid;
    param->mutex = mutex;
    param->sockfd = sockfd;
    param->datafd = datafd;
    param->thread_complete_status = false;

    return param;
}
void thread_data_deinit(struct thread_data *param, pthread_t *ptid, int sockfd)
{
    free(ptid);
    tcp_close(sockfd);
    free(param);
}