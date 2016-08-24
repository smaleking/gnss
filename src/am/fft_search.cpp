/* this function is to performa fft search for a given PRN 
 * the acq engine searches the carrier frequency serialy but search the code phase in parallel
 * the coherent integration time is 1ms, non-coherent time is 10
 *
 * the sampling frequency is 4.0e6
 *
 * Input  - PRN of satellite (int)a
 *        - Raw data buffer address, at least 12 ms
 *  
 * Output - none
 * 
 * Author : Qiang Chen
 * Date   : 11/23/2015
 */

#include "fft_search.h"
#include "../utils/sampleCAcode.h"
#include "am.h"
#include <math.h>
// sample number of 1ms
#define ACQ_SAMPLE_1MS (4000)
#define BLK_SIZE (40000)
int phase_table[8] = {1,2,2,1,-1,-2,-2,-1};

void fft_search(int prn, int16_t *raw_data)
{
  int i,j,k;
  int phase,coslut,sinlut;
  double pow2_32 =  4294967296.0;
  uint32_t carr_nco_add, carr_nco_acc = 0;
  double carr_search_range, carr_search_step;
  int    carr_search_num;
  carr_search_range = 2900.0; // -3k to +3k
  carr_search_step  = 500.0;
  carr_search_num   = carr_search_range/500 + 1; // single band search number
  double  center_freq = 20.0e3, current_freq;
  
  double fs = 4.0e6;  // sampling frequency
  int Nsamps  = (int)(4.0e6*1e-3); // one ms sampling number  
  int ca_code[1023];  
  int sampledCAcode[ACQ_SAMPLE_1MS]; // 1ms samples of local code

  kiss_fft_cpx code_fft_in[ACQ_SAMPLE_1MS], code_fft_out[ACQ_SAMPLE_1MS];  // input/output of code fft
  kiss_fft_cpx bb_fft_in[BLK_SIZE], bb_fft_out[BLK_SIZE];      // input/output of baseband fft
  kiss_fft_cpx ifft_in[ACQ_SAMPLE_1MS], ifft_out[ACQ_SAMPLE_1MS];          // input/output of ifft
  double power[ACQ_SAMPLE_1MS], accu_power[ACQ_SAMPLE_1MS], opt_power[ACQ_SAMPLE_1MS];
  double maxPower_singleFreq = 0, maxPower_all = 0;
  double optFreq = 0.0;
  int    optCodePhase = 0, optCodePhase_singleFreq;

  /* part 1. do CA code and sample CA code  */
  cacode(prn, ca_code);   // raw CA code
  sampleCAcode(ca_code, fs, Nsamps, sampledCAcode);

  /* part 2. do FFt to local CA code */
  kiss_fft_cfg cfg1 = kiss_fft_alloc( Nsamps, 0, NULL, NULL );   // forward fft
  kiss_fft_cfg cfg2 = kiss_fft_alloc( Nsamps, 1, NULL, NULL );   // inverse fft

  for ( i = 0; i < ACQ_SAMPLE_1MS; i++ )
  {
    code_fft_in[i].r = (float)(sampledCAcode[i]); // float
    code_fft_in[i].i = 0.0;                         // float
  }
  kiss_fft(cfg1, code_fft_in, code_fft_out);
#if (qchen_debugi && 1)
  FILE *tempFile;
  tempFile = fopen("code_fft_out.bin","wb+");
  fwrite(code_fft_out, sizeof(kiss_fft_cpx), 4000, tempFile);
  fclose(tempFile);
  tempFile = fopen("code_fft_in.bin","wb+");
  fwrite(code_fft_in, sizeof(kiss_fft_cpx), 4000, tempFile);
  fclose(tempFile);
#endif
  for ( i = 0; i < ACQ_SAMPLE_1MS; i++ )
    code_fft_out[i].i = -code_fft_out[i].i; // take the conjugate for local code fft

  /* part 3. remove carrier and then do FFT */
  for( i = 0; i < 2*carr_search_num + 1; i++ )
  {
    // get current carrier frequency
    current_freq = center_freq + ( i - carr_search_num ) * carr_search_step;
    // convert current_freq to nco_add, totally 10 ms data
    carr_nco_acc = 0;
    carr_nco_add = (uint32_t)(current_freq/4.0e6*pow2_32+0.5);
    for ( j = 0; j < BLK_SIZE; j++ )
    {
      phase   = (carr_nco_acc >> 29);
      sinlut  = phase_table[phase];
      coslut  = phase_table[(phase+2)&0x7];
      bb_fft_in[j].r = (float)(raw_data[j] * coslut);
      bb_fft_in[j].i = (float)(raw_data[j] * sinlut);
      // update carrier nco accumulation
      carr_nco_acc += carr_nco_add;
    }
    // do fft: 10 x 1 ms
    // multiply and then do ifft
    for ( k = 0; k < Nsamps; k++)
    {
      accu_power[k] = 0;
      power[k] = 0;
    }
    for ( j = 0; j < 10; j++ )
    {
      kiss_fft(cfg1, bb_fft_in+j*Nsamps, bb_fft_out+j*Nsamps);
      for ( k = 0; k < Nsamps; k++ )
      {
        ifft_in[k].r = code_fft_out[k].r * (bb_fft_out+j*Nsamps)[k].r - code_fft_out[k].i * (bb_fft_out+j*Nsamps)[k].i;
        ifft_in[k].i = code_fft_out[k].r * (bb_fft_out+j*Nsamps)[k].i + code_fft_out[k].i * (bb_fft_out+j*Nsamps)[k].r;
      }
      // do ifft and then calculate the power
      kiss_fft(cfg2, ifft_in, ifft_out);
      for ( k = 0; k < Nsamps; k++)
      {
        power[k] = sqrt((double)(ifft_out[k].r) * ifft_out[k].r + (double)(ifft_out[k].i) * ifft_out[k].i);
        accu_power[k] += power[k];
      }
    }
    // search the maximum power value for all code phases
    maxPower_singleFreq = 0;
    for (k = 0; k < Nsamps; k++)
    {
      if (accu_power[k] > maxPower_singleFreq)
      {
        maxPower_singleFreq = accu_power[k];
        optCodePhase_singleFreq = k;      
      }
    }
    // then compare current maxPower with maxPower_all
    if ( maxPower_singleFreq > maxPower_all )
    {
      maxPower_all = maxPower_singleFreq;
      // save the optimal frequency and code phase
      optCodePhase = optCodePhase_singleFreq;
      optFreq =  current_freq;
      // save the correlation values for this particular frequency
      for(k = 0; k < Nsamps; k++)
        opt_power[k] = accu_power[k];
    }
  } // all frequency has been searched
  free(cfg1);
  free(cfg2);


  // lets calculate the ratio of maxPeak to 2nd maxPeak;
  int winSize = (int)(fs/1023000)+1;
  int leftBound = optCodePhase - winSize;
  int rightBound = optCodePhase + winSize;
  double secMaxPeak = 0.0;
  double peakRatio = 1.0;
  if (leftBound < 0 )
  {
    for( k = rightBound; k < leftBound + Nsamps; k++ )
      if ( opt_power[k] > secMaxPeak )
        secMaxPeak = opt_power[k];

  }
  else if ( rightBound > Nsamps - 1  )
  {
    for( k = rightBound - Nsamps; k < leftBound; k++)
      if ( opt_power[k] > secMaxPeak )
        secMaxPeak = opt_power[k];
  }
  else
  {
    for( k = 0; k < Nsamps; k++ )
    {
      if (  k < leftBound || k > rightBound )
      {
        if ( opt_power[k] > secMaxPeak)
            secMaxPeak = opt_power[k];
      }
    }
  }
  peakRatio = maxPower_all / secMaxPeak;
  if (peakRatio > 2.0)
  {
    printf("prn %02d, ratio is %.1f,freq is %f, code phase is %d\n", prn, peakRatio, optFreq, optCodePhase);
    // push this acquisition result into queue
    acq_search_results temp;
    temp.prn = prn;
    temp.code_phase = optCodePhase;
    temp.carrier_freq = optFreq;
    pushAcqResultQueue(pAcqResQueue, temp);
  }
}

