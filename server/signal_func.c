

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include "aesdsocket.h"

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

    // Initialize new_action struct
    memset(&new_action, 0, sizeof(struct sigaction));
    
    new_action.sa_handler = signal_handler;
    va_start(ap, number);
    while(number > 0)
    {
        signal = va_arg(ap, int);
        rc = sigaction(signal, &new_action, NULL);
        if(rc != 0) {
            break;
        }
        number--;
    }

    va_end(ap);

    return rc;
    
}

void timer_setup(int period, struct itimerval *timer, struct sigaction *sa) {
    // alarm signal variables 

    // Configure signal handler for timer expiration
    memset(sa, 0, sizeof(*sa));
    sa->sa_handler = timer_handler;
    sigaction(SIGALRM, sa, NULL);

    // Setup the signal handler
    sa->sa_handler = timer_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    sigaction(SIGALRM, sa, NULL);

    // Configure the timer to trigger every 10 seconds
    timer->it_value.tv_sec = 10;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = 10;
    timer->it_interval.tv_usec = 0;
}

int start_timer(struct itimerval *timer) {
    // Start the timer
    if (setitimer(ITIMER_REAL, timer, NULL) == -1) {
        perror("Error calling setitimer");
        return EXIT_FAILURE;
    }
    return 1;
}

void stop_timer(struct itimerval *timer) {
    // terminate timer 
    // Iterate over the list and print elements
    //struct thread_list *current_thread;
    // Disable the timer
    timer->it_value.tv_sec = 0;
    timer->it_value.tv_usec = 0;
    timer->it_interval.tv_sec = 0;
    timer->it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, timer, NULL);
}