#include "def.h"
#include "fft.h"
#include "mqtt.h"

#ifndef VAD_MOATT_H_
#define VAD_MOATT_H_

typedef struct {
  long double energy;
  short       F;
  float       SFM;
} Features;

extern short *real_buffer;   // global variable for acquired pcm signal
typedef double complex cplx;

float calculate_energy(short *signal);
float calculate_dominant(cplx *spectrum);
float calculate_sfm(cplx *spectrum);
void *vad_thrd();

#endif
