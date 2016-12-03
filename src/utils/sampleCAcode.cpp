/* =================================================================================
 * This function is to sample the given GPS CA code of a particular satellite.
 * 
 *    Input - sampling frequency in Hz (double)
 *            sample number (int)
 *            pointer of memory for CA code
 *          
 *    In/Ou - sampled CA code
 *
 *  This function cannot adjust the initial code phase. 
 *  It alwasy perform the sampling from the initial code phase, which is 0.
 *
 * =================================================================================*/

#include "sampleCAcode.h"

void sampleCAcode(int *cacode, double fs, int totalSamples, int *sampledCAcode)
{
  int i;
  double Tc;
  Tc = 1.0/1023*1.0e-3;  // chip width in sec of CA code
  int chipIdx;
  for ( i = 0; i < totalSamples; i++)
  {
    chipIdx = ((int)(i/fs/Tc))%1023;
    assert(chipIdx>=0||chipIdx<=1022);
    sampledCAcode[i] = cacode[chipIdx];
  }
}
