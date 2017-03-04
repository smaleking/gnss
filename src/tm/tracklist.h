/*
 * tracklist.h
 *
 *  Created on: Aug 10, 2016
 *      Author: dma
 */

#ifndef TRACKLIST_H_
#define TRACKLIST_H_

#include "kiwi_data_types.h"
#include "ShiftRegister.h"

#define	TRACK_LIST_MAX (32)

// define track_information struct
typedef struct track_information_s{
	  int prn;
	  //int codePhase;	  
	  double codeFreqBasis;
      //carrier nco
      double carrierFreq;
      double carrierFreqBasis;	  
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
      int P_i_20ms, P_q_20ms;
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
      int state;               // 1. tracking 2. bit sync 3. frame sync
      unsigned long oneMsCnt;  // one ms count 
      int mod20cnt;    // from 0 to 19
      int bitIdx;      // from 0 to 29, bit index within a word (30 bits)
      int wordIdx;     // from 0 to 9,  word index within a subframe (10 words)
      int subframeIdx; // from 0 to 4;
      int Bits[20];
	  int BitFlipCnt[20];
      char polarKnown;
      char polarPositive;
      U32 wordBuffer[20];       // 2 consecutive subframes
      U32 frameData[5][10];
} track_information;

// define track_array
typedef struct track_array_list_s{
	track_information info[TRACK_LIST_MAX];
	int capacity;
} track_array_list;

// 1. initialzie teh track_arrayList object
void init_track_array_list(track_array_list* track_info_array_list);

// 2. check if a satellite is in track list
int check_satellite_in_track_array_list(track_array_list *track_info_array_list, int prn);

// 3. add a channel to channel list
void add_track_info_to_array_list(track_array_list *track_info_array_list, track_information *one_satellite_info);

// 4. remove a channel from channel list
void remove_track_info_from_array_list(track_array_list * track_info_array_list, int prn);

// 5. get the active channel size
int track_info_array_list_size(track_array_list *track_info_array_list);

// 6. get track info
track_information *get_track_info(track_array_list *track_info_array_list, int index);

#endif /* TRACKLIST_H_ */
