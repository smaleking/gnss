/* =============================================================================
 * function name : tm.c
 *        author : Qiang Chen
 *          date : Dec. 9, 2015
 *
 *   description : this file defines the function of tracking manager (tm)
 * =============================================================================*/
#include <stdio.h>
#include <math.h>
#include "../rtos/kiwi_wrapper.h"
#include "../am/am.h"
#include "tm.h"
#include "tracklist.h"
#include "loopfilterconfig.h"
#include "../utils/cacode.h"
#include "../inc/constantparameters.h"
#include "../inc/rx_config.h"

// IF sample buffer
extern S16 inbuffer[SAMPS_PER_MSEC*20];

// 32 cosine/sine lookup table
const int costable[] = { 252, 247, 233, 210, 178, 140, 96, 49, 0, -49, -96, -140, -178, -210, -233, -247, -252, -247, -233, -210, -178, -140, -96, -49, 0, 49, 96, 140, 178, 210, 233, 247 };
const int sintable[] = { 0, 49, 96, 140, 178, 210, 233, 247, 252, 247, 233, 210, 178, 140, 96, 49, 0, -49, -96, -140, -178, -210, -233, -247, -252, -247, -233, -210, -178, -140, -96, -49 };

// tm global variables
int prncodetable[GPS_SAT_NUM][1023] = {0};
filter_parameters pll_filter_parameters,dll_filter_parameters;
ThirdOrderFilterParameters thirdOrderPhaseFilter;
SecondOrderFilterParameters secondOrderFreqFilter;
SecondOrderFilterParameters secondOrderCodeFilter;
track_array_list track_info_array_list;

// log files
FILE *testFileIQ;
FILE *testFileCarrierFreq;
FILE *testFileCodeFreq;
int oneTimeFlag = 0;
char testBuffer[128];

// 1. initial cacode
void initPRNCodeTable(int *prncodeTable)
{
	int i;
	for(i = 0; i < GPS_SAT_NUM ; i++)
        cacode(i+1, prncodeTable+i*1023);
}

// 2. initial track_list and filter related variables
void init_track()
{
	init_track_array_list(&track_info_array_list);
	init_filter_parameters(&pll_filter_parameters,PLL_Noise_Bandwidth, PLL_Damping_Ratio, 0.25);
	init_filter_parameters(&dll_filter_parameters,DLL_Noise_Bandwidth, DLL_Damping_Ratio, 1.0);

	InitThirdOrderFilter(&thirdOrderPhaseFilter, 15);
	InitSecondOrderFilter(&secondOrderFreqFilter, 30);
	InitSecondOrderFilter(&secondOrderCodeFilter, 5);

	initPRNCodeTable(prncodetable[0]);
	testFileIQ = fopen("./testFileIQ.bin","w");
	testFileCarrierFreq = fopen("./testFileCarrierFreq.bin","w");
	testFileCodeFreq = fopen("./testFileCodeFreq.bin","w");
	//testFileOneCode = fopen("/mnt/Win7Share/testFileOneCode.bin","w");
}

// 3. carrier tracking loop
void carrierTrackingSDR(track_information *one_satellite_info)
{
	double carrError;
	carrError = atan((double)one_satellite_info->measuresIQ[PROMPT_TAP_POS][1] / (double)one_satellite_info->measuresIQ[PROMPT_TAP_POS][0]) / (2.0 * pi);
	one_satellite_info->carrNco = one_satellite_info->oldCarrNco + (pll_filter_parameters.tau2 / pll_filter_parameters.tau1)
			                    * (carrError - one_satellite_info->oldCarrError) + carrError * 0.001 / pll_filter_parameters.tau1;
	one_satellite_info->oldCarrNco = one_satellite_info->carrNco;
	one_satellite_info->oldCarrError = carrError;
	one_satellite_info->carrierFreq = one_satellite_info->carrierFreqBasis + one_satellite_info->carrNco;
	one_satellite_info->carrierPhaseStep = (unsigned int)(pow(2,32) * one_satellite_info->carrierFreq / SAMPLING_FREQUENCY);

	sprintf(testBuffer, "%f\n", one_satellite_info->carrierFreq);
	fputs(testBuffer, testFileCarrierFreq);
	fflush(testFileCarrierFreq);
}

// 4. code tracking loop
void codeTrackingSDR(track_information *one_satellite_info)
{
	double early, late, codeError;
	early = sqrt(one_satellite_info->measuresIQ[EARLY_TAP_POS][0] * one_satellite_info->measuresIQ[EARLY_TAP_POS][0] + one_satellite_info->measuresIQ[EARLY_TAP_POS][1] * one_satellite_info->measuresIQ[EARLY_TAP_POS][1]);
	late = sqrt(one_satellite_info->measuresIQ[LATE_TAP_POS][0] * one_satellite_info->measuresIQ[LATE_TAP_POS][0] + one_satellite_info->measuresIQ[LATE_TAP_POS][1] * one_satellite_info->measuresIQ[LATE_TAP_POS][1]);
	codeError = (early-late)/(early+late)/2;
	one_satellite_info->codeNco = one_satellite_info->oldCodeNco + dll_filter_parameters.tau2 / dll_filter_parameters.tau1
			* (codeError - one_satellite_info->oldCodeError) + codeError * 0.001 / dll_filter_parameters.tau1;
	one_satellite_info->oldCodeNco = one_satellite_info->codeNco;
	one_satellite_info->oldCodeError = codeError;
	one_satellite_info->codeFreq = one_satellite_info->codeFreqBasis + one_satellite_info->codeNco;
	one_satellite_info->codePhaseStep = (unsigned int)(pow(2,32) * one_satellite_info->codeFreq * one_satellite_info->halfChipSamples / SAMPLING_FREQUENCY);

	sprintf(testBuffer, "%f\n", one_satellite_info->codeFreq);
	fputs(testBuffer, testFileCodeFreq);
	fflush(testFileCodeFreq);
}

//  not used ?
void carrierTracking(track_information *one_satellite_info)
{
	double cross, dot;
	double freqError, phaseError, carrierLoopError1st;
	dot = one_satellite_info->prePromptIQ[0] * one_satellite_info->measuresIQ[1][0] + one_satellite_info->prePromptIQ[1] * one_satellite_info->measuresIQ[1][1];
	cross = one_satellite_info->prePromptIQ[0] * one_satellite_info->measuresIQ[1][1] - one_satellite_info->prePromptIQ[1] * one_satellite_info->measuresIQ[1][0];
	if((one_satellite_info->prePromptIQ[0] != 0) && (one_satellite_info->measuresIQ[1][0] != 0))
	{
		freqError = atan(cross/dot);
		phaseError = atan((float)(one_satellite_info->measuresIQ[1][1])/(float)(one_satellite_info->measuresIQ[1][0]));

		one_satellite_info->carrierLoopError3rd = one_satellite_info->carrierLoopError3rd + 0.001 * (secondOrderFreqFilter.C2 * freqError + thirdOrderPhaseFilter.C3 * phaseError);
		one_satellite_info->carrierLoopError2nd = one_satellite_info->carrierLoopError2nd + 0.001 * (secondOrderFreqFilter.C1 * freqError + thirdOrderPhaseFilter.C2 * phaseError + one_satellite_info->carrierLoopError3rd);
		carrierLoopError1st = one_satellite_info->carrierLoopError2nd + thirdOrderPhaseFilter.C1 * phaseError;
		one_satellite_info->carrierFreq -= carrierLoopError1st;
		one_satellite_info->carrierPhaseStep = (unsigned int)(pow(2,32) * one_satellite_info->carrierFreq / SAMPLING_FREQUENCY);

		sprintf(testBuffer, "%f\n", one_satellite_info->carrierFreq);
		fputs(testBuffer, testFileCarrierFreq);
		fflush(testFileCarrierFreq);
	}
}

// not used ?
void codeTracking(track_information *one_satellite_info)
{
	double early,late, earlyLateError, codeFreqError;
	early = sqrt(one_satellite_info->measuresIQ[0][0] * one_satellite_info->measuresIQ[0][0] + one_satellite_info->measuresIQ[0][1] * one_satellite_info->measuresIQ[0][1]);
	late = sqrt(one_satellite_info->measuresIQ[2][0] * one_satellite_info->measuresIQ[2][0] + one_satellite_info->measuresIQ[2][1] * one_satellite_info->measuresIQ[2][1]);
	earlyLateError = -(early - late) / (early + late);
	one_satellite_info->codeLoopError2nd = one_satellite_info->codeLoopError2nd + 0.001 * secondOrderCodeFilter.C2 * earlyLateError;
	codeFreqError = one_satellite_info->codeLoopError2nd + secondOrderCodeFilter.C1 * earlyLateError;
	one_satellite_info->codeFreq += codeFreqError/5;
	one_satellite_info->codePhaseStep = (unsigned int)(pow(2,32) * one_satellite_info->codeFreq * one_satellite_info->halfChipSamples / SAMPLING_FREQUENCY);
	sprintf(testBuffer, "%f\n", one_satellite_info->codeFreq);
	fputs(testBuffer, testFileCodeFreq);
	fflush(testFileCodeFreq);
}

// 6. correlation for 20 ms
void correlate(track_information *one_satellite_info, char *inbuffer)
{
	int i,j;
	int cossin[2] = {0};
	int currentCode;
	int complexData[2];

    // sample by sample processing
	for(i = 0; i < SAMPS_PER_MSEC*40; i=i+2)
	{
		// carrier nco tick
		one_satellite_info->carrierPhase += one_satellite_info->carrierPhaseStep;
		if(((one_satellite_info->carrierPhase &0x80000000) == 0) && ((one_satellite_info->carrierPhaseBack & 0x80000000) != 0))
		{
			one_satellite_info->carrierPhaseCycleCount++;
		}
		one_satellite_info->carrierPhaseBack = one_satellite_info->carrierPhase;
		one_satellite_info->carrierPhaseIndex = one_satellite_info->carrierPhase >> 27; // 32 loop-up table
		cossin[0] =  costable[one_satellite_info->carrierPhaseIndex];
		cossin[1] = -sintable[one_satellite_info->carrierPhaseIndex];

		// code nco tick
		one_satellite_info->codeCycle = 0;
		currentCode = prncodetable[one_satellite_info->prn-1][one_satellite_info->codePhaseIndex];

        // update code phase
		one_satellite_info->codePhase += one_satellite_info->codePhaseStep;
		one_satellite_info->codeClockMultiX = (((one_satellite_info->codePhase & 0x80000000) ^ (one_satellite_info->codePhaseBack & 0x80000000)) != 0) ? 1 : 0;
		if(one_satellite_info->codeClockMultiX ==1)
		{
            // shift in half chip sample
			shiftIn(&(one_satellite_info->taps), currentCode);
			one_satellite_info->divideCount++;
            // one chip boundary
			if(one_satellite_info->divideCount >= one_satellite_info->fullChipSamples)
			{
				one_satellite_info->divideCount=0;
				one_satellite_info->codePhaseIndex++;
                // one code period 
				if(one_satellite_info->codePhaseIndex >= one_satellite_info->codePeriodLength)
				{
					one_satellite_info->codePhaseIndex = 0;
					one_satellite_info->codeCycle = 1;
				}
			}
		}
		one_satellite_info->codePhaseBack = one_satellite_info->codePhase;

		// carrier removal, get baseband signal
		complexData[0] = (inbuffer[i] * cossin[0] - inbuffer[i+1] * cossin[1]) >> 3;
		complexData[1] = (inbuffer[i] * cossin[1] + inbuffer[i+1] * cossin[0]) >> 3;

		//correlate with E, P, L local codes
		for(j = 0; j < TM_TAP_NUM; j++)
		{
			one_satellite_info->correlates[j][0] += one_satellite_info->taps.taps[j] * complexData[0];  // real
			one_satellite_info->correlates[j][1] += one_satellite_info->taps.taps[j] * complexData[1];  // imag
		}

		// 1ms boundary
		if(one_satellite_info->codeCycle == 1)
		{
			one_satellite_info->prePromptIQ[0] = one_satellite_info->measuresIQ[1][0];
			one_satellite_info->prePromptIQ[1] = one_satellite_info->measuresIQ[1][1];
			for(j = 0; j < TM_TAP_NUM; j++)
			{
				one_satellite_info->measuresIQ[j][0] = one_satellite_info->correlates[j][0]>>5;
				one_satellite_info->measuresIQ[j][1] = one_satellite_info->correlates[j][1]>>5;
				one_satellite_info->correlates[j][0] = 0;
				one_satellite_info->correlates[j][1] = 0;
			}
			if(one_satellite_info->prn == 5)
			{
                for(int k = 0; k < TM_TAP_NUM; k++)
                    fprintf(testFileIQ, "%d,%d,", one_satellite_info->measuresIQ[k][0], one_satellite_info->measuresIQ[k][1]);

                fprintf(testFileIQ, "\n");
                /*
                sprintf(testBuffer, "\n");
				sprintf(testBuffer,"%d\t%d\t%d\t%d\t%d\t%d\n",one_satellite_info->measuresIQ[0][0],one_satellite_info->measuresIQ[0][1],
						one_satellite_info->measuresIQ[1][0],one_satellite_info->measuresIQ[1][1],
						one_satellite_info->measuresIQ[2][0],one_satellite_info->measuresIQ[2][1]);
				fputs(testBuffer,testFileIQ);
                */
				fflush(testFileIQ);
			}
            // loop update at 1ms boundary
			carrierTrackingSDR(one_satellite_info);
			codeTrackingSDR(one_satellite_info);
		}
	}
}

// tm thread function
void tm_proc(void)
{
  printf( "tm starts to run! task_id = %d\n", kiwi_get_taskID() );
  while(1)
  {
    kiwi_run_next_task();
    // message buffer
    char *p_message = (char*)malloc(4096);
    kiwi_get_message( TASK_ID_TM, p_message );

    // decode message
    S32 msg_source  = *(p_message);
    S32 msg_tag     = *(p_message + 4);
    S32 msg_len     = *(p_message + 8);

    if ( msg_tag == 1)
    {
      acq_search_results temp;
      while (  !isAcqResultQueueEmpty(pAcqResQueue) )
      {
        popAcqResultQueue(pAcqResQueue, &temp);
        if((temp.prn == 5) && (check_satellite_in_track_array_list(&track_info_array_list, 5) == 0))
        {
			track_information one_satellite_info;
			one_satellite_info.prn = temp.prn;
			one_satellite_info.carrierFreq = 20000-1956;//temp.carrier_freq;
			one_satellite_info.carrierFreqBasis = 20000-1956;
			one_satellite_info.codeFreqBasis=1023000.0;
			one_satellite_info.carrierPhase = 0;
			one_satellite_info.carrierPhaseIndex = 0;
			one_satellite_info.carrierPhaseCycleCount = 0;
			one_satellite_info.carrierPhaseStep = (unsigned int)(pow(2,32) * one_satellite_info.carrierFreq / SAMPLING_FREQUENCY);

			one_satellite_info.codeFreq = 1023000.0;
			one_satellite_info.codePhaseIndex = (4000 - temp.code_phase)*1023/4000 + 1;
			one_satellite_info.halfChipSamples = 1;
			one_satellite_info.fullChipSamples = 2;
			one_satellite_info.codePeriodLength = 1023;
			one_satellite_info.codePhaseStep = (unsigned int)(pow(2,32) * 1023000 * one_satellite_info.halfChipSamples / SAMPLING_FREQUENCY);
			initShiftRegisterTaps(&(one_satellite_info.taps),1);

			one_satellite_info.carrierLoopError3rd = 0;
			one_satellite_info.carrierLoopError2nd = 0;
			one_satellite_info.codeLoopError2nd = 0;
			add_track_info_to_array_list(&track_info_array_list, &one_satellite_info);
        }
        printf("prn is %d\n", temp.prn);
        printf("code phase is %d\n", temp.code_phase);
        printf("carrier freqeuncy is %f \n", temp.carrier_freq);
      }

      int track_num = track_info_array_list_size(&track_info_array_list);
      int i;
      // do correlate for each PRN
      for(i = 0;i<track_num;i++)      
      {
    	  correlate(get_track_info(&track_info_array_list, i), (char *)inbuffer);
      }
      printf("tm: 20ms interrupt executed\n");

    }
  }
}
