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
    struct thread_data *td;

    td = (struct thread_data *)thread_param;
    if (usleep(td->wait_for * 1000) < 0) {
        perror("usleep");
        return thread_param;
    }
    pthread_mutex_lock(td->mutex);
    if (usleep(td->wait_release * 1000) < 0)  {
        perror("usleep");
        return thread_param;
    }
    pthread_mutex_unlock(td->mutex);
    td->thread_complete_success = true;
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
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
    struct thread_data *thread_data;

    if ((thread_data = (struct thread_data *)malloc(sizeof(struct thread_data))) ==
        (struct thread_data *)NULL)  {
            ERROR_LOG("malloc");
            return false;
        }
    thread_data -> mutex = mutex;
    thread_data -> wait_for = wait_to_obtain_ms;
    thread_data -> wait_release = wait_to_release_ms;

    if (pthread_create(thread, NULL, threadfunc, thread_data) != 0)  {
        ERROR_LOG("pthread_create");
        return false;
    }
    return true;
}

