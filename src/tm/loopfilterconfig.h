/*
 * loopfilterconfig.h
 *
 *  Created on: Aug 10, 2016
 *      Author: dma
 */

#ifndef LOOPFILTERCONFIG_H_
#define LOOPFILTERCONFIG_H_

// define code/carrier loop parameters
typedef struct filter_parameters_s{
	double tau1;
	double tau2;
} filter_parameters;

// initialize filter parameters
void init_filter_parameters(filter_parameters *filter_par,double bandwidth, double dampingratio, double gain);

// third-order parameters
typedef struct ThirdOrderFilterParametersS{
	double C1;
	double C2;
	double C3;
} ThirdOrderFilterParameters;

// second-order parameters
typedef struct SecondOrderFilterParametersS{
	double C1;
	double C2;
} SecondOrderFilterParameters;

// initialize third order parameters
void InitThirdOrderFilter(ThirdOrderFilterParameters *filter, double bandwidth);
// initialize second order parameters
void InitSecondOrderFilter(SecondOrderFilterParameters *filter, double bandwidth);

#endif /* LOOPFILTERCONFIG_H_ */
