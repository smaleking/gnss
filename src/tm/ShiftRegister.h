/*
 * ShiftRegister.h
 *
 *  Created on: Aug 12, 2016
 *      Author: Kunlun Yan
 */

#ifndef SHIFTREGISTER_H_
#define SHIFTREGISTER_H_

#define TM_TAP_NUM (3)
#define EARLY_TAP_POS (0)
#define PROMPT_TAP_POS (1)
#define LATE_TAP_POS (2)

// define shift register taps
typedef struct ShiftRegisterTapsS{
	int* reg;
	int taps[TM_TAP_NUM];
	int halfChipSamples;
	int len;
} ShiftRegisterTaps;

// 1. initialize shift register taps
void initShiftRegisterTaps(ShiftRegisterTaps *shiftRegister, int halfChipSamples);

// 2. shift in a new sample
void shiftIn(ShiftRegisterTaps *shiftRegister, int val);

#endif /* SHIFTREGISTER_H_ */
