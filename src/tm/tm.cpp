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
void carrierTrackingSDR(track_information *ch)
{
	double carrError;
	carrError = atan((double)ch->measuresIQ[1][1] / (double)ch->measuresIQ[1][0]) / (2.0 * pi);
	ch->carrNco = ch->oldCarrNco + (pll_filter_parameters.tau2 / pll_filter_parameters.tau1)
			* (carrError - ch->oldCarrError) + carrError * 0.001 / pll_filter_parameters.tau1;
	ch->oldCarrNco = ch->carrNco;
	ch->oldCarrError = carrError;
	ch->carrierFreq = ch->carrierFreqBasis + ch->carrNco;
	ch->carrierPhaseStep = (unsigned int)(pow(2,32) * ch->carrierFreq / SAMPLING_FREQUENCY);

	sprintf(testBuffer, "%f\n", ch->carrierFreq);
	fputs(testBuffer, testFileCarrierFreq);
	fflush(testFileCarrierFreq);
}

// 4. code tracking loop
void codeTrackingSDR(track_information *ch)
{
	double early, late, codeError;
	early = sqrt(ch->measuresIQ[0][0] * ch->measuresIQ[0][0] + ch->measuresIQ[0][1] * ch->measuresIQ[0][1]);
	late = sqrt(ch->measuresIQ[2][0] * ch->measuresIQ[2][0] + ch->measuresIQ[2][1] * ch->measuresIQ[2][1]);
	codeError = (early-late)/(early+late)/2;
	ch->codeNco = ch->oldCodeNco + dll_filter_parameters.tau2 / dll_filter_parameters.tau1
			* (codeError - ch->oldCodeError) + codeError * 0.001 / dll_filter_parameters.tau1;
	ch->oldCodeNco = ch->codeNco;
	ch->oldCodeError = codeError;
	ch->codeFreq = ch->codeFreqBasis + ch->codeNco;
	ch->codePhaseStep = (unsigned int)(pow(2,32) * ch->codeFreq * ch->halfChipSamples / SAMPLING_FREQUENCY);

	sprintf(testBuffer, "%f\n", ch->codeFreq);
	fputs(testBuffer, testFileCodeFreq);
	fflush(testFileCodeFreq);
}

//  not used?
void carrierTracking(track_information *ch)
{
	double cross, dot;
	double freqError, phaseError, carrierLoopError1st;
	dot = ch->prePromptIQ[0] * ch->measuresIQ[1][0] + ch->prePromptIQ[1] * ch->measuresIQ[1][1];
	cross = ch->prePromptIQ[0] * ch->measuresIQ[1][1] - ch->prePromptIQ[1] * ch->measuresIQ[1][0];
	if((ch->prePromptIQ[0] != 0) && (ch->measuresIQ[1][0] != 0))
	{
		freqError = atan(cross/dot);
		phaseError = atan((float)(ch->measuresIQ[1][1])/(float)(ch->measuresIQ[1][0]));

		ch->carrierLoopError3rd = ch->carrierLoopError3rd + 0.001 * (secondOrderFreqFilter.C2 * freqError + thirdOrderPhaseFilter.C3 * phaseError);
		ch->carrierLoopError2nd = ch->carrierLoopError2nd + 0.001 * (secondOrderFreqFilter.C1 * freqError + thirdOrderPhaseFilter.C2 * phaseError + ch->carrierLoopError3rd);
		carrierLoopError1st = ch->carrierLoopError2nd + thirdOrderPhaseFilter.C1 * phaseError;
		ch->carrierFreq -= carrierLoopError1st;
		ch->carrierPhaseStep = (unsigned int)(pow(2,32) * ch->carrierFreq / SAMPLING_FREQUENCY);

		sprintf(testBuffer, "%f\n", ch->carrierFreq);
		fputs(testBuffer, testFileCarrierFreq);
		fflush(testFileCarrierFreq);
	}
}

// not used ?
void codeTracking(track_information *ch)
{
	double early,late, earlyLateError, codeFreqError;
	early = sqrt(ch->measuresIQ[0][0] * ch->measuresIQ[0][0] + ch->measuresIQ[0][1] * ch->measuresIQ[0][1]);
	late = sqrt(ch->measuresIQ[2][0] * ch->measuresIQ[2][0] + ch->measuresIQ[2][1] * ch->measuresIQ[2][1]);
	earlyLateError = -(early - late) / (early + late);
	ch->codeLoopError2nd = ch->codeLoopError2nd + 0.001 * secondOrderCodeFilter.C2 * earlyLateError;
	codeFreqError = ch->codeLoopError2nd + secondOrderCodeFilter.C1 * earlyLateError;
	ch->codeFreq += codeFreqError/5;
	ch->codePhaseStep = (unsigned int)(pow(2,32) * ch->codeFreq * ch->halfChipSamples / SAMPLING_FREQUENCY);
	sprintf(testBuffer, "%f\n", ch->codeFreq);
	fputs(testBuffer, testFileCodeFreq);
	fflush(testFileCodeFreq);
}

// 6. correlation for 20 ms
void correlate(track_information *ch, char *inbuffer)
{
	int i,j;
	int cossin[2] = {0};
	int currentCode;
	int complexData[2];

    // sample by sample processing
	for(i = 0; i < SAMPS_PER_MSEC*40; i=i+2)
	{
		// carrier nco tick
		ch->carrierPhase += ch->carrierPhaseStep;
		if(((ch->carrierPhase &0x80000000) == 0) && ((ch->carrierPhaseBack & 0x80000000) != 0))
		{
			ch->carrierPhaseCycleCount++;
		}
		ch->carrierPhaseBack = ch->carrierPhase;
		ch->carrierPhaseIndex = ch->carrierPhase >> 27; // 32 loop-up table
		cossin[0] =  costable[ch->carrierPhaseIndex];
		cossin[1] = -sintable[ch->carrierPhaseIndex];

		// code nco tick
		ch->codeCycle = 0;
		currentCode = prncodetable[ch->prn-1][ch->codePhaseIndex];

		ch->codePhase += ch->codePhaseStep;
		ch->codeClockMultiX = (((ch->codePhase & 0x80000000) ^ ch->codePhaseBack) != 0) ? 1 : 0;
		if(ch->codeClockMultiX ==1)
		{
            // shift in half chip sample
			shiftIn(&(ch->taps), currentCode);
			ch->divideCount++;
            // one chip boundary
			if(ch->divideCount >= ch->fullChipSamples)
			{
				ch->divideCount=0;
				ch->codePhaseIndex++;
                // one code period 
				if(ch->codePhaseIndex >= ch->codePeriodLength)
				{
					ch->codePhaseIndex = 0;
					ch->codeCycle = 1;
				}
			}
		}
		ch->codePhaseBack = ch->codePhase & 0x80000000;

		// carrier removal, get baseband signal
		complexData[0] = (inbuffer[i] * cossin[0] - inbuffer[i+1] * cossin[1]) >> 3;
		complexData[1] = (inbuffer[i] * cossin[1] + inbuffer[i+1] * cossin[0]) >> 3;

		//correlate with E, P, L local codes
		for(j = 0; j < 3; j++)
		{
			ch->correlates[j][0] += ch->taps.taps[j] * complexData[0];  // real
			ch->correlates[j][1] += ch->taps.taps[j] * complexData[1];  // imag
		}

		// 1ms boundary
		if(ch->codeCycle == 1)
		{
            // save prompt I/Q
			ch->prePromptIQ[0] = ch->measuresIQ[1][0];
			ch->prePromptIQ[1] = ch->measuresIQ[1][1];
            // update 1ms cnt
            ch->oneMsCnt++;
            ch->mod20cnt = (ch->mod20cnt+1)%20;
            int mod20cnt = ch->mod20cnt;
            ch->Bits[mod20cnt] = (ch->correlates[1][0])>0? 1:-1;
            if (ch->oneMsCnt>1000 && ch->state==1 )    // > 1s, assume stable tracking loop
            {
                int preMod20cnt = (ch->mod20cnt == 0)? 19: ch->mod20cnt-1;
                if (ch->Bits[mod20cnt] != ch->Bits[preMod20cnt]) 
                {
                    ch->BitFlipCnt[mod20cnt]++;
                    if (ch->BitFlipCnt[mod20cnt] > 5)
                    {
                        for (int k = 0; k < 20; k++)
                            if( ch->BitFlipCnt[k] > 1 && k != mod20cnt)
                                exit(1);
                        ch->state = 2;  // in bit sync mode
                        ch->mod20cnt  = 0;
                    }
                }
            }
            // save for measurement
			for (j = 0; j < 3; j++)
			{
				ch->measuresIQ[j][0] = ch->correlates[j][0]>>5;
				ch->measuresIQ[j][1] = ch->correlates[j][1]>>5;
				ch->correlates[j][0] = 0;
				ch->correlates[j][1] = 0;
			}
            if (ch->state == 2) // bit synced
            {
                if (ch->mod20cnt == 0)
                    ch->P_i_20ms =  ch->measuresIQ[1][0];
                else
                    ch->P_i_20ms += ch->measuresIQ[1][0];
                if (ch->mod20cnt ==19)
                {
                    int currBit = (ch->P_i_20ms > 0)?1:0;
                    // save currBit
                    for (int k = 0; k < 19; k++)
                    {
                        ch->wordBuffer[k] <<= 1; 
                        ch->wordBuffer[k] |= ((ch->wordBuffer[k+1]&0x20000000)>>29);
                    }
                    ch->wordBuffer[19] <<= 1;
                    ch->wordBuffer[19] |= currBit;
                    // check if have preamble or inv-preamble
                    if ((ch->wordBuffer[0]&0x3fc00000)>>22==0x8b || (ch->wordBuffer[0]&0x3fc00000)>>22==0x74)
                    {
                        if ((ch->wordBuffer[10]&0x3fc00000)>>22==0x8b || (ch->wordBuffer[10]&0x3fc00000)>>22==0x74)
                        {
                            printf("subframe sync!\n");
                        }
                        else
                            exit(2);
                    }
                }

            }
			// save 1ms correlation values after bit sync
            if (ch->state == 2) 
            {
                if (ch->prn == 5)
			    {
                    fprintf(testFileIQ, "%d, %d, %d, %d, %d, %d\n", 
                                        ch->measuresIQ[0][0],ch->measuresIQ[0][1],
                                        ch->measuresIQ[1][0],ch->measuresIQ[1][1],
                                        ch->measuresIQ[2][0],ch->measuresIQ[2][1]);
			    }
            }

            // loop update at 1ms boundary
			carrierTrackingSDR(ch);
			codeTrackingSDR(ch);
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
			track_information ch;
			ch.prn = temp.prn;
			ch.carrierFreq = 20000-1956;//temp.carrier_freq;
			ch.carrierFreqBasis = 20000-1956;
			ch.codeFreqBasis=1023000.0;
			ch.carrierPhase = 0;
			ch.carrierPhaseIndex = 0;
			ch.carrierPhaseCycleCount = 0;
			ch.carrierPhaseStep = (unsigned int)(pow(2,32) * ch.carrierFreq / SAMPLING_FREQUENCY);

			ch.codeFreq = 1023000.0;
			ch.codePhaseIndex = (4000 - temp.code_phase)*1023/4000 + 1;
			ch.halfChipSamples = 1;
			ch.fullChipSamples = 2;
			ch.codePeriodLength = 1023;
			ch.codePhaseStep = (unsigned int)(pow(2,32) * 1023000 * ch.halfChipSamples / SAMPLING_FREQUENCY);
			initShiftRegisterTaps(&(ch.taps),1);

			ch.carrierLoopError3rd = 0;
			ch.carrierLoopError2nd = 0;
			ch.codeLoopError2nd = 0;

            ch.P_i_20ms = 0;
            ch.P_q_20ms = 0;
            ch.oneMsCnt = 0;
            ch.mod20cnt = 0;
            ch.state    = 1;
            for (int k = 0; k < 20; k++) {
                ch.BitFlipCnt[k] = ch.Bits[k] = 0;
                ch.wordBuffer[k] = 0;
            }
            ch.polarKnown = 0;
            ch.polarPositive = 0;

            // add channel
			add_track_info_to_array_list(&track_info_array_list, &ch);
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
