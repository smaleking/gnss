/* =============================================================================
 * function name : pf.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of position fix manager (pf)
 * =============================================================================*/
#include <stdio.h>
#include "../rtos/kiwi_wrapper.h"

void pf_proc(void)
{
  printf("pf starts to run! task_id = %d\n", kiwi_get_taskID() );
  while(1)
  {
    kiwi_run_next_task();
    sleep(1);
    printf("pf is executed\n");
  }
}
