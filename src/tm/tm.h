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
#include "ShiftRegister.h"

#define TM_TAG_INTERRUPT_20MS 1

// define message type
typedef struct tm_message{
  msg_header header;
  U32 ms;
} TM_MESSAGE;


typedef struct track_information_s{
	  int prn;
	  //int codePhase;
	  double carrierFreqBasis;
	  double codeFreqBasis;
	  double carrierFreq;
	  //carrier nco
	  unsigned int carrierPhase;
	  unsigned int carrierPhaseBack;
	  unsigned int carrierPhaseCycleCount;
	  unsigned int carrierPhaseIndex;
	  unsigned int carrierPhaseStep;
	  //code nco
	  double codeFreq;
	  unsigned int codePhase;
	  unsigned int codePhaseBack;
	  unsigned int codePhaseStep;
	  unsigned int codePhaseIndex;
	  int divideCount;
	  int codeCycle;
	  int codeClockMultiX;
	  int halfChipSamples;
	  int fullChipSamples;
	  int codePeriodLength;
	  ShiftRegisterTaps taps;
	  //collerate results
	  int correlates[3][2];//early prompt late collerate results
	  int prePromptIQ[2];
	  int measuresIQ[3][2];
	  //loop filters
	  double carrierLoopError3rd;
	  double carrierLoopError2nd;
	  double codeLoopError2nd;
	  //loop filters SDR
	  double oldCarrNco;
	  double carrNco;
	  double oldCarrError;

	  double oldCodeNco;
	  double codeNco;
	  double oldCodeError;
	  //nav bits
	  unsigned int twoFrameBits[20];
} track_information;


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
