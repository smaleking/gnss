/* =========================================================
*
*     filename :  nm.h
*       author :  Qiang Chen
*         date :  Dec. 10, 2015
*
*  description :  the header file of navigation message manager(nm)
*
*===========================================================*/
#ifndef NM_H
#define NM_H

#include "kiwi_data_types.h"

typedef struct {
    U32 week;       // gps week
    double tow;     // gps time of week
    U64 superCount; // 1ms interrupt count
}  rx_time_t;

extern rx_time_t rxClock;

const U8 parity_table[] =
{
  0x29, 0x16, 0x2a, 0x34,  // D29*, D30*, d1, d2
  0x3b, 0x1c, 0x2f, 0x37,  //  d3,  d4, d5, d6
  0x1a, 0x0d, 0x07, 0x23,  //  d7,  d8, d9, d10
  0x31, 0x38, 0x3d, 0x3e,  // d11, d12, d13, d14
  0x1f, 0x0e, 0x26, 0x32,  // d15, d16, d17, d18
  0x19, 0x2c, 0x16, 0x0b,  // d19, d20, d21, d22
  0x25, 0x13 };            // d23, d24


// struct of ephemeris
struct ephemeris_t {
    U8 IODE2;           // issue of data ephemeris, 8 bits, from subframe 2
    U8 IODE3;           // issue of data ephemeris, 8 bits, from subframe 3
    U32 toe;            // time of ephemeris, week seconds
    double sqrt_a;      // sqrt of seme-major
    double e;           // eccentricity
    double i0;          // inclination angle
    double Omega0;      // RAAN at week epoch
    double omega;       // argument of perigee
    double M0;          // Mean anomaly at Reference time
    double delta_n;     // Mean motion correction
    double i_dot;       // rate of inclination angle
    double Omega_dot;   // rate of RAAN
    double Cuc;         // cosine correction to argument of latitude
    double Cus;         // sine correction to argument of latitude
    double Crc;         // cosine correction to radius
    double Crs;         // sine correction to radius
    double Cic;         // cosine correcton to inclination
    double Cis;         // sine correction to inclination
};

// struct of gps time
struct gps_time_t {
    U32 gpsWeek;
    U32 gpsSeconds;
};

// struct of gps clock 
struct clock_correction_t {
    U16 IODC;
    U32 toc;            //  clock data reference time
    double tgd;         //  group delay
    double af0;         //  clock corrections
    double af1;         
    double af2;
};

// struct of sat info
struct sat_info_t {
    U8 URA;             // user range accuracy
    U8 health;          // 0 is health
};

// combined decoded data 
struct decode_data_t {
    ephemeris_t eph;
    gps_time_t  gpstime;
    clock_correction_t clkCorr;
    sat_info_t  info;
    U8 ready;   // bit-wise indicactor: sf1 bit0, sf2 bit1, sf3 bit2, sf4 bit3, sf5 bit4
};

extern decode_data_t decode_data[32];


/* parity check for each word */
U8 parity_check(U32 *p_dword, U32 *p_data);

/* decode subframe 1*/
void decodeSubframe1(U32 *pSrc, decode_data_t *pDst);

/* decode subframe 2*/
void decodeSubframe2(U32 *pSrc, decode_data_t *pDst);

/* decode subframe 3*/
void decodeSubframe3(U32 *pSrc, decode_data_t *pDst);

/* print out decoded message */
void print_decodedMsg(decode_data_t *pDecodedMsg);

/* main process of nm */
void nm_proc(void);

#endif
