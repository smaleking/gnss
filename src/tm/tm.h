/* =========================================================
*
*     filename :  tm.h
*       author :  Qiang Chen
*         date :  Dec. 10, 2015
*
*  description :  the header file of tracking manager (tm)
*
*===========================================================*/
#ifndef TM_H
#define TM_H

#include "../rtos/kiwi_wrapper.h"
#include "tracklist.h"

#define TM_TAG_INTERRUPT_20MS 1

// define message type
typedef struct tm_message{
  msg_header header;
  U32 ms;
} TM_MESSAGE;

//correlator
void initPRNCodeTable(int *prncodeTable);
void init_track();
void carrierTrackingSDR(track_information *one_satellite_info);
void codeTrackingSDR(track_information *one_satellite_info);
void carrierTracking(track_information *one_satellite_info);
void codeTracking(track_information *one_satellite_info);
void correlate(track_information *one_satellite_info, char *inbuffer);

void tm_proc(void);

#endif
