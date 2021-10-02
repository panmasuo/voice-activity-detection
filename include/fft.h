#ifndef FFT_H_
#define FFT_H_

#include <complex.h>
#include <stdio.h>

typedef double complex cplx;

void _fft(cplx buf[], cplx out[], int n, int step);
void fft(cplx buf[], int n);
void show(const char * s, cplx buf[], int frames);

#endif
