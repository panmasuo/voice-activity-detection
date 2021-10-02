/* file for project defines and includes */
#ifndef DEF_H_
#define DEF_H_
// #include <stdio.h>
// #include <stdlib.h>
#include <time.h>
#include <pthread.h>		// -lpthread
// #include <math.h>		   	// -lm
#include <semaphore.h>


#define SUCCESS 0
#define FAILURE 1

#define ALSA_PCM_NEW_HW_PARAMS_API
#define FRAME_SIZE      0.01         // ms
#define SAMPLING_RATE   44100

#define FFT_POINTS      256
#define FFT_STEP        (SAMPLING_RATE/FFT_POINTS)
#define NUM_OF_FRAMES   (FRAME_SIZE*SAMPLING_RATE) * 10000

#define DELAY           10000       // us

pthread_mutex_t mx_sync1, signal_buffer_lock;
sem_t sx_vadLock1, sx_vadLock2;
short *real_buffer;   // global variable for acquired pcm signal

/* time calculating */
double total_time;
clock_t start, end;

#endif
