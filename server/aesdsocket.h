#ifndef __AESDSOCKET_H_
#define __AESDSOCKET_H_

#include <stdbool.h>
#include <pthread.h>
#include <sys/queue.h>

#define SOCKET_RECV_FILE        "/var/tmp/aesdsocketdata.txt"
#define MAX_BUFF_ALLOC_SIZE     (1024 * 1024)

struct thread_data
{
    int socketfd;
    pthread_mutex_t *mutex;
    char *ip;
    
    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    bool thread_complete_success;
};

typedef struct slist_socket_threads_s slist_socket_threads_t;
struct slist_socket_threads_s
{
    pthread_t *thread;
    struct thread_data *thrdata;
    SLIST_ENTRY(slist_socket_threads_s) entries;
};

#endif