/* =============================================================================
 * function name : nm.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of navigation manager (nm)
 * =============================================================================*/
#include <stdio.h>
#include "../rtos/kiwi_wrapper.h"

void nm_proc(void)
{
  printf("nm starts to run! task_id = %d\n", kiwi_get_taskID());
  while(1)
  {
    kiwi_run_next_task();
    printf("nm is executed\n");
  }
}
