/* =============================================================================
 * function name : am.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of acquisition manager (am) 
 * =============================================================================*/
#include "am.h"
#include "../rtos/kiwi_wrapper.h"
#include "fft_search.h"
#include "../tm/tm.h"
#include <stdio.h>
#include <stdlib.h>
#include "../tm/tracklist.h"

static acq_search_parameters acq_settings;
extern S16 inbuffer[SAMPS_PER_MSEC*20];
extern track_array_list track_info_array_list;
// declare acq results queue
acq_result_queue acqResQueue, *pAcqResQueue;
bool is_blind_search_done = false;

// initailized acquisition result queue
void initAcqResultQueue(acq_result_queue *pAcqResQueue) 
{
  pAcqResQueue->front = 0;
  pAcqResQueue->tail = 0;
}

// initialized acquisition result queue
bool isAcqResultQueueEmpty(acq_result_queue *pAcqResQueue) 
{
  return (pAcqResQueue->front == pAcqResQueue->tail);
}

// get element number in the queue
int getAcqResultQueueLength(acq_result_queue *pAcqResQueue)
{
  int temp = pAcqResQueue->tail - pAcqResQueue->front;
  return (temp < 0)? temp+20:temp;
}

// push element into acquisiton result queue
bool pushAcqResultQueue( acq_result_queue *pAcqResQueue, acq_search_results acqResult)
{
  if ( getAcqResultQueueLength(pAcqResQueue) < 19 )   // 0 - 18
  {
    pAcqResQueue->a[pAcqResQueue->tail] = acqResult;
    pAcqResQueue->tail++;
    if ( pAcqResQueue->tail  == 20 )
      pAcqResQueue -= 20;
    return true;
  }
  else 
    return false;
}

// pop element from acquisition result queue
bool popAcqResultQueue( acq_result_queue *pAcqResQueue, acq_search_results *pAcqResult)
{
  if ( isAcqResultQueueEmpty(pAcqResQueue) )
      return false;
  else
  {
    (*pAcqResult) = pAcqResQueue->a[pAcqResQueue->front];
    pAcqResQueue->front++;
    if (pAcqResQueue->front == 20)
      pAcqResQueue->front = 0;
    return true;
  }
}

// initialize search parameters
void acq_init_search_parameters( int prn, acq_search_parameters *p_acq )
{
  p_acq->prn = prn;
  p_acq->coherent_ms = 1;               // 1ms coherent ms
  p_acq->non_coherent_ms = 10;          // 10 non_coherent_ms
  p_acq->freq_search_range = 10.0e3;    // search range +/- 5 KHz
  p_acq->freq_search_step = 500;        // search step is 500 Hz
}


void am_proc(void)
{
    printf("am starts to run! task_id = %d\n", kiwi_get_taskID());
    // initialzie acq result queue;
    pAcqResQueue = &acqResQueue;
    initAcqResultQueue(pAcqResQueue);
    if ( isAcqResultQueueEmpty( pAcqResQueue))
        printf("acquisition result queue is empty\n");
  
  // read in 12 ms data, real, short
  //size_t eleNum;
  //eleNum = fread(inbuffer, sizeof(S16), 4000*12, pDataFile);

    int prn = 1;
    // start main loop
    while(1)
    {
        kiwi_run_next_task();

        /* start to do fft acq search */
        // initiate search paramters
        //acq_init_search_parameters(prn, &acq_settings);

        if(is_blind_search_done == false)
        {
            for(prn = 1; prn < 33; prn++)
            {
                if(check_satellite_in_track_array_list(&track_info_array_list,prn)==0)
                    fft_search(prn, inbuffer);
            }
            is_blind_search_done=true;
        }
        printf("am is executed\n");
    }
}
