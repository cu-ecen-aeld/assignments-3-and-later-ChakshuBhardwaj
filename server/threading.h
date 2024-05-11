#ifndef THREADING
#define THREADING

#include <stdbool.h>
#include <pthread.h>
#include "aesdsocket.h"

//#define MUTEX_INIT
// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
//#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{
    /*
     * TODO: add other values your thread will need to manage
     * into this structure, use this structure to communicate
     * between the start_thread_obtaining_mutex function and
     * your thread implementation.
     */
    pthread_t        *ptid; // thread pointer
    pthread_mutex_t  *mutex; // mutex pointer
    int              sockfd; //socket file descriptor
    FILE             *datafd; // data file descriptor
    bool thread_complete_status; // set false if the thread didn't finish yet
};


struct thread_data* thread_data_init(struct thread_data *param, int sockfd, FILE *datafd, pthread_mutex_t  *mutex);
void thread_data_deinit(struct thread_data *param, pthread_t *ptid, int sockfd);

#endif