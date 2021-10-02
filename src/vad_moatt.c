/* VAD algorithm file */
#include "def.h"
#include "vad_moatt.h"

#define SPEECH_RUN_MIN_FRAMES   5
#define SILENCE_RUN_MIN_FRAMES  10
#define SILENCE 0
#define SPEECH  1
#define TRUE 1
#define FALSE 0

extern short *real_buffer; // global variable for acquired pcm signal

/* local variables */
typedef struct {
    long double energy;
    short F;
    float SFM;
} features_t;

typedef struct {
    unsigned char speech_run;
    unsigned char silence_run;
    unsigned char decision;
} vad_t;

/* static functions prototypes */
static void  calculate_fft(cplx *fft_signal);
static float calculate_energy(short *signal);
static float calculate_dominant(cplx *spectrum);
static float calculate_sfm(cplx *spectrum);
static void  set_minimum_feature(features_t *minimum, features_t *current, int i);
static int   calculate_counter(features_t *minimum, features_t *current, features_t *threshold);
static void  calculate_decision(vad_t *state, int counter);

/* static functions declarations */
static void calculate_fft(cplx *fft_signal)
{
    int i;
    for (i = 0; i < FFT_POINTS; i++) {
        fft_signal[i] = (real_buffer[i] + 0.0f * _Complex_I);
    }

    fft(fft_signal, FFT_POINTS);
}

static float calculate_sfm(cplx *spectrum) 
{
    int i;
    float sum_ari = 0;
    float sum_geo = 0;
    float abs_signal;

    for (i = 0; i < FFT_POINTS; i++) {
        abs_signal = cabsf(spectrum[i]);
        sum_ari += abs_signal;
        sum_geo += logf(abs_signal);
    }

    sum_ari = sum_ari / FFT_POINTS;
    sum_geo = expf(sum_geo / FFT_POINTS);

    /* reversing with '-' */
    return -10 * log10f(sum_geo / sum_ari);
}

static float calculate_dominant(cplx *spectrum)
{
    int i;
    int i_real, i_imag;
    float real, imag;
    float max_real, max_imag;

    for (i = 0; i < FFT_POINTS / 2; i++) {
        real = crealf(spectrum[i]);
        imag = cimagf(spectrum[i]);

        if (i == 0) {
            max_real = real;
            max_imag = imag;
            i_real = i;
            i_imag = i;
        } 
        else {
            if (real > max_real) {
                max_real = real;
                i_real = i;
            }

            if (imag > max_imag) {
                max_imag = imag;
                i_imag = i;
            }
        }
    }

    if (max_real > max_imag) {
        return i_real * FFT_STEP;
    } else {
        return i_imag * FFT_STEP;
    }
}

float calculate_energy(short *signal)
{
    int i;
    long double sum;

    sum = 0;
    for (i = 0; i < FFT_POINTS; i++) {
        sum += powl(signal[i], 2);
    }

    return sqrtl(sum / FFT_POINTS);
}

static void set_minimum_feature(features_t *minimum, features_t *current, int i)
{
    if (i == 0) {
        minimum->energy = current->energy;
        minimum->F = current->F;
        minimum->SFM = current->SFM;
    }
    else if (i < 30) {
        minimum->energy = (current->energy > minimum->energy) ? minimum->energy : current->energy;
        minimum->F = (current->F > minimum->F) ? minimum->F : current->F;
        minimum->SFM = (current->SFM > minimum->SFM) ? minimum->SFM : current->SFM;
    }
}

static int calculate_counter(features_t *minimum, features_t *current, features_t *threshold)
{
    int counter = 0;

    if ((current->energy - minimum->energy) >= threshold->energy) {
        counter++;
    }
    
    if ((current->F - minimum->F) >= threshold->F) {
        counter++;
    }
    
    if ((current->SFM - minimum->SFM) >= threshold->SFM) {
        counter++;
    }

    return counter;
}

static void calculate_decision(vad_t *state, int counter)
{
    /* 3-6: mark the current frame as speech else mark it as silence */
    if (counter > 1) {
        state->speech_run++;
        state->silence_run = 0;
    }
    else { 
        state->silence_run++;
        state->speech_run = 0;
    }

    /* 4-0 ignore silence run if less than 10 frames*/
    if (state->silence_run > SILENCE_RUN_MIN_FRAMES) {
        state->decision = FALSE;
    }

    /* 5-0 ignore speech run if less than 5 frames */
    if (state->speech_run > SPEECH_RUN_MIN_FRAMES)  {
        state->decision = TRUE;
    }
}

void *vad_moatt_thrd()
{
    int i;
    int counter;

    vad_t state;
    features_t minFeat;
    features_t curFeat;
    features_t primThresh;
    features_t currThresh;

    short *real_signal;
    real_signal = (short *)malloc(sizeof(short) * FFT_POINTS);
    cplx *fft_signal;
    fft_signal = (cplx *)malloc(sizeof(cplx) * FFT_POINTS);

    /* based on Moatt */
    primThresh.energy = 40;
    primThresh.F = 185;
    primThresh.SFM = 5;

    /* initial VAD decision */
    currThresh.F = primThresh.F; // moved from 3-4 for opt
    currThresh.SFM = primThresh.SFM;

    while (1) {
        for (i = 0; i < NUM_OF_FRAMES; i++) {
            sem_wait(&sx_vadLock1);
            printf("halo\r\n");
            pthread_mutex_lock(&signal_buffer_lock);
            memcpy(real_signal, real_buffer, sizeof(short) * FFT_POINTS);
            pthread_mutex_unlock(&signal_buffer_lock);

            /* 3-1, 3-2 calculate features */
            calculate_fft(fft_signal);
            curFeat.energy = calculate_energy(real_buffer);
            curFeat.F = calculate_dominant(fft_signal);
            curFeat.SFM = calculate_sfm(fft_signal);

            /* 3-3 calculate minimum value for first 30 frames */
            set_minimum_feature(&minFeat, &curFeat, i);

            /* 3-4 set thresholds, only energy threashold is changing */
            currThresh.energy = primThresh.energy * log10f(minFeat.energy);

            /* 3-5 calculate counter */
            counter = calculate_counter(&curFeat, &minFeat, &currThresh);

            /* 3-6, 4-0, 5-0: VAD */
            calculate_decision(&state, counter);

            /* 3-7, 3-8: update minimum energy */
            if (state.decision == SILENCE) {
                minFeat.energy = ((state.silence_run * minFeat.energy) + curFeat.energy) / (state.silence_run + 1);
            }
            currThresh.energy = primThresh.energy * log10f(minFeat.energy);
            sem_post(&sx_vadLock2);
        }
    }
    return 0;
}
