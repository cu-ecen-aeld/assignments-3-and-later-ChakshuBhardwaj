#define __USE_POSIX

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 20100
#define MAX_LINE_LEN 20100

void *SendAndReceive(void *arg);
void *TimerRoutine(void *argument);

pid_t process_id = 0, sid = 0;
pthread_t AllThreadID, MainThreadID, TimerThreadID;
pthread_mutex_t Lock;

struct addrinfo hints;
struct addrinfo *servinfo;
struct sockaddr acceptsockaddr;
struct sigaction new_action;
socklen_t acceptaddrlen;

FILE *file = NULL;
char *fileaddress = "/var/tmp/aesdsocketdata.txt";
char *filewritepermissions = "a+";
char *filereadpermissions = "r";
char *Port = "9000";
char ReadData[BUFFER_SIZE];
char WriteFileData[BUFFER_SIZE];
char line[MAX_LINE_LEN];

ssize_t recvdatabytes;
int socketfd, acceptfd, status;
uint64_t Index;
int32_t ListHead = 0;

bool success = true;
bool IS_EXIT = false;
bool New_Line = false;
bool Daemon_Mode = false;

bool caught_sigint = false;
bool caught_sigterm = false;
bool IsDone = true;

struct slist_data_s
{
    bool IsComplete;
    pthread_t Thread_ID;
    SLIST_ENTRY(slist_data_s)
    entries;
};
struct slist_data_s *listaddhead;
struct slist_data_s *listaddafter;
struct slist_data_s *listaddold;

struct tm *Info;
time_t t;
char s[100];
char timestring[200];

static void signal_handler(int signal_number)
{
    if (signal_number == SIGINT)
    {
        caught_sigint = true;
    }

    else if (signal_number == SIGTERM)
    {
        caught_sigterm = true;
    }
}

int main(int argc, char **argv)
{
    SLIST_HEAD(slisthead, slist_data_s)
    head;
    // head = SLIST_HEAD_INITIALIZER(head);
    //  struct slisthead *datap = NULL;

    SLIST_INIT(&head);
    pthread_mutex_init(&Lock, NULL);

    if (argc == 2 && (strcmp(argv[1], "-d") == 0))
    {
        Daemon_Mode = true;
        printf("Entered Daemon mode\n");
    }

    if (Daemon_Mode == false)
    {
        pthread_create(&TimerThreadID, NULL, TimerRoutine, NULL);
    }

    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    if (sigaction(SIGTERM, &new_action, NULL) != 0)
    {
        printf("Error registering for SIGTERM\n");
        success = false;
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &new_action, NULL) != 0)
    {
        printf("Error registering for SIGINT\n");
        success = false;
        exit(EXIT_FAILURE);
    }

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    openlog(NULL, 0, LOG_USER);
    if (socketfd == -1)
    {
        printf("Failded to open socket\n");
        close(socketfd);
        exit(EXIT_FAILURE);
    }

    else
    {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        status = getaddrinfo(NULL, Port, &hints, &servinfo);
        if (status != 0)
        {
            printf("Getaddrinfo error\n");
            close(socketfd);
            exit(EXIT_FAILURE);
        }
        status = bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);
        if (status == -1)
        {
            printf("Failed to bind\n");
            close(socketfd);
            exit(EXIT_FAILURE);
        }
        if (Daemon_Mode == true)
        {
            process_id = fork();
            if (process_id < 0)
            {
                close(socketfd);
                printf("Failed to fork\n");
                exit(EXIT_FAILURE);
            }

            if (process_id > 0)
            {
                printf("The PID of child process is = %d\n", process_id);
                exit(EXIT_SUCCESS);
            }

            sid = setsid();
            if (sid < 0)
            {
                close(socketfd);
                printf("SID failed\n");
                exit(EXIT_FAILURE);
            }
            pthread_create(&TimerThreadID, NULL, TimerRoutine, NULL);
        }
    }

    status = listen(socketfd, 100);
    if (status != 0)
    {
        printf("Unable to run the listen function\n");
        freeaddrinfo(servinfo);
        close(socketfd);
        exit(EXIT_FAILURE);
    }

    while (IS_EXIT == false)
    {
        acceptaddrlen = sizeof(acceptsockaddr);
        acceptfd = accept(socketfd, &acceptsockaddr, &acceptaddrlen);
        if (acceptfd != -1)
        {
            // syslog(LOG_SYSLOG, "Accepted connection from %s\n", acceptsockaddr.sa_data);
            // printf("******Connected to = %s\n", acceptsockaddr.sa_data);
            if (ListHead == 0)
            {
                listaddhead = malloc(sizeof(struct slist_data_s));
                listaddhead->IsComplete = false;
                SLIST_INSERT_HEAD(&head, listaddhead, entries);
                pthread_create(&(listaddhead->Thread_ID), NULL, SendAndReceive, NULL);
                pthread_join(listaddhead->Thread_ID, NULL);
            }

            else
            {
                listaddafter = malloc(sizeof(struct slist_data_s));
                listaddafter->IsComplete = false;
                if (ListHead == 1)
                    SLIST_INSERT_AFTER(listaddhead, listaddafter, entries);
                else
                    SLIST_INSERT_AFTER(listaddold, listaddafter, entries);
                ListHead = 2;
                pthread_create(&(listaddafter->Thread_ID), NULL, SendAndReceive, NULL);
                listaddold = listaddafter;
                pthread_join(listaddafter->Thread_ID, NULL);
            }
        }

        if (success)
        {
            if (caught_sigint)
            {
                printf("Caught SIGINT\n");
                syslog(LOG_SYSLOG, "Caught signal,exiting");
                remove(fileaddress);
                IsDone = false;
                IS_EXIT = true;
                // pthread_join(TimerThreadID, NULL);
            }

            if (caught_sigterm)
            {
                printf("Caught SIGTERM\n");
                syslog(LOG_SYSLOG, "Caught signal,exiting");
                remove(fileaddress);
                IsDone = false;
                IS_EXIT = true;
                // pthread_join(TimerThreadID, NULL);
            }
        }
    }

    freeaddrinfo(servinfo);
    closelog();
    close(socketfd);
    pthread_mutex_destroy(&Lock);
    // pthread_join(TimerThreadID, NULL);
    return 0;
}

void *SendAndReceive(__attribute__((unused)) void *arg)
{
    pthread_mutex_lock(&Lock);
    recvdatabytes = recv(acceptfd, &ReadData, BUFFER_SIZE, 0);
    New_Line = true;
    for (ssize_t i = 0; i < recvdatabytes; i++)
    {
        if (ReadData[i] == '\n')
        {
            New_Line = false;
        }
    }

    if (recvdatabytes == -1)
    {
        printf("Receive data failed\n");
        freeaddrinfo(servinfo);
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    Index = 0;
    memset(WriteFileData, 0, BUFFER_SIZE);
    for (ssize_t i = 0; i < recvdatabytes; i++)
    {
        WriteFileData[i] = ReadData[i];
        Index++;
    }
    file = fopen(fileaddress, filewritepermissions);
    if (file == NULL)
    {
        // syslog(LOG_ERR,"Unable to open the file!!\n");
        printf("Unable to open file\n");
        freeaddrinfo(servinfo);
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s", WriteFileData);
    if (New_Line == true)
    {
        fprintf(file, "%s", "\n");
    }

    fclose(file);
    file = fopen(fileaddress, filereadpermissions);

    if (file == NULL)
    {
        printf("Could not open file to read and send data\n");
        freeaddrinfo(servinfo);
        close(socketfd);
        exit(EXIT_FAILURE);
    }
    while (fgets(line, sizeof(line), file))
    {
        send(acceptfd, line, strlen(line), 0);
    }
    fclose(file);
    if (ListHead == 0)
    {
        // pthread_join(listaddhead->Thread_ID, NULL);
        ListHead = 1;
    }
    // else
    //     pthread_join(listaddafter->Thread_ID, NULL);

    pthread_mutex_unlock(&Lock);
    return 0;
}

void *TimerRoutine(__attribute__((unused)) void *argument)
{

    while (IsDone)
    {
        t = time(NULL);
        Info = localtime(&t);
        strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", Info);
        strcat(s, "\n");
        memset(timestring, 0, 200);
        strcpy(timestring, "timestamp:");
        strcat(timestring, s);
        sleep(10);
        pthread_mutex_lock(&Lock);
        file = fopen(fileaddress, filewritepermissions);
        if (file == NULL)
        {
            printf("Unable to open file\n");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%s", timestring);
        fclose(file);
        pthread_mutex_unlock(&Lock);
    }
    return 0;
}
