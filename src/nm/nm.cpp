/* =============================================================================
 * function name : nm.c
 *        author : Qiang Chen
 *          date : Dec. 10, 2015
 *
 *   description : this file defines the function of navigation manager (nm)
 * =============================================================================*/
#include <stdio.h>
#include <math.h>
#include <assert.h>
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

/* decode subframe 1*/
void decodeSubframe1(U32 *pSrc, decode_data_t *pDst)
{
    // get 10 words
    U32 TLW     =   pSrc[0]; 
    U32 HOW     =   pSrc[1];
    U32 word3   =   pSrc[2];
    U32 word4   =   pSrc[3];
    U32 word5   =   pSrc[4];
    U32 word6   =   pSrc[5];
    U32 word7   =   pSrc[6];
    U32 word8   =   pSrc[7];
    U32 word9   =   pSrc[8];
    U32 word10  =   pSrc[9];
    // decode data
    U8 subframe  =  HOW>>8 & 0x7;
    U32 tow      =  (HOW>>13 & 0x1ffff)*6;
    U32 week     =  word3>>20 & 0x3ff;
    U8  uraIndex =  word3>>14 & 0xf;
    U8  svHealth =  word3>>8  & 0x3f;
    U32 toc      =  (word8>>6 & 0xffff)* 16;
    U16 iodc     =  (word8>>22)&0xff;   // iodc 8 lsb
    iodc         =  iodc | ((word3&0xc0)<<2);                                
    double tgd   =  (S8)(word7>>6 & 0xff) * pow(2,-31);
    S32 af0_int  =  ((S32)((word10&0x3fffff00)<<2))>>10;
    double af0   =  (double)af0_int * pow(2.0,-31);
    S16 af1_int  =  (word9>>6) & 0xffff;
    double af1   =  (double)af1_int * pow(2.0,-43);
    S8  af2_int  =  (word9>>22)&0xff;
    double af2   =  (double)af2_int * pow(2.0,-55);
    // save decoded data
    pDst->gpstime.gpsWeek       =   week + 1024;
    pDst->gpstime.gpsSeconds    =   tow;
    pDst->info.URA              =   uraIndex;
    pDst->info.health           =   svHealth;
    pDst->clkCorr.tgd           =   tgd;
    pDst->clkCorr.af0           =   af0;
    pDst->clkCorr.af1           =   af1;
    pDst->clkCorr.af2           =   af2;
    pDst->clkCorr.toc           =   toc;
    pDst->clkCorr.IODC          =   iodc;
    // set ready flag
    pDst->ready                 |= 1;
    assert(subframe == 1);
}

/* decode subframe 2*/
void decodeSubframe2(U32 *pSrc, decode_data_t *pDst)
{    
    // get 10 words
    U32 TLW     =   pSrc[0]; 
    U32 HOW     =   pSrc[1];
    U32 word3   =   pSrc[2];
    U32 word4   =   pSrc[3];
    U32 word5   =   pSrc[4];
    U32 word6   =   pSrc[5];
    U32 word7   =   pSrc[6];
    U32 word8   =   pSrc[7];
    U32 word9   =   pSrc[8];
    U32 word10  =   pSrc[9];
    // decode data
    U8 subframe     =   HOW>>8 & 0x7;
    U32 tow         =   (HOW>>13 & 0x1ffff)*6;
    U8 iode2        =   word3>>22 & 0xff;                                     
    S16 Crs_i       =   (word3 & 0x3fffc0)>>6;
    double Crs      =   Crs_i * (1/32.0);
    S16 deltan      =   (word4&0x3fffc000)>>14;
    double delta_n  =   deltan * pow(2.0, -43);
    S32 M0_i        =   (word4<<18&0xff000000) | (word5>>6&0x00ffffff);
    double M0       =   M0_i * pow(2.0, -31);
    S16 Cuc_i       =   (word6&0x3fffc000)>>14;
    double Cuc      =   Cuc_i * pow(2.0, -29);
    U32 e_i         =   (word6<<18&0xff000000) | (word7>>6&0x00ffffff);
    double e        =   e_i * pow(2, -33);
    S16 Cus_i       =   (word8&0x3fffc000)>>14;
    double Cus      =   Cus_i * pow(2, -29);
    U32 sqrtA_i     =   (word8<<18&0xff000000) | (word9>>6&0x00ffffff);
    double a_sqrt   =   sqrtA_i * pow(2.0, -19);
    U32 toe         =   ((word10&0x3fffc000)>>14)*16;
    // save decoded data
    pDst->eph.IODE2     = iode2;
    pDst->eph.Crs       = Crs;
    pDst->eph.delta_n   = delta_n;
    pDst->eph.M0        = M0;
    pDst->eph.Cuc       = Cuc;
    pDst->eph.Cus       = Cus;
    pDst->eph.e         = e;
    pDst->eph.sqrt_a    = a_sqrt;
    pDst->eph.toe       = toe;
    // set ready flag
    pDst->ready         |= (1<<1);
    assert(subframe == 2);
}

/* decode subframe 3*/
void decodeSubframe3(U32 *pSrc, decode_data_t *pDst)
{
    // get 10 words
    U32 TLW     =   pSrc[0]; 
    U32 HOW     =   pSrc[1];
    U32 word3   =   pSrc[2];
    U32 word4   =   pSrc[3];
    U32 word5   =   pSrc[4];
    U32 word6   =   pSrc[5];
    U32 word7   =   pSrc[6];
    U32 word8   =   pSrc[7];
    U32 word9   =   pSrc[8];
    U32 word10  =   pSrc[9];
    // decode data
    U8 subframe         =   HOW>>8 & 0x7;
    U32 tow             =   (HOW>>13 & 0x1ffff)*6;
    S16 Cic_i           =   (word3&0x3fffc000)>>14;
    double Cic          =   Cic_i* pow(2.0, -29);
    S32 Omega0_i        =   (word3<<18 & 0xff000000) | (word4>>6 & 0x00ffffff);
    double Omega0       =   Omega0_i * pow(2.0,-31);
    S16 Cis_i           =   (word5&0x3fffc000)>>14;
    double Cis          =   Cis_i * pow(2.0, -29);
    S32 i0_i            =   (word5<<18&0xff000000) | (word6>>6&0x00ffffff);
    double i0           =   i0_i * pow(2.0, -31);
    S16 Crc_i           =   (word7&0x3fffc000)>>14;
    double Crc          =   Crc_i * pow(2.0,-5);
    S32 omega_i         =   (word7<<18&0xff000000) | (word8>>6&0x00ffffff);
    double omega        =   omega_i * pow(2.0,-31);
    S32 Omega_dot_i     =   ((S32)(word9<<2))>>8;
    double Omega_dot    =   Omega_dot_i * pow(2.0,-43);
    U8 iode3            =   (word10>>22)&0xff;
    S16 idot_i          =   ((S32)(word10<<10))>>18;
    double idot         =   idot_i*pow(2.0, -43);
    // save decoded data
    pDst->eph.IODE3     =   iode3;
    pDst->eph.Cic       =   Cic;
    pDst->eph.Omega0    =   Omega0;
    pDst->eph.Cis       =   Cis;
    pDst->eph.i0        =   i0;
    pDst->eph.Crc       =   Crc;
    pDst->eph.omega     =   omega;
    pDst->eph.Omega_dot =   Omega_dot;
    pDst->eph.i_dot     =   idot;
    // set ready flag
    pDst->ready         |=  (1<<2);
    assert(subframe == 3);
}

/* print out decoded message */
void print_decodedMsg(decode_data_t *pDecodedMsg)
{
    /* print satellite info */
    printf("URA is %d\n", pDecodedMsg->info.URA);
    printf("health is %d\n", pDecodedMsg->info.health);
    printf("\n");
    /* print gps time */
    printf("gps week is %d\n", pDecodedMsg->gpstime.gpsWeek);
    printf("gps secdons is %d\n", pDecodedMsg->gpstime.gpsSeconds);
    printf("\n");
    /* printf clock correction */
    printf("IODC is %d\n", pDecodedMsg->clkCorr.IODC);
    printf("toc is %d\n", pDecodedMsg->clkCorr.toc);
    printf("tgd is %.12e\n", pDecodedMsg->clkCorr.tgd);
    printf("af0 is %.12e\n", pDecodedMsg->clkCorr.af0);
    printf("af1 is %.12e\n", pDecodedMsg->clkCorr.af1);
    printf("af2 is %.12e\n", pDecodedMsg->clkCorr.af2);
    printf("\n");
    /* print ephemeris */
    printf("IODE2 is %d\n", pDecodedMsg->eph.IODE2);
    printf("IODE3 is %d\n", pDecodedMsg->eph.IODE3);
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
        //printf("nm is executed\n");
    }
}
