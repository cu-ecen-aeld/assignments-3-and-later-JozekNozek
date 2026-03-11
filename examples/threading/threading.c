#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
// #include <cstdint.h>


// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("threading DEBUG: " msg "\n" , ##__VA_ARGS__)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data *thread_func_args = (struct thread_data *)thread_param;

    static int mutexLockCode, mutexUnlockCode;
    void* status = malloc(sizeof(int));
    // retval = malloc(sizeof(int));
    
    do
    {
        /* WAIT TO OBTAIN MUTEX */
        struct timespec waitToObtain = {thread_func_args->wait_to_obtain_ms / 1000, (thread_func_args->wait_to_obtain_ms % 1000) * 1000000};
        struct timespec waitToObtainRem = {0};
        DEBUG_LOG("WAIT OBTAIN SLEEP: %d seconds, %d miliseconds", (int)waitToObtain.tv_sec, (int)waitToObtain.tv_nsec / 1000000);
        nanosleep(&waitToObtain, &waitToObtainRem);
        int iter = 5;
        while( (waitToObtainRem.tv_sec > 0) && (waitToObtainRem.tv_nsec > 0) && (iter > 0) )
        {
            memcpy(&waitToObtain, &waitToObtainRem, sizeof(struct timespec));
            memset(&waitToObtainRem, 0, sizeof(struct timespec));
            nanosleep(&waitToObtain, &waitToObtainRem);
            iter--;
        }

        mutexLockCode = pthread_mutex_lock(thread_func_args->mutex); 
        if( mutexLockCode )
        {
            ERROR_LOG("Failed to lock mutex, error code: %d", mutexLockCode);
            thread_func_args->exit_code = mutexLockCode;
            thread_func_args->thread_complete_success = false;
            // retval = mutexLockCode;
            memcpy(status, &mutexLockCode, sizeof(int));
            pthread_exit(status); 
        }

        /* WAIT TO RELEASE MUTEX */
        struct timespec waitToRelease = {thread_func_args->wait_to_release_ms / 1000, (thread_func_args->wait_to_release_ms % 1000) * 1000000};
        struct timespec waitToReleaseRem = {0};
        DEBUG_LOG("WAIT RELEASE SLEEP: %d seconds, %d miliseconds", (int)waitToRelease.tv_sec, (int)waitToRelease.tv_nsec / 1000000);
        nanosleep(&waitToRelease, &waitToReleaseRem);
        iter = 5;
        while( (waitToReleaseRem.tv_sec > 0) && (waitToReleaseRem.tv_nsec > 0) && (iter > 0) )
        {
            memcpy(&waitToRelease, &waitToReleaseRem, sizeof(struct timespec));
            memset(&waitToReleaseRem, 0, sizeof(struct timespec));
            nanosleep(&waitToRelease, &waitToReleaseRem);
            iter--;
        }

        mutexUnlockCode = pthread_mutex_unlock(thread_func_args->mutex);
        if( mutexUnlockCode )
        {
            ERROR_LOG("Failed to unlock mutex, error code: %d", mutexUnlockCode);
            thread_func_args->exit_code = mutexUnlockCode;
            thread_func_args->thread_complete_success = false;
            memcpy(status, &mutexUnlockCode, sizeof(int));
            pthread_exit(status);
        }

        thread_func_args->thread_complete_success = true;

    } while(0);

    // pthread_detach()

    DEBUG_LOG("RETURNING FROM THREAD");

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    bool retval = false;

    do
    {
        // int mutexInitCode = pthread_mutex_init(mutex, NULL);
        // if( mutexInitCode )
        // {
        //     ERROR_LOG("Could not create mutex, error code: %d", mutexInitCode);
        //     break;
        // }

        struct thread_data *thrdata = (struct thread_data *)malloc(sizeof(struct thread_data));
        if( !thrdata )
        {
            ERROR_LOG("malloc returned NULL");
            break;
        }
        thrdata->mutex = mutex;
        thrdata->wait_to_obtain_ms = wait_to_obtain_ms;
        thrdata->wait_to_release_ms = wait_to_release_ms;

        int threadCreateCode = pthread_create(thread, NULL, threadfunc, thrdata);
        if( threadCreateCode )
        {
            ERROR_LOG("Could not create thread, error code: %d", threadCreateCode);
            break;
        }

        retval = true;

    } while(0);

    return retval;
}

