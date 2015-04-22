/*----------------------------------------------------------------------------
fft.c - fast Fourier transform and its inverse (both recursively)
Copyright (C) 2004, Jerome R. Breitenbach.  All rights reserved.

The author gives permission to anyone to freely copy, distribute, and use
this file, under the following conditions:
- No changes are made.
- No direct commercial advantage is obtained.
- No liability is attributed to the author for any damages incurred.
----------------------------------------------------------------------------*/

/******************************************************************************
* This file defines a C function fft that, by calling another function       *
* fft_rec (also defined), calculates an FFT recursively.  Usage:             *
*   fft(nNumPts, x_time_d, x_frequency_d);                                                            *
* Parameters:                                                                *
*   nNumPts: number of points in FFT (must equal 2^n for some integer n >= 1)      *
*   x_time_d: pointer to nNumPts time-domain samples given in rectangular form (Re x_time_d,     *
*      Im x_time_d)                                                                 *
*   x_frequency_d: pointer to nNumPts frequency-domain samples calculated in rectangular form  *
*      (Re x_frequency_d, Im x_frequency_d)                                                          *
* Similarly, a function IFFT with the same parameters is defined that        *
* calculates an inverse FFT (IFFT) recursively.  Usage:                      *
*   IFFT(nNumPts, x_time_d, x_frequency_d);                                                           *
* Here, nNumPts and x_frequency_d are given, and x_time_d is calculated.                              *
******************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#ifndef MY_PI
#define MY_PI		(3.1415926535897932384626433832795)
#endif

#ifndef TWO_PI
#define TWO_PI		(6.283185307179586476925286766559)
#endif

#ifndef MY_CPXV_TYPE
#define MY_CPXV_TYPE
typedef struct _cpxv_t {
	double re;
	double im;
} cpxv_t;
#endif

//#define NumSamplesPerFrameM			1850
//#define NumBinsInFftWinM			2048
extern const int NumSamplesPerFrameM = 1850;
extern const int NumBinsInFftWinM = 2048;

void fft_rec(int nNumPts, const int offset, const int delta, cpxv_t *x_td, cpxv_t *x_fd, cpxv_t *XX);

/* FFT */
// nNumPts : number of samples(points) in time domain;
// x_time_d : (input)sequence of samples(each is a complex number);
// x_frequency_d : (output)sequence of samples in frequency domain(each is a complex number).
void FFT(int nNumPts, cpxv_t *x_time_d, cpxv_t *x_frequency_d) {
	/* Declare a pointer to scratch space. */
	//cpxv_t *XX = (cpxv_t *)malloc(nNumPts * sizeof(cpxv_t));
	cpxv_t XX[NumBinsInFftWinM];
	/* Calculate FFT by a recursion. */
	fft_rec(nNumPts, 0, 1, x_time_d, x_frequency_d, XX);

	/* Free memory. */
	//free(XX);
}

/* FFT recursion */
void fft_rec(int nNumPts, const int offset, const int delta,
	cpxv_t *x_time_d, cpxv_t *x_frequency_d, cpxv_t *XX) {
	int N2 = nNumPts / 2;            /* half the number of points in FFT */
	int k;                   /* generic index */
	double cs, sn;           /* cosine and sine */
	int k00, k01, k10, k11;  /* indices for butterflies */
	double tmp0, tmp1;       /* temporary storage */

	if (nNumPts != 2) {
		/* Perform recursive step. */
		/* Calculate two (nNumPts/2)-point DFT's. */
		fft_rec(N2, offset, 2 * delta, x_time_d, XX, x_frequency_d);
		fft_rec(N2, offset + delta, 2 * delta, x_time_d, XX, x_frequency_d);

		/* Combine the two (nNumPts/2)-point DFT's into one nNumPts-point DFT. */
		for (k = 0; k<N2; k++) {
			cs = cos(TWO_PI*k / (double)nNumPts); sn = sin(TWO_PI*k / (double)nNumPts);

			k10 = offset + 2 * k*delta;
			k11 = k10 + delta;
			tmp0 = XX[k11].re * cs + XX[k11].im * sn;	// Re part
			tmp1 = XX[k11].im * cs - XX[k11].re * sn;	// Im part

			k00 = offset + k*delta;
			k01 = k00 + N2*delta;

			x_frequency_d[k00].re = XX[k10].re + tmp0;
			x_frequency_d[k00].im = XX[k10].im + tmp1;

			x_frequency_d[k01].re = XX[k10].re - tmp0;
			x_frequency_d[k01].im = XX[k10].im - tmp1;
		}
	}
	else {
		/* Perform 2-point DFT. */
		k00 = offset;
		k01 = k00 + delta;
		x_frequency_d[k00].re = x_time_d[k00].re + x_time_d[k01].re;
		x_frequency_d[k00].im = x_time_d[k00].im + x_time_d[k01].im;

		x_frequency_d[k01].re = x_time_d[k00].re - x_time_d[k01].re;
		x_frequency_d[k01].im = x_time_d[k00].im - x_time_d[k01].im;
	}
}


/* IFFT
void IFFT(int nNumPts, cpxv_t *x_time_d, cpxv_t *x_frequency_d)
{
int N2 = nNumPts/2;       // half the number of points in IFFT
int i;              // generic index
double tmp0, tmp1;  // temporary storage

// Calculate IFFT via reciprocity property of DFT.
FFT(nNumPts, x_frequency_d, x_time_d);
x_time_d[ 0].re = x_time_d[ 0].re/nNumPts;
x_time_d[ 0].im = x_time_d[ 0].im/nNumPts;
x_time_d[N2].re = x_time_d[N2].re/nNumPts;
x_time_d[N2].im = x_time_d[N2].im/nNumPts;
for(i=1; i<N2; i++) {
tmp0 = x_time_d[i].re/nNumPts;
tmp1 = x_time_d[i].im/nNumPts;
x_time_d[i].re = x_time_d[nNumPts-i].re/nNumPts;
x_time_d[i].im = x_time_d[nNumPts-i].im/nNumPts;
x_time_d[nNumPts-i].re = tmp0;
x_time_d[nNumPts-i].im = tmp1;
}
}
*/

void PreEmphasize(cpxv_t *sample, int nNumSamps);

void DoFFT(const short xxx[], cpxv_t fdom[]) {
	//cpxv_t* tdom = new cpxv_t[NumBinsInFftWinM];
	cpxv_t tdom[NumBinsInFftWinM];
	int isamp;
	for (isamp = 0; isamp<NumSamplesPerFrameM; isamp++) {// copy
		tdom[isamp].re = xxx[isamp];
		tdom[isamp].im = 0.0;
	}
	PreEmphasize(tdom, NumSamplesPerFrameM);
	for (isamp = 0; isamp<NumSamplesPerFrameM; isamp++) {
		// Hamming windowing
		tdom[isamp].re *= (0.54 - 0.46*cos(TWO_PI*isamp / NumSamplesPerFrameM));
		tdom[isamp].im = 0.0;
	}
	for (; isamp<NumBinsInFftWinM; isamp++) {// zero padding
		tdom[isamp].re = 0.0;
		tdom[isamp].im = 0.0;
	}
	FFT(NumBinsInFftWinM, tdom, fdom);
	//free(tdom);
}


// Filter : H(z) = 1-a*z^(-1)
void PreEmphasize(cpxv_t *sample, int nNumSamps) {
	// Setting emphFac=0 turns off preemphasis.

	double fEmphFactor = 0.92;	//0.95;	//0.97;
	for (int iSamp = nNumSamps - 1; iSamp > 0; iSamp--) {
		sample[iSamp].re = sample[iSamp].re - fEmphFactor * sample[iSamp - 1].re;
	}
	sample[0].re = (1.0 - fEmphFactor) * sample[0].re;
}