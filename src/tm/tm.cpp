/* =============================================================================
 * function name : tm.c
 *        author : Qiang Chen
 *          date : Dec. 9, 2015
 *
 *   description : this file defines the function of tracking manager (tm)
 * =============================================================================*/
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
extern FILE *pfRinex;

// 32 cosine/sine lookup table
const int costable[] = { 252, 247, 233, 210, 178, 140, 96, 49, 0, -49, -96, -140, -178, -210, -233, -247, -252, -247, -233, -210, -178, -140, -96, -49, 0, 49, 96, 140, 178, 210, 233, 247 };
const int sintable[] = { 0, 49, 96, 140, 178, 210, 233, 247, 252, 247, 233, 210, 178, 140, 96, 49, 0, -49, -96, -140, -178, -210, -233, -247, -252, -247, -233, -210, -178, -140, -96, -49 };

// TODO: move these to Channel 
int prncodetable[GPS_SAT_NUM][1023] = {0};
filter_parameters pll_filter_parameters,dll_filter_parameters;
ThirdOrderFilterParameters thirdOrderPhaseFilter;
SecondOrderFilterParameters secondOrderFreqFilter;
SecondOrderFilterParameters secondOrderCodeFilter;
// tm global variables
track_array_list track_info_array_list;

// log files
int oneTimeFlag = 0;

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
}

// 3. second order carrier tracking loop
void carrierTrackingSDR(Channel *ch)
{
	double carrError;
	carrError = atan((double)ch->measuresIQ[1][1] / (double)ch->measuresIQ[1][0]) / (2.0 * pi);
	ch->carrNco = ch->oldCarrNco + (pll_filter_parameters.tau2 / pll_filter_parameters.tau1)
			* (carrError - ch->oldCarrError) + carrError * 0.001 / pll_filter_parameters.tau1;
	ch->oldCarrNco = ch->carrNco;
	ch->oldCarrError = carrError;
	ch->carrierFreq = ch->carrierFreqBasis + ch->carrNco;
	ch->carrierPhaseStep = (unsigned int)(pow(2,32) * ch->carrierFreq / SAMPLING_FREQUENCY);
}

// 4. second order code tracking loop
void codeTrackingSDR(Channel *ch)
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
}

//  not used?
void carrierTracking(Channel *ch)
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
	}
}

// not used ?
void codeTracking(Channel *ch)
{
	double early,late, earlyLateError, codeFreqError;
	early = sqrt(ch->measuresIQ[0][0] * ch->measuresIQ[0][0] + ch->measuresIQ[0][1] * ch->measuresIQ[0][1]);
	late = sqrt(ch->measuresIQ[2][0] * ch->measuresIQ[2][0] + ch->measuresIQ[2][1] * ch->measuresIQ[2][1]);
	earlyLateError = -(early - late) / (early + late);
	ch->codeLoopError2nd = ch->codeLoopError2nd + 0.001 * secondOrderCodeFilter.C2 * earlyLateError;
	codeFreqError = ch->codeLoopError2nd + secondOrderCodeFilter.C1 * earlyLateError;
	ch->codeFreq += codeFreqError/5;
	ch->codePhaseStep = (unsigned int)(pow(2,32) * ch->codeFreq * ch->halfChipSamples / SAMPLING_FREQUENCY);
}

// 6. correlation for 20 ms data
void correlate(Channel *ch, char *inbuffer)
{
	int i,j;
	int cossin[2] = {0};
	int currentCode;
	int complexData[2];

    // sample by sample processing
	for(i = 0; i < SAMPS_PER_MSEC*40; i=i+2)  {
		// carrier nco tick
		ch->carrierPhase += ch->carrierPhaseStep;
		if(((ch->carrierPhase&0x80000000) == 0) && ((ch->carrierPhaseBack&0x80000000) != 0)) {
			ch->carrierPhaseCycleCount++;
		}
		ch->carrierPhaseBack = ch->carrierPhase;
		ch->carrierPhaseIndex = ch->carrierPhase >> 27; // 32 loop-up table
		cossin[0] =  costable[ch->carrierPhaseIndex];
		cossin[1] = -sintable[ch->carrierPhaseIndex];
        // carrier removal, get baseband signal
        complexData[0] = (inbuffer[i] * cossin[0] - inbuffer[i + 1] * cossin[1]) >> 3;
        complexData[1] = (inbuffer[i] * cossin[1] + inbuffer[i + 1] * cossin[0]) >> 3;

		// code nco tick
		ch->codeCycle = 0;
		currentCode = prncodetable[ch->prn-1][ch->codePhaseIndex];

		ch->codePhase += ch->codePhaseStep;
		ch->codeClockMultiX = (((ch->codePhase & 0x80000000) ^ ch->codePhaseBack) != 0) ? 1 : 0;
		if(ch->codeClockMultiX ==1) {
            // shift in half chip sample
			shiftIn(&(ch->taps), currentCode);
			ch->divideCount++;
            // one chip boundary
			if(ch->divideCount >= ch->fullChipSamples) {
				ch->divideCount=0;
				ch->codePhaseIndex++;
                // one code period 
				if(ch->codePhaseIndex >= ch->codePeriodLength) {
					ch->codePhaseIndex = 0;
					ch->codeCycle = 1;
				}
			}
		}
		ch->codePhaseBack = ch->codePhase & 0x80000000;
		
		//correlate with E, P, L local codes
		for(j = 0; j < 3; j++) {
			ch->correlates[j][0] += ch->taps.taps[j] * complexData[0];  // real
			ch->correlates[j][1] += ch->taps.taps[j] * complexData[1];  // imag
		}

		// 1ms boundary
		if(ch->codeCycle == 1)  {
            // save prompt I/Q
			ch->prePromptIQ[0] = ch->measuresIQ[1][0];
			ch->prePromptIQ[1] = ch->measuresIQ[1][1];
            // update 1ms cnt
            ch->oneMsCnt++;
            ch->mod20cnt = (ch->mod20cnt+1)%20;
            int mod20cnt = ch->mod20cnt;
            ch->Bits[mod20cnt] = (ch->correlates[1][0]>0)? 1:-1;
            if (ch->oneMsCnt>1000 && ch->state==1 )    // > 1s, assume stable tracking loop
            {
                int preMod20cnt = (ch->mod20cnt == 0)? 19: ch->mod20cnt-1;
                if (ch->Bits[mod20cnt] != ch->Bits[preMod20cnt])  {
                    ch->BitFlipCnt[mod20cnt]++;
                    if (ch->BitFlipCnt[mod20cnt] > 5) {
                        for (int k = 0; k < 20; k++)
                            if( ch->BitFlipCnt[k] > 1 && k != mod20cnt)
                                exit(1);
                        ch->state = 2;  // in bit sync mode
                        ch->mod20cnt  = 0;
                    }
                }
            }
            // save for measurement
			for (j = 0; j < 3; j++) {
				ch->measuresIQ[j][0] = ch->correlates[j][0]>>5;
				ch->measuresIQ[j][1] = ch->correlates[j][1]>>5;
				ch->correlates[j][0] = 0;
				ch->correlates[j][1] = 0;
			}
            // bit synced or frame sync
            if (ch->state >= 2)  {
                if (ch->mod20cnt == 0)
                    ch->P_i_20ms =  ch->measuresIQ[1][0];
                else
                    ch->P_i_20ms += ch->measuresIQ[1][0];
                // new bit collected
                if (ch->mod20cnt == 19)
                {
                    int currBit = (ch->P_i_20ms>0)? 1:0;
                    // save currBit
                    for (int k = 0; k < 19; k++)
                    {
                        ch->wordBuffer[k] <<= 1; 
                        ch->wordBuffer[k] |= ((ch->wordBuffer[k+1]&0x20000000)>>29);
                    }
                    ch->wordBuffer[19] <<= 1;
                    ch->wordBuffer[19] |= currBit;
                    
                    if (ch->state == 2) 
                    {
                        // check if have preamble or inv-preamble
                        if ((ch->wordBuffer[0]&0x3fc00000)>>22==0x8b || (ch->wordBuffer[0]&0x3fc00000)>>22==0x74)
                        {
                            if ((ch->wordBuffer[10]&0x3fc00000)>>22==0x8b ||(ch->wordBuffer[10]&0x3fc00000)>>22==0x74)
                            {
                                // decode TOW for frame sync check
                                U32 TLW1 = ch->wordBuffer[0], HOW1 = ch->wordBuffer[1];
                                U32 TLW2 = ch->wordBuffer[10], HOW2 = ch->wordBuffer[11];
                                U32 mod6_tow_now  = ((TLW1&0x1)==0)? (HOW1&0x3fffe000)>>13: ((HOW1&0x3fffe000)^(0x3fffe000)) >>13;
                                U32 mod6_tow_next = ((TLW2&0x1)==0)? (HOW2&0x3fffe000)>>13: ((HOW2&0x3fffe000)^(0x3fffe000)) >>13;
                                if (mod6_tow_next-mod6_tow_now == 1 || (mod6_tow_next == 0 && mod6_tow_now == 100799))
                                {
                                    // frame sync succeed
                                    ch->state = 3;
                                    // get subframe number
                                    U8 subframe = ((TLW1&0x1)==0)? (HOW1>>8&0x7) : ((HOW1>>8&0x7)^(0x7));
                                    printf("prn %02d, subframe is %d\n", ch->prn, subframe);

                                    // now parity-check, get the raw bits, and save current two subframes to buffered data
                                    U8 check_result;
                                    for (int k=0; k<20; k++)
                                    {
                                        U8 temp = (k<10)? (subframe-1):(subframe%5);
                                        if (k==0 || k==10)
                                            check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[temp][0]);
                                        else {
                                            U32 tempWord = ch->wordBuffer[k-1] << 30;
                                            ch->wordBuffer[k] &= 0x3fffffff;
                                            ch->wordBuffer[k] |= tempWord;
                                            check_result = parity_check(&ch->wordBuffer[k], &ch->frameData[temp][k%10]);
                                        }
                                        if (check_result != 0) 
                                            printf("prn %02d, word = %d, parity check fails\n", ch->prn, k);                                        
                                    }
                                    // set subframe index and word index
                                    ch->bitIdx = 0;
                                    ch->wordIdx = 0;
                                    ch->subframeIdx = (subframe+1)%5; // subframeIdx from 0 to 4
                                    ch->frameEdgeTow = mod6_tow_next * 6;
                                }
                            }
                        }
                    }  /* if (ch->state == 2)  */
                    else // frame synced!
                    {
                        // we have a complete subframe
                        if (ch->wordIdx == 9 && ch->bitIdx == 29) 
                        {
                            U8 check_result;
                            // parity check, convert to raw bits for last 10 words
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
                            U8 prn = ch->prn;
                            /* decode data */
                            if (ch->subframeIdx == 0) {             // subframe 1
                                decodeSubframe1(ch->frameData[ch->subframeIdx], &decode_data[prn-1]);
                                // printf("subframe1\n");
                            } else if (ch->subframeIdx == 1) {      // subframe 2
                                decodeSubframe2(ch->frameData[ch->subframeIdx],&decode_data[prn-1]);
                                // printf("subframe2\n");
                            } else if (ch->subframeIdx == 2) {      // subframe 3
                                decodeSubframe3(ch->frameData[ch->subframeIdx],&decode_data[prn-1]);
                                // printf("subframe3\n");
                            } else if (ch->subframeIdx == 3) {      // subframe 4                          
                                // printf("subframe4\n");
                            } else if (ch->subframeIdx == 4) {      // subframe 5
                                // printf("subframe5\n");
                            } else {
                                printf("subframe index is not right\n");
                            }
                            // check if we have collected suframes 1-3
                            if ((decode_data[prn-1].ready&0x7) == 0x7) {
                                // printf("we have colleced enough data for position fix\n");
                                print_decodedMsg(ch->pfEph, &decode_data[prn-1]);

                            }
                        }

                        // update bitIdx, wordIdx, and subframeIdx
                        ch->bitIdx++;
                        if(ch->bitIdx == 30) {
                            ch->bitIdx = 0;
                            ch->wordIdx++;
                            if (ch->wordIdx == 10) {
                                ch->wordIdx = 0;
                                ch->subframeIdx++;                                                     
                                if (ch->subframeIdx == 5)
                                    ch->subframeIdx = 0;
                                ch->frameEdgeTow += 6;
                                if (ch->frameEdgeTow == 100800) {
                                    ch->frameEdgeTow = 0;
                                }
                            }
                        } // if(ch->bitIdx == 30)
                    } // if (ch->state == 2) 
                } // if (ch->mod20cnt == 19)
            } // if (ch->state >= 2)
			// save 1ms correlation values after bit sync
            if (ch->state >= -10) {                
                fprintf(ch->pfTracking, "%d, %d, %d, %d, %d, %d\n", 
                                        ch->measuresIQ[0][0],ch->measuresIQ[0][1],
                                        ch->measuresIQ[1][0],ch->measuresIQ[1][1],
                                        ch->measuresIQ[2][0],ch->measuresIQ[2][1]);
                fflush(ch->pfTracking);			   
            }

            // loop update at 1ms boundary
			carrierTrackingSDR(ch);
			codeTrackingSDR(ch);
		} // if(ch->codeCycle == 1) 
	} // for(i = 0; i < SAMPS_PER_MSEC*40; i=i+2)     

    // get measurement time if frame sync is successful
    if (ch->state >= 3)
        ch->tx_time = ch->frameEdgeTow + ch->wordIdx*0.6 + ch->bitIdx*0.02 + ch->mod20cnt*1e-3
                    + ch->codePhaseIndex / ch->codeFreq + ch->codePhase / P2TO32 / ch->codeFreq;
}  // correlate 


// tm thread function
void tm_proc(void)
{
    static bool first_line = true;
    printf( "tm starts to run! task_id = %d\n", kiwi_get_taskID() );    
    init_track();
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

        if (msg_tag == 1)
        {
            acq_search_results temp;
            Channel ch;
            while (!isAcqResultQueueEmpty(pAcqResQueue)) {
                popAcqResultQueue(pAcqResQueue, &temp);
                init_channel(&ch, temp);
                add_track_info_to_array_list(&track_info_array_list, &ch);
            } /* while (!isAcqResultQueueEmpty(pAcqResQueue)) */

            int track_num = track_info_array_list_size(&track_info_array_list);

            // do correlate for each PRN
            for (int i = 0; i < track_num; i++) {
                correlate(get_track_info(&track_info_array_list, i), (char *)inbuffer);
            }
            // update rx timer 
            rxClock.superCount += 20;
#if 1
            if (first_line && track_num == 6) {
                fprintf(pfRinex, "# prn list: ");
                for (int i = 0; i < track_num; i++) {
                    Channel *ptemp = get_track_info(&track_info_array_list, i);
                    fprintf(pfRinex, "%02d ", ptemp->prn);
                }
                first_line = false;
            }
            // get measurements
            fprintf(pfRinex, "%10llu ", rxClock.superCount);
            for (int i = 0; i < track_num; i++) {
                Channel *pch = get_track_info(&track_info_array_list, i);
                if (pch->state >= 3) {                                        
                    fprintf(pfRinex, "%.9f ", pch->tx_time);                    
                }
            }
            fprintf(pfRinex, "\n");
#endif
        }   // if (msg_tag == 1) 
    } /* while(1) */
}
