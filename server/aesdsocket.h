/*
 * =====================================================================================
 *
 *       Filename:  aesdsocket.h
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
#ifndef AESD_SOCKET
#define AESD_SOCKET

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/queue.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sched.h> // For sched_param
#include <signal.h> // For signal handling
#include <time.h> // For POSIX interval timer

#include "threading.h"
#include "aesd_ioctl.h"

#define MYPORT          9000
#define LISTEN_BACKLOG  10
#define BUFFER_SIZE     (1024*30)

#define ERROR_HANDLER(func) { perror(#func); exit(EXIT_FAILURE); }
#define SOCKET_LOGGING(format, addr)         syslog(LOG_DEBUG, format, addr)
#define LOGGING(format)                      syslog(LOG_DEBUG, format)
#define DEBUG_MSG(msg,...) printf(msg "\n" , ##__VA_ARGS__)

#define SLIST_FOREACH_SAFE(var, head, field, tvar)                           \
    for ((var) = SLIST_FIRST((head));                                        \
            (var) && ((tvar) = SLIST_NEXT((var), field), 1);                 \
            (var) = (tvar))

extern int signal_sign;

extern struct thread_data *param;

// Define the structure for thread_list
struct thread_list {
    struct thread_data *thread_data;
    SLIST_ENTRY(thread_list) thread_entries; // Structure required by SLIST macros
};

int tcp_socket_setup(int domain, int type, int protocol, struct sockaddr_in addr, int reuse);
int tcp_incoming_check(int sockfd, struct sockaddr_in *client, socklen_t addrlen);
void tcp_shutdown(int sockfd, int how);
void tcp_close (int sockfd);
int tcp_receive(int acceptfd, char *buffer, int size);
int tcp_send(int acceptfd, char *buffer, int size);
void *tcp_echoback (void *arg);
void tcp_set_nonblock(int sockfd, int invert);
int tcp_select(int sockfd);
int tcp_getopt(int argc, char *argv[]);
void process_kill(int sockfd, struct itimerval *timer, pthread_mutex_t *mutex);
int add_thread_to_list(void * thread_param);
void check_for_completed_threads();
void timer_handler(int signum);


#endif 