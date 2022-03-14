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
    
    struct thread_data* thread_func_args = (struct thread_data*) thread_param;
    
    //Wait
    int rc = usleep(thread_func_args->wait_to_obtain_ms);
    if(rc == -1)
    {
    	printf("\n\rusleep for wait to obtain ms failed\n\r");
    	thread_func_args->thread_complete_success = false;
    	return thread_param;
    }
    

    
    //Lock mutex
    rc = pthread_mutex_lock(thread_func_args->mutex);
    if(rc != 0)
    {
    	printf("\n\rpthread_mutex_lock failed\n\r");
    	thread_func_args->thread_complete_success = false;
		return thread_param;
    }
    
    
    //wait
    rc = usleep(thread_func_args->wait_to_release_ms);
    if(rc == -1)
    {
    	printf("\n\rusleep for wait to obtain ms failed\n\r");
    	thread_func_args->thread_complete_success = false;
    	return thread_param;
    }
    
    //release mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if(rc != 0)
    {
    	printf("\n\rpthread_mutex_lock failed\n\r");
    	thread_func_args->thread_complete_success = false;
    	return thread_param;
    }
    
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
     
     if(mutex == NULL)
     {
     	printf("\n\rMutex is NULL\n\r");
     	return false;
     }
     
     
    //Allocate memory for thread data
    struct thread_data* t_data = malloc(sizeof(struct thread_data));
    if(t_data == NULL)
    {
    	printf("malloc failed\n\r");
    	return false;
    }
    
    //setup mutex and wait arguments
    
    t_data->mutex = mutex;
    t_data->wait_to_obtain_ms = wait_to_obtain_ms;
    t_data->wait_to_release_ms = wait_to_release_ms;

    //Pass thread_data to created thread

    int status = 0;
      
    status = pthread_create(thread, NULL,threadfunc,t_data);
    if(status != 0)
    {
  	  printf("\n\rpthread_create failed\n\r");
  	  return false;
    }
      
    return true;
}

