/* =============================================================================
 * function name : tm.c
 *        author : Qiang Chen
 *          date : Dec. 9, 2015
 *
 *   description : this file defines the function of tracking manager (tm)
 * =============================================================================*/
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "../rtos/kiwi_wrapper.h"
#include "../am/am.h"
#include "../nm/nm.h"
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
            if (ch->state >= 2) // bit synced or frame sync
            {
                if (ch->mod20cnt == 0)
                    ch->P_i_20ms =  ch->measuresIQ[1][0];
                else
                    ch->P_i_20ms += ch->measuresIQ[1][0];
                // new bit
                if (ch->mod20cnt == 19)
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
                    
                    if ( ch->state == 2) 
                    {
                        // check if have preamble or inv-preamble
                        if ((ch->wordBuffer[0]&0x3fc00000)>>22==0x8b || (ch->wordBuffer[0]&0x3fc00000)>>22==0x74)
                        {
                            if ((ch->wordBuffer[10]&0x3fc00000)>>22==0x8b||(ch->wordBuffer[10]&0x3fc00000)>>22==0x74)
                            {
                                // decode TOW for frame sync check
                                U32 TLW1 = ch->wordBuffer[0], HOW1 = ch->wordBuffer[1];
                                U32 TLW2 = ch->wordBuffer[10], HOW2 = ch->wordBuffer[11];
                                U32 mod6_tow_now  = ((TLW1&0x1)==0)? (HOW1&0x3fffe000)>>13: ((HOW1&0x3fffe000)^(0x3fffe000)) >>13;
                                U32 mod6_tow_next = ((TLW2&0x1)==0)? (HOW2&0x3fffe000)>>13: ((HOW2&0x3fffe000)^(0x3fffe000)) >>13;
                                if (mod6_tow_next-mod6_tow_now == 1 || (mod6_tow_next == 0 && mod6_tow_now == 100799))
                                {
                                    ch->state = 3;
                                    // get subframe number
                                    U8 sfIdx = ((TLW1&0x1)==0)? (HOW1>>8&0x7) : ((HOW1>>8&0x7)^(0x7));
                                    printf("subframe is %d\n", sfIdx);

                                    // now parity-check, get the raw bits, and save current two subframes to buffered data
                                    U8 check_result;
                                    for (int k=0; k<20; k++)
                                    {
                                        U8 temp = (k<10)? sfIdx: sfIdx+1;
                                        if (k==0 || k==10)
                                            check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[temp][0]);
                                        else {
                                            U32 tempWord = ch->wordBuffer[k-1] << 30;
                                            ch->wordBuffer[k] &= 0x3fffffff;
                                            ch->wordBuffer[k] |= tempWord;
                                            check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[temp][k%10]);
                                        }
                                        if (check_result != 0) {
                                            printf("word = %d, parity check fails\n", k);
                                        } 
                                    }
                                    // set subframe index and word index
                                    ch->bitIdx = 0;
                                    ch->wordIdx = 0;
                                    ch->subframeIdx = (sfIdx+1)%5; // subframeIdx from 0 to 4
                                }
                            }
                        }
                    }
                    else 
                    {
                        // we have a complete subframe
                        if (ch->wordIdx == 9 && ch->bitIdx == 29) 
                        {
                            U8 check_result;
                            // do parity check, convert to raw bits
                            for (int k = 10; k < 20; k++) {
                                if (k==10)
                                    check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[ch->subframeIdx][0]);
                                else {
                                    U32 temp = ch->wordBuffer[k-1] << 30;
                                    ch->wordBuffer[k] &= 0x3fffffff;
                                    ch->wordBuffer[k] |= temp;      // D29* D30*, D1, D2, ...D30
                                    check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[ch->subframeIdx][k%10]);
                                }
                            }
                            // get 10 words
                            U32 TLW   = ch->frameData[ch->subframeIdx][0];
                            U32 HOW   = ch->frameData[ch->subframeIdx][1];
                            U32 word3 = ch->frameData[ch->subframeIdx][2];
                            U32 word4 = ch->frameData[ch->subframeIdx][3];
                            U32 word5 = ch->frameData[ch->subframeIdx][4];
                            U32 word6 = ch->frameData[ch->subframeIdx][5];
                            U32 word7 = ch->frameData[ch->subframeIdx][6];
                            U32 word8 = ch->frameData[ch->subframeIdx][7];
                            U32 word9 = ch->frameData[ch->subframeIdx][8];
                            U32 word10 = ch->frameData[ch->subframeIdx][9];
                            U32 prn    =  ch->prn;
                            // decode  
                            if (ch->subframeIdx == 0) {             // subframe 1
                                U8 subframe  =  HOW>>8 & 0x7;
                                U32 tow      =  (HOW>>13 & 0x1ffff)*6;
                                printf("subframe is %d, tow is %d\n", subframe, tow);
                                U32 week     =  word3>>20 & 0x3ff;
                                U8  uraIndex =  word3>>14 & 0xf;
                                U8  svHealth =  word3>>8  & 0x3f;
                                U32 toc      =  (word8>>6 & 0xffff)* 16;
                                U16 iodc     =  (word8>>22)&0xff;   // iodc 8 lsb
                                iodc         =  iodc | ((word3&0xc0)<<2);                                
                                /* 2's complement */
                                double tgd   =  (S8)(word7>>6 & 0xff) * pow(2,-31);
                                S32 af0_int  =  ((S32)((word10&0x3fffff00)<<2))>>10;
                                double af0   =  (double)af0_int * pow(2.0,-31);
                                S16 af1_int  =  (word9>>6) & 0xffff;
                                double af1   =  (double)af1_int * pow(2.0,-43);
                                S8  af2_int  =  (word9>>22)&0xff;
                                double af2   =  (double)af2_int * pow(2.0,-55);

                                // save decoded data
                                decode_data[prn-1].gpstime.gpsWeek    = week + 1024;
                                decode_data[prn-1].gpstime.gpsSeconds = tow;
                                decode_data[prn-1].info.URA = uraIndex;
                                decode_data[prn-1].info.health = svHealth;
                                decode_data[prn-1].clkCorr.tgd = tgd;
                                decode_data[prn-1].clkCorr.af0 = af0;
                                decode_data[prn-1].clkCorr.af1 = af1;
                                decode_data[prn-1].clkCorr.af2 = af2;
                                decode_data[prn-1].clkCorr.toc = toc;
                                decode_data[prn-1].clkCorr.IODC = iodc;
                                decode_data[prn-1].ready |= 1;
                                assert(subframe == ch->subframeIdx+1);
                            } else if (ch->subframeIdx == 1) {      // subframe 2
                                U8 subframe  =  HOW>>8 & 0x7;
                                U32 tow  = (HOW>>13 & 0x1ffff)*6;
                                printf("subframe is %d, tow is %d\n", subframe, tow);
                                U8  IODE = word3>>22 & 0xff;                                     
                                S16 Crs_i = (word3 & 0x3fffc0)>>6;
                                double Crs = Crs_i * (1/32.0);
                                S16 deltan = (word4&0x3fffc000)>>14;
                                double delta_n = deltan * pow(2.0, -43);
                                S32 M0_i = (word4<<18&0xff000000) | (word5>>6&0x00ffffff);
                                double M0 = M0_i * pow(2.0, -31);
                                S16 Cuc_i = (word6&0x3fffc000)>>14;
                                double Cuc = Cuc_i * pow(2.0, -29);
                                U32 e_i = (word6<<18&0xff000000) | (word7>>6&0x00ffffff);
                                double e = e_i * pow(2, -33);
                                S16 Cus_i = (word8&0x3fffc000)>>14;
                                double Cus = Cus_i * pow(2, -29);
                                U32 sqrtA_i = (word8<<18&0xff000000) | (word9>>6&0x00ffffff);
                                double a_sqrt = sqrtA_i * pow(2.0, -19);
                                U32 toe = ((word10&0x3fffc000)>>14)*16;
                                // save decoded data
                                decode_data[prn-1].eph.IODE = IODE;
                                decode_data[prn-1].eph.Crs = Crs;
                                decode_data[prn-1].eph.delta_n = delta_n;
                                decode_data[prn-1].eph.M0 = M0;
                                decode_data[prn-1].eph.Cuc = Cuc;
                                decode_data[prn-1].eph.Cus = Cus;
                                decode_data[prn-1].eph.e = e;
                                decode_data[prn-1].eph.sqrt_a = a_sqrt;
                                decode_data[prn-1].eph.toe = toe;
                                decode_data[prn-1].ready |= (1<<1);
                                assert(subframe == ch->subframeIdx+1);
                            } else if (ch->subframeIdx == 2) {      // subframe 3
                                U8 subframe  =  HOW>>8 & 0x7;
                                U32 tow  = (HOW>>13 & 0x1ffff)*6;
                                printf("subframe is %d, tow is %d\n", subframe, tow);
                                S16 Cic_i = (word3&0x3fffc000)>>14;
                                double Cic = Cic_i* pow(2.0, -29);
                                S32 Omega0_i = (word3<<18 & 0xff000000) | (word4>>6 & 0x00ffffff);
                                double Omega0 = Omega0_i * pow(2.0,-31);
                                S16 Cis_i = (word5&0x3fffc000)>>14;
                                double Cis = Cis_i * pow(2.0, -29);
                                S32 i0_i = (word5<<18&0xff000000) | (word6>>6&0x00ffffff);
                                double i0 = i0_i * pow(2.0, -31);
                                S16 Crc_i = (word7&0x3fffc000)>>14;
                                double Crc = Crc_i * pow(2.0,-5);
                                S32 omega_i = (word7<<18&0xff000000) | (word8>>6&0x00ffffff);
                                double omega = omega_i * pow(2.0,-31);
                                S32 Omega_dot_i = ((S32)(word9<<2))>>8;
                                double Omega_dot = Omega_dot_i * pow(2.0,-43);
                                U8 IODE = (word10>>22)&0xff;
                                S16 idot_i = ((S32)(word10<<10))>>18;
                                double idot = idot_i*pow(2.0, -43);
                                // save decoded data
                                decode_data[prn-1].eph.Cic = Cic;
                                decode_data[prn-1].eph.Omega0 = Omega0;
                                decode_data[prn-1].eph.Cis = Cis;
                                decode_data[prn-1].eph.i0 = i0;
                                decode_data[prn-1].eph.Crc = Crc;
                                decode_data[prn-1].eph.omega = omega;
                                decode_data[prn-1].eph.Omega_dot = Omega_dot;
                                decode_data[prn-1].eph.i_dot = idot;
                                decode_data[prn-1].ready |= (1<<2);
                                assert(subframe == ch->subframeIdx+1);
                            } else if (ch->subframeIdx == 3) {
                                U8 subframe  =  HOW>>8 & 0x7;
                                U32 tow  = (HOW>>13 & 0x1ffff)*6;
                                printf("subframe is %d, tow is %d\n", subframe, tow);
                            } else if (ch->subframeIdx == 4) {
                                U8 subframe  =  HOW>>8 & 0x7;
                                U32 tow  = (HOW>>13 & 0x1ffff)*6;
                                printf("subframe is %d, tow is %d\n", subframe, tow);
                            } else {
                                printf("subframe index is not right\n");
                            }
                            // check if we have collected suframes 1-3
                            if ((decode_data[prn-1].ready&0x7) == 0x7) {
                                printf("we have colleced enough data for position fix\n");
                                print_decodedMsg(&decode_data[prn-1]);

                            }
                        }

                        // update bitIdx, wordIdx, and subframeIdx
                        ch->bitIdx++;
                        if(ch->bitIdx == 30)    
                        {
                            ch->bitIdx = 0;
                            ch->wordIdx++;
                            if (ch->wordIdx == 10) 
                            {
                                ch->wordIdx = 0;
                                ch->subframeIdx++;
                                if (ch->subframeIdx == 5)
                                    ch->subframeIdx = 0;
                            }
                        }
                    }
                }//if (ch->mod20cnt == 19)
            } // if (ch->state >= 2)
			// save 1ms correlation values after bit sync
            if (ch->state >= 2) 
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
            ch.bitIdx   = 0;
            ch.wordIdx  = 0;
            ch.subframeIdx = 0;
            for (int k = 0; k < 20; k++) {
                ch.BitFlipCnt[k] = ch.Bits[k] = 0;
                ch.wordBuffer[k] = 0;
            }
            ch.polarKnown = 0;
            ch.polarPositive = 0;
            for (int i = 0; i < 5; i++)
                for (int j = 0; j < 10; j++)
                    ch.frameData[i][j] = 0;
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
//      printf("tm: 20ms interrupt executed\n");

    }
  }
}
