/*==================================================================
 * function name : global_variables.c
 *        author : Qiang Chen
 *          date : 12/10/2015
 * 
 *   description : this function defines several global varialbes and functions.
 *                 To use these variables/functions, you need to include the 
 *                 header file.
 *================================================================== */
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "kiwi_wrapper.h"

/* prototype of actual thread entry function */
typedef void (*kiwi_ThreadFunc_t)(void);

/* define task_start structure */
typedef struct kiwi_task_start_s
{
  U16 task_id;
  kiwi_ThreadFunc_t entry;
} kiwi_task_start_t;

/* define task structure */
typedef struct kiwi_task_s
{
  U16   task_id;                        // an integer to represent the task
  char  name[MAX_TASK_NAME_LENGTH];     // task name

  pthread_mutex_t mutex;                // protect the thread shared variables
  char  msg_buf[MAX_MSG_BUFFER_LENGTH]; // actual buffer of message struct
  kiwi_msg_t message;                   // message.buffer should be msg_buf;
  kiwi_ThreadFunc_t entry;              // the actual entry function
  sem_t  sem_wait_startup;              // use this to block before running actual entry function

} kiwi_task_t;

/* declaration of tasks array */
static kiwi_task_t gnss_tasks[MAX_TASK_NUM];
static sem_t serTaskSem[MAX_TASK_NUM];
static BOOLEAN serTaskSem_initialized[MAX_TASK_NUM];
static pthread_key_t kiwi_key;

/* initialize kiwi_key */
void kiwi_setup( void )
{
  int ret;
  // create key varialbe 
  ret = pthread_key_create( &kiwi_key, NULL );
  assert( ret == 0 );

  // Initialize the serialization semaphore
  int i;
  for ( i = TASK_ID_MAIN; i <= TASK_ID_PF; i++ )
  {
    sem_init( &serTaskSem[i], 0, 0 );
    serTaskSem_initialized[i] = 0;
  }
  // for main_task, post it once
  sem_post( &serTaskSem[1] );
}

/* generic entry function 
 * data contains task_id and actual entry function
 */
void *generic_thread_entry(void *data)
{
  // just run the entry function
  kiwi_task_start_t *p_start = (kiwi_task_start_t *)data;
  kiwi_task_t *p_task = &gnss_tasks[p_start->task_id];

  long temp_task_id = p_start->task_id;
  // associate task_id with thread local storage
  pthread_setspecific( kiwi_key, (void *)temp_task_id );

  // for main task, it will not block here
  sem_wait(&p_task->sem_wait_startup);

  // run the acual function
  p_start->entry();

  return NULL;
}

/* get the task id */
U16 kiwi_get_taskID(void)
{
  long task_id = 1;
  task_id = (long)pthread_getspecific( kiwi_key );
  return (U16)task_id;
}

/* create thread, use generic_thread_entry as entry function */
inline void kiwi_create_thread( U16 task_id,
                        void (*entry_func)(void)
                        )
{
  pthread_attr_t attr;
  int  ret = pthread_attr_init(&attr);
  assert( ret == 0);

  // fill task_start, pass this as argument
  kiwi_task_start_t *p_task_start;
  p_task_start = (kiwi_task_start_t *)malloc( sizeof(kiwi_task_start_t) );
  p_task_start->task_id = task_id;
  p_task_start->entry = entry_func;

  // create thread
  pthread_t thread;
  ret = pthread_create( &thread,
                        &attr,
                        generic_thread_entry,
                        p_task_start
                        );
  assert(ret == 0);
}


/* create task/thread */
int kiwi_create_task_thread ( U16  task_id,
                            const char *task_name,
                            void (*entry_func)(void)
                            )   
{
  // make sure task thread 
  assert( task_id > 0 && task_id < MAX_TASK_NUM );
  // get the correct 
  kiwi_task_t *p_task;
  p_task = &gnss_tasks[task_id];

  // initiate mutex and lock
  pthread_mutex_init(&p_task->mutex, NULL);
  pthread_mutex_lock(&p_task->mutex);

  p_task->task_id = task_id;          // task_id
  strcpy(p_task->name, task_name);    // task_name
  p_task->entry = entry_func;         // actual entry function
  // Initiate msg
  kiwi_init_msg( &(p_task->message), p_task->msg_buf );
  
  // main_task need not wait at start up
  if ( task_id > TASK_ID_MAIN && task_id <= TASK_ID_PF )
    sem_init(&p_task->sem_wait_startup, 0, 0);  // it will block at start up
  else
    sem_init(&p_task->sem_wait_startup, 0, 1);  // it will not block at start up

  // unlock mutex
  pthread_mutex_unlock(&p_task->mutex);

  // create thread
  kiwi_create_thread( task_id, entry_func);

  // it means all the gnss_tasks fields are set 
  return 0;
}

/* let all threads go - before they are waiting at startup */
void kiwi_go(void)
{
  sem_post(&gnss_tasks[TASK_ID_TM].sem_wait_startup);
  sem_post(&gnss_tasks[TASK_ID_AM].sem_wait_startup);
  sem_post(&gnss_tasks[TASK_ID_NM].sem_wait_startup);
  sem_post(&gnss_tasks[TASK_ID_PF].sem_wait_startup);
}

/* get next task id */
U16 kiwi_get_next_taskID(U16 task_id)
{
  U16 next_task_id;

  if ( task_id == TASK_ID_MAIN )
    next_task_id = TASK_ID_TM;
  else if( task_id == TASK_ID_TM )
    next_task_id = TASK_ID_AM;
  else if( task_id == TASK_ID_AM )
    next_task_id = TASK_ID_NM;
  else if( task_id == TASK_ID_NM )
    next_task_id = TASK_ID_PF;
  else if( task_id == TASK_ID_PF )
    next_task_id = TASK_ID_MAIN;
  else
    assert( 1 == 0);

  return next_task_id;
}

/* suspend itself, run next task */
void kiwi_run_next_task( void )
{
  // get current task_id
  U16 task_id = kiwi_get_taskID();
  if ( !serTaskSem_initialized[task_id] )
  {    
    sem_wait(&serTaskSem[task_id]);   // for the first time, all tasks except main will suspend here;
    serTaskSem_initialized[task_id] = 1;
  }

  // wake up next task
  U16 next_task_id;
  next_task_id = kiwi_get_next_taskID( task_id ) ;
  sem_post(&serTaskSem[next_task_id]);

  // suspend itself
  sem_wait(&serTaskSem[task_id]);
}

// get the availabe space in message buffer
int kiwi_get_message_buffer_space( kiwi_msg_t *p_msg )
{
  S32 free_space = p_msg->head - p_msg->tail;
  if ( free_space > 0 )
    return (U32)( free_space - 1 );
  else
    return (U32)( free_space + MAX_MSG_BUFFER_LENGTH - 1 );
}

// send message
void kiwi_send_message( U16 task_id, void *msg_ptr, int bytes )
{
  // get task pointer
  assert( task_id > 0 && task_id <= TASK_ID_PF );
  kiwi_task_t *p_task = &gnss_tasks[task_id];

  // lock mutex
  pthread_mutex_lock(&p_task->mutex);

  int available_bytes = kiwi_get_message_buffer_space( &p_task->message );
  assert( available_bytes > bytes );

  int i;
  char *p_des = p_task->message.buffer + p_task->message.tail;
  char *p_src = (char*)msg_ptr;
  for ( i = 0; i < bytes; i++  )
  {
    (*p_des++) = (*p_src++);
    p_task->message.tail++;
    // it is the end of buffer, then move it to head
    if ( p_des - p_task->message.buffer == MAX_MSG_BUFFER_LENGTH )
    {
      p_des = p_task->message.buffer;
      p_task->message.tail = 0;
    }
  }
  // unlocak
  pthread_mutex_unlock(&p_task->mutex);
}

// copy message from tasks to the specified address
void kiwi_get_message(U16 task_id, void *p_des )
{
  // get task pointer
  assert( task_id > 0 && task_id <= TASK_ID_PF );
  kiwi_task_t *p_task = &gnss_tasks[task_id];

  // lock mutext
  pthread_mutex_lock(&p_task->mutex);

  // figure out the message length
  char *p_src = p_task->message.buffer + p_task->message.head;
  S32 *p_len  = (S32 *)( p_src + sizeof(S32)*2) ;
  S32 message_length  = *p_len;
  int i;
  char *p_des2 = (char*)p_des;
  for ( i = 0; i < message_length; i++ )
  {
    *p_des2++ = *p_src++;
    p_task->message.head++;
    if ( p_src - p_task->message.buffer == MAX_MSG_BUFFER_LENGTH )
    {
      p_src = p_task->message.buffer;
      p_task->message.head = 0;
    }
  }
  // unlock mutex
  pthread_mutex_unlock(&p_task->mutex);
}
