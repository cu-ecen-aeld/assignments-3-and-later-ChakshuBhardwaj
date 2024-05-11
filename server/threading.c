#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "threading.h"



struct thread_data* thread_data_init(struct thread_data *param, int sockfd, FILE *datafd, pthread_mutex_t  *mutex){
    
    // start by dynamically allocating memory for thread_data struct
    DEBUG_MSG("thread_data_init\n");
    param = !param ? (struct thread_data *) malloc(sizeof(struct thread_data)) : param;
    // Check if creating a new thread would exceed available memory
    // return NULL if the initialization failed 
    if(param == NULL)
    {
        return param;
    }
    // initializing the thread
    pthread_t *ptid = (pthread_t *) malloc(sizeof(pthread_t));
    if (ptid == NULL)
    {
        LOGGING("thread initialize fail");
        thread_data_deinit(param, ptid,sockfd);
        return NULL;
    }
    

    // assigning the struct initial variables 
    param->ptid =  ptid;
    param->mutex = mutex;
    param->sockfd = sockfd;
    param->datafd = datafd;
    param->thread_complete_status = false;

    return param;

}
void thread_data_deinit(struct thread_data *param, pthread_t *ptid, int sockfd)
{
    // free and deallocate the memory 
    free(ptid);
    tcp_close(sockfd);
    free(param);
}

