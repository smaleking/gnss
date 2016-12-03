/* ===============================================================================
*
*     filename :  rx_config.h
*       author :  Qiang Chen
*         date :  12/15/2015
*
*  description :  this header file defines the receiver related paramters
*             
*=================================================================================*/
#ifndef KIWI_CONFIG_H
#define KIWI_CONFIG_H

// GNSS parameters
#define GPS_SAT_NUM (32)

// sampling frequency
#define SAMPLING_FREQUENCY (4000000)

// intermediate frequency
#define INTERMEDIATE_FREQUENCY (-20000.0e3)

// # of samples per msec
#define SAMPS_PER_MSEC (SAMPLING_FREQUENCY/1000)

//loop parameters
#define	DLL_Damping_Ratio (0.7)
#define DLL_Noise_Bandwidth (2)
#define	PLL_Damping_Ratio (0.7)
#define	PLL_Noise_Bandwidth (15)

#endif /* rx_config.h */


