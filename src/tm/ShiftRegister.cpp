/*
 * ShiftRegister.c
 *
 *  Created on: Aug 12, 2016
 *      Author: Kunlun Yan
 */

#include <stdlib.h>
#include "ShiftRegister.h"

// 1. initialize shift register taps
void initShiftRegisterTaps(ShiftRegisterTaps *shiftRegister, int halfChipSamples)
{
	shiftRegister->halfChipSamples = halfChipSamples;
	shiftRegister->len = halfChipSamples * 5 + 1;
	shiftRegister->reg = (int*)malloc(shiftRegister->len * sizeof(int));
	int i;
	for(i = 0; i < shiftRegister->len; i++)
	{
		shiftRegister->reg[i] = -1;
	}
	for(i = 0; i < 3; i++)
	{
		shiftRegister->taps[i] = 0;
	}
}

// 2. read in one data sample
void shiftIn(ShiftRegisterTaps *shiftRegister, int val)
{
	int i;
    // right shift, half chip spaced
	for(i = shiftRegister->len - 1; i > 0; i--) {
		shiftRegister->reg[i] = shiftRegister->reg[i-1];
	}
	shiftRegister->reg[0] = val;
    
	shiftRegister->taps[0] = shiftRegister->reg[0];                                     // Early    
	shiftRegister->taps[1] = shiftRegister->reg[shiftRegister->halfChipSamples];        // Prompt 
	shiftRegister->taps[2] = shiftRegister->reg[shiftRegister->halfChipSamples << 1];   // Late
}
