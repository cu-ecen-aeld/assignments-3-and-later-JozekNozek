#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include "aesdsocket.h"

bool signalReceived = false;

static void signalHandler(int signal_number)
{
    if( (signal_number == SIGINT) || (signal_number == SIGTERM) ) signalReceived = true;
}



static char* read_file_bytes(const char* filename, size_t* size_out, pthread_mutex_t *mutex)
{
    /* OBTAIN MUTEX */
    int mutexLockCode = pthread_mutex_lock(mutex); 
    if( mutexLockCode )
    {
        printf("[ERROR%d (%s)] Failed to lock mutex in read file!\r\n", errno, strerror(errno));
        return NULL;
    }
    /* OBTAIN MUTEX */

    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen failed");
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek failed");
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        perror("ftell failed");
        fclose(file);
        return NULL;
    }

    rewind(file);

    char* buffer = malloc(size);
    if (!buffer) {
        perror("malloc failed");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read != (size_t)size) {
        perror("fread incomplete");
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);

    /* RELEASE MUTEX */
    int mutexUnlockCode = pthread_mutex_unlock(mutex);
    if( mutexUnlockCode )
    {
        printf("[ERROR%d (%s)] Failed to unlock mutex in read file!\r\n", errno, strerror(errno));
        // What to do here???
    }
    /* RELEASE MUTEX */

    if( size_out ) *size_out = size;
    return buffer;
}

static bool writeToFile(const char *filename, char *data, int len, pthread_mutex_t *mutex, char *errorRet)
{
    /* OBTAIN MUTEX */
    int mutexLockCode = pthread_mutex_lock(mutex); 
    if( mutexLockCode )
    {
        printf("[ERROR%d (%s)] Failed to lock mutex!\r\n", errno, strerror(errno));
        if( errorRet ) strcpy(errorRet, strerror(errno));
        return false; 
    }
    /* OBTAIN MUTEX */

    FILE *fptr = fopen(filename, "a+");
    if( !fptr )
    {
        printf("[ERROR%d (%s)] NULL file pointer to %s, option a+\r\n", errno, strerror(errno), filename);
        if( errorRet ) strcpy(errorRet, strerror(errno));
        return false;
    }

    bool retval = true;
    if( fwrite(data, 1, len, fptr) != len )
    {
        printf("[ERROR%d (%s)] Not all data written to %s\r\n", errno, strerror(errno), filename);
        if( errorRet ) strcpy(errorRet, strerror(errno));
        retval = false;
    }

    if( fclose(fptr) != 0 )
    {
        printf("[ERROR%d (%s)] fclose(%s) FAIL!\r\n", errno, strerror(errno), filename);
        if( errorRet ) strcpy(errorRet, strerror(errno));
        retval = false;
    }

    /* RELEASE MUTEX */
    int mutexUnlockCode = pthread_mutex_unlock(mutex);
    if( mutexUnlockCode )
    {
        printf("[ERROR%d (%s)] Failed to unlock mutex!\r\n", errno, strerror(errno));
        if( errorRet ) strcpy(errorRet, strerror(errno));
        retval = false;
    }
    /* RELEASE MUTEX */
    
    return retval;
}

void* threadfunc(void* thread_param)
{
    struct thread_data *thread_func_args = (struct thread_data *)thread_param;
    
    char *recvBuff = malloc(MAX_BUFF_ALLOC_SIZE);
    if( !recvBuff )
    {
        printf("[ERROR %d (%s)] malloc returned NULL\r\n", errno, strerror(errno));
        return NULL;
    }

    while( 1 )
    {    
        memset(recvBuff, 0, MAX_BUFF_ALLOC_SIZE);
        int recvBytes = recv(thread_func_args->socketfd, recvBuff, MAX_BUFF_ALLOC_SIZE, 0);
        if( recvBytes < 1 )
        {
            if( recvBytes == 0 && thread_func_args->ip ) { syslog(LOG_DEBUG, "Closed connection from %s", thread_func_args->ip); }
            else { printf("[ERROR %d (%s)] recv from socket fd %d returned: %d\r\n", errno, strerror(errno), thread_func_args->socketfd, recvBytes); }
            break;
        }
        else
        {
            char *errorMsg = malloc(32);
            memset(errorMsg, 0, 32);
            if( !writeToFile(SOCKET_RECV_FILE, recvBuff, recvBytes, thread_func_args->mutex, errorMsg) )
            {
                // File error handling?
                // Read errorMsg
            }
            else
            {
                size_t size;
                char *filesenddata = read_file_bytes(SOCKET_RECV_FILE, &size, thread_func_args->mutex);
                if( filesenddata )
                {
                    printf("Read %zu bytes from %s\n", size, SOCKET_RECV_FILE);
                    int sendcode = send(thread_func_args->socketfd, filesenddata, size, 0);
                    free(filesenddata);
                    if( sendcode == -1 )
                    {
                        printf("[ERROR %d (%s)] send to socket fd %d FAILED!\r\n", errno, strerror(errno), thread_func_args->socketfd);
                        break;
                    }
                }
            }
            free(errorMsg);
        }
    }

    free(recvBuff);

    return NULL;
}

void* timestampThread(void* thread_param)
{
    struct timespec sleepparams = {10, 0};
    struct timespec sleepremainder = {0};
    struct timespec currenttime = {0};

    while( !signalReceived )
    {
        nanosleep(&sleepparams, &sleepremainder);
        // if( sleepremainder.tv_sec > 0 ) break;
        clock_gettime(CLOCK_REALTIME, &currenttime);
        struct tm *tm_info = localtime(&currenttime.tv_sec);
        char *buffer = malloc(64);
        if( buffer )
        {
            strftime(buffer, 64, "%Y-%m-%d %H:%M:%S", tm_info);
            char writetime[64] = "timestamp:";
            strcat(writetime, buffer);
            strcat(writetime, "\n");
            free(buffer);
            writeToFile(SOCKET_RECV_FILE, writetime, strlen(writetime), (pthread_mutex_t *)thread_param, NULL);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int retval = 0;

    int currPID = 0;
    printf("\r\n\r\nLets run assignment 6\r\n\r\n");

    bool daemon_mode = false;
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        printf("\r\nDAEMON MODE ON\r\n");
        daemon_mode = true;
    }

    pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
    if( !mutex ) return -1;
    int mutexInitCode = pthread_mutex_init(mutex, NULL);
    if( mutexInitCode )
    {
        printf("[ERROR %d (%s)] pthread_mutex_init returned: %d, FAIL!\r\n", errno, strerror(errno), mutexInitCode);
        free(mutex);
        return -1;
    }

    slist_socket_threads_t *threadlist_p = NULL;
    SLIST_HEAD(slisthead, slist_socket_threads_s) head;
    SLIST_INIT(&head);

    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(struct sigaction));
    sig_action.sa_handler = signalHandler;

    if( sigaction(SIGTERM, &sig_action, NULL) != 0 )
    {
        printf("[ERROR %d (%s)] Registering handler for SIGTERM FAIL!\r\n", errno, strerror(errno));
        return -1;
    }

    if( sigaction(SIGINT, &sig_action, NULL) != 0 )
    {
        printf("[ERROR %d (%s)] Registering handler for SIGINT FAIL!\r\n", errno, strerror(errno));
        return -1;
    }

    struct addrinfo *sockaddrinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    openlog(NULL, LOG_CONS, LOG_USER);

    int socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if( socketfd == -1 )
    {
        // some error logging
        printf("[ERROR %d (%s)] socket syscall returned -1\r\n", errno, strerror(errno));
        pthread_mutex_destroy(mutex);
        free(mutex);
        return -1;
    }

    int yes = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt");
        printf("[ERROR %d (%s)] setsockopt returned -1\r\n", errno, strerror(errno));
    }

    int addrinforeturncode = getaddrinfo(NULL, "9000", &hints, &sockaddrinfo);
    if( addrinforeturncode != 0 )
    {
        // some error logging
        printf("[ERROR %d (%s)] getaddrinfo returned -1\r\n", errno, strerror(errno));
        pthread_mutex_destroy(mutex);
        free(mutex);
        return -1;
    }

    pthread_t *timestamp_thread;

    do
    {
        int bindreturncode = bind(socketfd, sockaddrinfo->ai_addr, sockaddrinfo->ai_addrlen);
        if( bindreturncode == -1 )
        {
            // some error logging
            printf("[ERROR%d (%s)] bind syscall returned -1\r\n", errno, strerror(errno));
            retval = -1;
            break;
        }

        if( daemon_mode )
        {
            int pid = fork();
            currPID = pid;
            if( pid == -1 )
            {
                printf("[ERROR%d (%s)] fork fail\r\n", errno, strerror(errno));
                break;
            }

            if( pid != 0 )
            {
                pthread_mutex_destroy(mutex);
                free(mutex);
                exit(0);
            }
            // else 
            // {
            //     setsid();
            //     chdir("/");
            //     umask(0);
            //     close(STDIN_FILENO);
            //     close(STDOUT_FILENO);
            //     close(STDERR_FILENO);
            // }
        }
        

        int listenreturncode = listen(socketfd, 5);     // Could try with 0 maybe instead of 5?
        if( listenreturncode == -1 )
        {
            // some error logging
            printf("[ERROR%d (%s)] listen syscall returned -1\r\n", errno, strerror(errno));
            retval = -1;
            break;
        }

        timestamp_thread = malloc(sizeof(pthread_t));
        if( !timestamp_thread )
        {
            printf("[ERROR] malloc failed for timestamp_thread\n");
            retval = -1;
            break;
        }

        int threadCreateCode = pthread_create(timestamp_thread, NULL, timestampThread, mutex);
        if( threadCreateCode )
        {
            printf("[ERROR %d (%s)] Could not create thread!\r\n", errno, strerror(errno));
            retval = -1;
            break;
        }

        while( !signalReceived )
        {
            struct sockaddr recvConnAdr;
            socklen_t len = sizeof(struct sockaddr);
            int acceptfd = accept(socketfd, &recvConnAdr, &len);
            if( acceptfd == -1 )
            {
                // some error logging
                printf("[ERROR %d (%s)] accept returned -1\r\n", errno, strerror(errno));
                retval = -1;
                break;
            }
            
            struct sockaddr_in *connAdr = (struct sockaddr_in *)&recvConnAdr;
            char *ip = malloc(16);
            if( ip )
            {
                memset(ip, 0, 16);
                inet_ntop(AF_INET, &connAdr->sin_addr, ip, 16);
                printf("\r\nACCEPTED CONNECTION FROM ADDR: %s\r\n", ip);
                syslog(LOG_DEBUG, "Accepted connection from %s", ip);
            }
            

            struct thread_data *thrdata = (struct thread_data *)malloc(sizeof(struct thread_data));
            if( !thrdata )
            {
                printf("[ERROR %d (%s)] malloc for thread data returned -1\r\n", errno, strerror(errno));
                close(acceptfd);
                retval = -1;
                break;
            }
            thrdata->mutex = mutex;
            thrdata->socketfd = acceptfd;
            thrdata->ip = ip;

            pthread_t *thread = malloc(sizeof(pthread_t));
            threadCreateCode = pthread_create(thread, NULL, threadfunc, thrdata);
            if( threadCreateCode )
            {
                printf("[ERROR %d (%s)] Could not create thread!\r\n", errno, strerror(errno));
                close(acceptfd);
                free(thread);
                retval = -1;
                break;
            }

            threadlist_p = malloc(sizeof(slist_socket_threads_t));
            if( threadlist_p )
            {
                threadlist_p->thread = thread;
                threadlist_p->thrdata = thrdata;
                SLIST_INSERT_HEAD(&head, threadlist_p, entries);
            }
            else
            {
                printf("[ERROR %d (%s)] Could not allocate memory for threads list element!\r\n", errno, strerror(errno));
                break;
            }

            /*******************************************************************************************/
        }

        syslog(LOG_DEBUG, "Caught signal, exiting");
        printf("\r\nCaught signal, exiting\r\n");

        while( !SLIST_EMPTY(&head) )
        {
            threadlist_p = SLIST_FIRST(&head);

            int joincode = pthread_join(*threadlist_p->thread, NULL);
            if( joincode ) printf("[ERROR %d (%s)] pthread join returned fail!\r\n", errno, strerror(errno));

            free(threadlist_p->thrdata->ip);
            close(threadlist_p->thrdata->socketfd);
            free(threadlist_p->thrdata);
            free(threadlist_p->thread);
            SLIST_REMOVE_HEAD(&head, entries);
            free(threadlist_p);
        }

        if( retval == -1 ) break;

    } while(0);

    /* LAST FUNCTION CALLS: free allocated memory to avoid memory leaks */
    free(timestamp_thread);
    pthread_mutex_destroy(mutex);
    free(mutex);
    remove(SOCKET_RECV_FILE);
    freeaddrinfo(sockaddrinfo);
    close(socketfd);
    closelog();

    printf("\r\n\r\n%s process finished, returning: %d\r\n\r\n", currPID == 0 ? "CHILD" : "PARENT", retval);

    return retval;
}