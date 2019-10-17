/* file for project defines and includes */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>     //TODO, refactor this into ifdefs
#include <pthread.h>		// -lpthread
#include <math.h>		   	// -lm
#include <semaphore.h>
#include <complex.h>

#ifndef PCM_CAPTURE_H_
#define PCM_CAPTURE_H_

/* Defines OPTIONS */

// #define FILES
// #define PRINT
// #define MQTT
// #define LEDBLINK

#define ALSA_PCM_NEW_HW_PARAMS_API
#define FRAME_SIZE      0.01         // ms
#define SAMPLING_RATE   44100

#define FFT_POINTS      256
#define FFT_STEP        (SAMPLING_RATE/FFT_POINTS)
#define NUM_OF_FRAMES   (FRAME_SIZE*SAMPLING_RATE) * 10000

#define DELAY           10000       // us

typedef double complex cplx;
pthread_mutex_t mx_sync1, mx_sync2;
sem_t sx_vadLock1, sx_vadLock2;
short *real_buffer;   // global variable for acquired pcm signal

/* time calculating */
double total_time;
clock_t start, end;

#ifdef FILES
FILE *fFeatures;
FILE *fSignal;
FILE *fSpectrum;
FILE *fVad;
#endif // FILES

#endif
