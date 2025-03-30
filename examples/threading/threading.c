#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    //

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    //printf("thread:%d sleep start :%d \n",(int)thread_func_args->tid,thread_func_args->wait_to_obtain_ms );
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    //printf("thread:%d wakeup and trying to get mutex \n",(int)thread_func_args->tid);
    if (0 != pthread_mutex_lock(thread_func_args->lock))
    {
	 return 0;
    }
    //printf("thread:%d wakeup and got mutex \n",(int)thread_func_args->tid);
    usleep(thread_func_args->wait_to_release_ms*1000);
    if (0 != pthread_mutex_unlock(thread_func_args->lock))
    {

	    return 0;
    }
    //printf("thread:%d release mutex \n",(int)thread_func_args->tid);
    //thread_func_args->callback(thread_param);
    //free(thread_param);
    thread_func_args->thread_complete_success = true; 
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
    struct thread_data *params = malloc(sizeof(struct thread_data));
    if (NULL == params || NULL == mutex ) 
    {
	    return false;
    }
    params->lock = mutex;
    params->wait_to_obtain_ms = wait_to_obtain_ms;
    params->wait_to_release_ms = wait_to_release_ms;
    params->thread_complete_success = false;

    int rc = pthread_create(&(params->tid), NULL, threadfunc, (void *)params); 
    if (rc != 0) {
            printf("\nThread can't be created :[%s]", strerror(rc)); 
	    free(params);
    	    return false;
    }
    *thread = params->tid; 
    
    printf("Thread id:%d created succesfully\n", (int)(params->tid));

    return true;
    
}

