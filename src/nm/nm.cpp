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

decode_data_t decode_data[32];

/* parity check for each word */
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

/* print out decoded message */
void print_decodedMsg(decode_data_t *pDecodedMsg)
{
    /* print gps time */
    printf("gps week is %d\n", pDecodedMsg->gpstime.gpsWeek);
    printf("gps secdons is %d\n", pDecodedMsg->gpstime.gpsSeconds);
    printf("\n");
    /* printf clock correction */
    printf("IODC is %x\n", pDecodedMsg->clkCorr.IODC);
    printf("toc is %d\n", pDecodedMsg->clkCorr.toc);
    printf("tgd is %.12e\n", pDecodedMsg->clkCorr.tgd);
    printf("af0 is %.12e\n", pDecodedMsg->clkCorr.af0);
    printf("af1 is %.12e\n", pDecodedMsg->clkCorr.af1);
    printf("af2 is %.12e\n", pDecodedMsg->clkCorr.af2);
    printf("\n");
    /* print satellite info */
    printf("URA is %d\n", pDecodedMsg->info.URA);
    printf("health is %d\n", pDecodedMsg->info.health);
    /* print ephemeris */
    printf("IODE is %d\n", pDecodedMsg->eph.IODE);
    printf("toe is %d\n", pDecodedMsg->eph.toe);
    printf("sqrt_a is %.12e\n", pDecodedMsg->eph.sqrt_a);
    printf("e is %.12e\n", pDecodedMsg->eph.e);
    printf("i0 is %.12e\n", pDecodedMsg->eph.i0);
    printf("Omega0 is %.12e\n", pDecodedMsg->eph.Omega0);
    printf("omega is %.12e\n", pDecodedMsg->eph.omega);
    printf("M0 is %.12e\n", pDecodedMsg->eph.M0);
    printf("delta_n is %.12e\n", pDecodedMsg->eph.delta_n);
    printf("i_dot is %.12e\n", pDecodedMsg->eph.i_dot);
    printf("Omega_dot is %.12e\n", pDecodedMsg->eph.Omega_dot);
    printf("Cuc is %.12e\n", pDecodedMsg->eph.Cuc);
    printf("Cus is %.12e\n", pDecodedMsg->eph.Cus);
    printf("Crc is %.12e\n", pDecodedMsg->eph.Crc);
    printf("Crs is %.12e\n", pDecodedMsg->eph.Crs);
    printf("Cic is %.12e\n", pDecodedMsg->eph.Cic);
    printf("Cis is %.12e\n", pDecodedMsg->eph.Cis);
}

/* main process of nm*/
void nm_proc(void)
{
  printf("nm starts to run! task_id = %d\n", kiwi_get_taskID());
  while(1)
  {
    kiwi_run_next_task();
//    printf("nm is executed\n");
  }
}
