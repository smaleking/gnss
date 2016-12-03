/*
 * loopfilterconfig.c
 *
 *  Created on: Aug 10, 2016
 *      Author: Kunlun Yan
 */

#include "loopfilterconfig.h"

// 1. initialize filter parameters given noise bandwidth, damping ratio, and gain
void init_filter_parameters(filter_parameters *filter_par,double bandwidth, double dampingratio, double gain)
{
	double wn;
	wn = bandwidth * 8 * dampingratio / (4 * dampingratio * dampingratio + 1);

	// solve for t1 & t2
	filter_par->tau1 = gain / (wn * wn);
	filter_par->tau2 = 2.0 * dampingratio / wn;
}

// 2. initialize third order filter coefficient
void InitThirdOrderFilter(ThirdOrderFilterParameters *filter, double bandwidth)
{
	double wop;
	wop = bandwidth / 0.7845;
	filter->C1 = 2.4 * wop;
	filter->C2 = 1.1 * wop * wop;
	filter->C3 = wop * wop * wop;
}

// 3. initialize second order filter coefficient
void InitSecondOrderFilter(SecondOrderFilterParameters *filter, double bandwidth)
{
	double wof;
	wof = bandwidth / 0.53;
	filter->C1 = 1.414 * wof;
	filter->C2 = wof * wof;
}
