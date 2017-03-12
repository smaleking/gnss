
/* =============================================================================
 * function name : main.c
 *         input : none
 *        output : none
 *        author : Qiang Chen
 *          date : Dec. 12, 2015
 *
 *   description : this function is to implement the serialization of multiple threads (e.g. 5)
 *                 for each thread entry function, it will first suspend itself and wake the next thread.
 *                 If it is wakened by another thread, then it starts to execute the main process. We use
 *                 semaphore to implement this.
 * =============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "rtos/kiwi_data_types.h"
#include "rtos/kiwi_tasks.h"
#include "rtos/kiwi_wrapper.h"
#include "am/am.h"
#include "tm/tm.h"
#include "nm/nm.h"
#include "pf/pf.h"
// TODO: should I create an interface file to include all the functions to be called ???

// 
bool start = false;
FILE *pDataFile;
S16 inbuffer[SAMPS_PER_MSEC*20];
/* entry function of thread main */
void gnss_main(void)
{
  printf("main task is created!\n");

  /* now start to create the 4 tasks */
  int ret;

  // create TM task, it is important to get ret
  ret =  kiwi_create_task_thread( TASK_ID_TM,
                                  "task_tm",
                                  tm_proc
                                );
  assert( ret == 0 );

  // create AM task
  ret = kiwi_create_task_thread(TASK_ID_AM,
                                "task_am",
                                am_proc
                                );
  assert( ret == 0 );

  // create NM task
  ret = kiwi_create_task_thread(TASK_ID_NM,
                                "task_nm",
                                nm_proc
                                );
  assert( ret == 0 );

  // create PF task
  ret = kiwi_create_task_thread(TASK_ID_PF,
                                "task_pf",
                                pf_proc
                                );
  assert( ret == 0 );

  // unlock sem_wait_startup
  kiwi_go();

  TM_MESSAGE tm_message;
  tm_message.header.source = TASK_ID_MAIN;
  tm_message.header.type   = TM_TAG_INTERRUPT_20MS;
  tm_message.header.length = sizeof(tm_message); // should be 16
  tm_message.ms = 20;
  // forever loop
  while(1)
  {
    // wake up next task and suspect itself
    kiwi_run_next_task();
    size_t eleNum;
    // read in 20 ms data 
    eleNum = fread(inbuffer, sizeof(S16), SAMPS_PER_MSEC*20, pDataFile);
    if (eleNum!= SAMPS_PER_MSEC*20) 
    {
    	printf("There is not enough IF data\n");
        exit(100);
    }

    // send an interrupt every 20 ms
    kiwi_send_message(TASK_ID_TM, (void *)(&tm_message), sizeof(tm_message));
//    printf("Interrupt is sent\n");
  }
}


/* entry of main function
 * it creates a main thread that will create another five threads to raun
 */
int main(int arg, char *argv[])
{
  int ret0;
  pDataFile = fopen("../data/gps_data.bin","rb");
  if (pDataFile == NULL)
  {
    printf("Raw data file does not exist!\n");
    return 1;
  }
  /* TODO: need to move this to somewhere else */
  init_track();
  // create key
  kiwi_setup();
  /* create main thread */
  U16 task_id = TASK_ID_MAIN;
  char task_name[] = "task_main";
  ret0 = kiwi_create_task_thread( task_id,
                                  task_name,
                                  gnss_main
                                 );
  assert(ret0 == 0);
  while(1);
  return 0;
}
