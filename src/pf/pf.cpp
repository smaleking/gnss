/* =============================================================================
 * function name : pf.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of position fix manager (pf)
 * =============================================================================*/
#include <stdio.h>
#include "../rtos/kiwi_wrapper.h"

FILE *pfRinex;
void pf_proc(void)
{
  printf("pf starts to run! task_id = %d\n", kiwi_get_taskID() );
  pfRinex = fopen("tx_time.log", "wt");
  if (pfRinex == NULL) {
      printf("cannot create tx_time.log!\n");
      exit(101);
  }

  while(1)
  {
    kiwi_run_next_task();
    //printf("pf is executed\n");
  }
}
