/*
 * tracklist.c
 *
 *  Created on: Aug 10, 2016
 *      Author: Kunlun Yan
 */
#include "tracklist.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// 0. initialize a single channel using acquisition result
void init_channel(Channel *pch, acq_search_results acqResult)
{
    pch->prn = acqResult.prn;
    /* hard coded carrier frequency. 
    TODO: improve anquisition accuracy
    */
    if (acqResult.prn == 2)
        pch->carrierFreq = 17738.0;
    else if (acqResult.prn == 5)
        pch->carrierFreq = 18059.0;    
    else if (acqResult.prn == 13)
        pch->carrierFreq = 22629.0;
    else if (acqResult.prn == 20)
        pch->carrierFreq = 21576.0;
    else if (acqResult.prn == 25)
        pch->carrierFreq = 17204.0;
    else if (acqResult.prn == 29)
        pch->carrierFreq = 21126.0;
    else
        pch->carrierFreq = acqResult.carrier_freq;   

    pch->carrierFreqBasis = pch->carrierFreq;
    pch->codeFreqBasis = 1023000.0;
    pch->oldCarrNco = 0;
    pch->oldCarrError = 0;
    pch->carrierPhase = 0;
    pch->carrierPhaseBack = 0;
    pch->carrierPhaseIndex = 0;
    pch->carrierPhaseCycleCount = 0;
    pch->carrierPhaseStep = (unsigned int)(pow(2, 32) * pch->carrierFreq / SAMPLING_FREQUENCY);

    pch->oldCodeNco = 0;
    pch->oldCodeError = 0;
    pch->codeFreq = 1023000.0;
    pch->codePhase = 0;
    pch->codePhaseBack = 0;
    pch->codePhaseIndex = (4000 - acqResult.code_phase) * 1023 / 4000 + 1;
    pch->halfChipSamples = 1;
    pch->fullChipSamples = 2;
    pch->codePeriodLength = 1023;
    pch->codePhaseStep = (unsigned int)(pow(2, 32) * 1023000 * pch->halfChipSamples / SAMPLING_FREQUENCY);
    initShiftRegisterTaps(&(pch->taps), 1);
    for (int k = 0; k < 3; k++) {
        pch->correlates[k][0] = pch->correlates[k][1] = 0;
    }
    pch->carrierLoopError3rd = 0;
    pch->carrierLoopError2nd = 0;
    pch->codeLoopError2nd = 0;
    pch->divideCount = 0;
    pch->P_i_20ms = 0;
    pch->P_q_20ms = 0;
    pch->oneMsCnt = 0;
    pch->mod20cnt = 0;
    pch->state = 1;
    pch->bitIdx = 0;
    pch->wordIdx = 0;
    pch->subframeIdx = 0;
    for (int k = 0; k < 20; k++) {
        pch->BitFlipCnt[k] = pch->Bits[k] = 0;
        pch->wordBuffer[k] = 0;
    }
    pch->polarKnown = 0;
    pch->polarPositive = 0;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 10; j++)
            pch->frameData[i][j] = 0;

    // log tracking variables    
    char fname[128];
    sprintf(fname, "gps_prn%02d_tracking.log", acqResult.prn);
    pch->pfTracking = fopen(fname, "wt");
    if (pch->pfTracking == NULL) {
        exit(101);
    }
    sprintf(fname, "gps_prn%02d_ephemeris.log", acqResult.prn);
    pch->pfEph = fopen(fname, "wt");
    if (pch->pfEph == NULL) {
        exit(101);
    }

}

// 1. initialize a track_array_list (channels)
void init_track_array_list(track_array_list* track_info_array_list)
{
    track_info_array_list->capacity = 0;
}

// 2. check if a satellite is in the track_array_list
int check_satellite_in_track_array_list(track_array_list *track_info_array_list, int prn)
{
    for (int i = 0; i<track_info_array_list->capacity; i++) {
        if(track_info_array_list->channels[i].prn == prn)
            return 1;       
    }
    return 0;
}

// 3. add a channel to track_array_list
void add_track_info_to_array_list(track_array_list *track_info_array_list, Channel *one_satellite_info)
{
    track_info_array_list->channels[track_info_array_list->capacity] = *one_satellite_info;
    track_info_array_list->capacity++;
}

// 4. remove a prn from track_array_list
void remove_track_info_from_array_list(track_array_list * track_info_array_list, int prn)
{
    int i;
    for (i = 0; i<track_info_array_list->capacity; i++) {
        if (track_info_array_list->channels[i].prn == prn)
            break;        
    }
    for(; i<track_info_array_list->capacity-1; i++) {
        track_info_array_list->channels[i] = track_info_array_list->channels[i+1];
    }
    track_info_array_list->capacity--;
}

// 5. get track_array_list size
int track_info_array_list_size(track_array_list *track_info_array_list)
{
    return track_info_array_list->capacity;
}

// 6. get the channel info given a channel index
Channel *get_track_info(track_array_list *track_info_array_list, int index)
{
    return &(track_info_array_list->channels[index]);
}


