/*
 * ShiftRegister.h
 *
 *  Created on: Aug 12, 2016
 *      Author: dma
 */

#ifndef SHIFTREGISTER_H_
#define SHIFTREGISTER_H_

// define shift register taps
typedef struct ShiftRegisterTapsS{
	int* reg;
	int taps[3];
	int halfChipSamples;
	int len;
} ShiftRegisterTaps;

// 1. initialize shift register taps
void initShiftRegisterTaps(ShiftRegisterTaps *shiftRegister, int halfChipSamples);

// 2. shift in a new sample
void shiftIn(ShiftRegisterTaps *shiftRegister, int val);

#endif /* SHIFTREGISTER_H_ */
