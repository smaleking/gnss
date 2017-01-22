/* =============================================================================
 * function name : nm.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of navigation manager (nm)
 * =============================================================================*/
#include <stdio.h>
#include "nm.h"
#include "../rtos/kiwi_wrapper.h"

struct decode_data_t decode_data[32];

U8 parity_check(U32 *p_dword, U32 *p_data)
{
  /* get the parity checksum */
  U8 parity_sum;
  int bit_count;
  U32 dword = *p_dword;

  /* get the parity checksum */
  parity_sum = dword &0x3f;   // get the last 6 bits (parity checksum)

  /* Remove the effect of D30* to recover d1...d24 */
  if ((dword&0x40000000) != 0 ) {
      *p_data = *p_dword ^ 0x3fffffc0;
      dword  = *p_data;
  } else {
      *p_data = *p_dword;
  }
  
  /* For bits D29* through d24 calculate the 6 parity bits */
  for (bit_count = 0; bit_count < 26; bit_count++)
  {
    if (dword & 0x80000000) 
        parity_sum ^= parity_table[bit_count];
    // move 1 bit left
    dword <<= 1;
  }
  if (!parity_sum)
      return 0x00;
  else
      return 0x01;
}

void nm_proc(void)
{
  printf("nm starts to run! task_id = %d\n", kiwi_get_taskID());
  while(1)
  {
    kiwi_run_next_task();
//    printf("nm is executed\n");
  }
}
