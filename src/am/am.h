/* =========================================================
*
*     filename :  am.h
*       author :  Qiang Chen
*         date :  12/09/2015
*
*  description :  the header file of acquisition manager(am)
*
*===========================================================*/
#ifndef AM_H
#define AM_H

#include "../inc/rx_config.h"
#include "../rtos/kiwi_data_types.h"
#include <stdbool.h>
/*************************************** 
 * Below we define several structures *
 ************************************* */

// buffered data for acquisition
typedef struct acq_data_s {
  short buffer[20*SAMPS_PER_MSEC]; // 20ms data buffer
  int   scount;                    // scount of buffer start
  BOOLEAN valid;                  // indicate if the buffered data is valid
  BOOLEAN streaming;               // indicate if buffer is streaming new data
  short ms_count;                 // when streaming data, use this to indicate how much data are buffered
} acq_data;

// acq search parameters
typedef struct acq_search_parameters_s {
  int prn;
  int coherent_ms;
  int non_coherent_ms;
  double freq_search_range;
  double freq_search_step;
} acq_search_parameters;

// acq search resutls
typedef struct acq_search_results_s {
  int prn;
  int code_phase;
  double carrier_freq;
} acq_search_results;

// acq resutls buffer: a queue
typedef struct acq_search_results_queue {
  acq_search_results a[20];
  int front;
  int tail;  
} acq_result_queue;


extern acq_result_queue acqResQueue, *pAcqResQueue;

/*************************************** 
  * Below we define several functions *
 ************************************* */
// initialize acqisition result queue
void initAcqResultQueue(acq_result_queue *pAcqResQueue);

// initialized acquisition result queue
bool isAcqResultQueueEmpty(acq_result_queue *pAcqResQueue);

// get element number in the queue
int getAcqResultQueueLength(acq_result_queue *pAcqResQueue);

// push element into acquisiton result queue
bool pushAcqResultQueue( acq_result_queue *pAcqResQueue, acq_search_results acqResult);

// pop element from acquisition result queue
bool popAcqResultQueue( acq_result_queue *pAcqResQueue, acq_search_results *pAcqResult);

// set up search parameters
void acq_init_search_parameters( int prn, acq_search_parameters *p_acq );

/* this is the fft search function.
   if search is successful, return 1; otherwise return 0
 */
BOOLEAN acq_search( acq_search_parameters *p_acq, acq_search_results *p_result);

// task_function
void am_proc(void);

#endif
