#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    int Obtain_Wait = thread_func_args->time_obtain_ms;
    int Release_Wait = thread_func_args->time_release_ms;
    pthread_mutex_t Thread_Mutex = thread_func_args->MUTEX;

    sleep((Obtain_Wait / 1000));

    pthread_mutex_lock(&Thread_Mutex);

    sleep((Release_Wait / 1000));

    pthread_mutex_unlock(&Thread_Mutex);

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *td = malloc(sizeof(struct thread_data));
    if (td == NULL)
    {
        return false;
    }

    else
    {
        td->thread_complete_success = false;
        td->time_obtain_ms = wait_to_obtain_ms;
        td->time_release_ms = wait_to_release_ms;
        td->MUTEX = *(mutex);
        //pthread_mutex_init(mutex,td);
        pthread_create(thread, NULL, threadfunc, td);
        //threadfunc((struct thread_data *)td);
    }

    return false;
}

