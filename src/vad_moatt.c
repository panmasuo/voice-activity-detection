/* VAD algorithm file */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "config.h"
#include "fft.h"
#include "vad_moatt.h"

#define SPEECH_RUN_MIN_FRAMES   4
#define SILENCE_RUN_MIN_FRAMES  10

#define FRAME_SIZE_MS   0.01
#define FFT_POINTS      NUMBER_OF_SAMPLES
#define FFT_STEP        (SAMPLING_RATE / FFT_POINTS)
#define NUM_OF_FRAMES   (FRAME_SIZE_MS * SAMPLING_RATE)

typedef enum {
    DECISION_SILENCE = 0,
    DECISION_SPEECH = 1
} vad_decision;

typedef struct {
    float energy;
    float F;
    float SFM;
} features;

typedef struct {
    int speech_run;
    int silence_run;
    int decision;
} vad;

static void initialize_primary_thresholds(features *primary);
static void initialize_current_thresholds(features *current, features *primary);

/**
 * @brief Calculate fast fourier transform for given signal.
 *
 * @param real_buffer buffer to calculate transform on.
 * @param fft_signal returned fft signal.
 */
static void calculate_fft(short *real_buffer, cplx *fft_signal);

static float calculate_energy(short *signal);
static float calculate_dominant(cplx *spectrum);
static float calculate_sfm(cplx *spectrum);

static void set_minimum_feature(features *minimum, features *current, int i);
static int calculate_counter(features *minimum, features *current, features *threshold);
static int calculate_decision(vad *state, int counter);

void initialize_primary_thresholds(features *primary)
{
    primary->energy = 40;
    primary->F = 185;
    primary->SFM = 5;
}

void initialize_current_thresholds(features *current, features *primary)
{
    current->energy = primary->energy;
    current->F = primary->F;
    current->SFM = primary->SFM;
}

void calculate_fft(short *real_buffer, cplx *fft_signal)
{
    for (int i = 0; i < FFT_POINTS; i++) {
        // zero out the imaginary part
        fft_signal[i] = (real_buffer[i] + 0.0f * _Complex_I);
    }

    fft(fft_signal, FFT_POINTS);
}

float calculate_energy(short *signal)
{
    float sum = 0;

    for (int i = 0; i < FFT_POINTS; i++) {
        sum += powl(signal[i], 2);
    }

    return sqrtl(sum / FFT_POINTS);
}

float calculate_dominant(cplx *spectrum)
{
    int i_real, i_imag;
    float real, imag;
    float max_real, max_imag;

    for (int i = 0; i < FFT_POINTS / 2; i++) {
        real = crealf(spectrum[i]);
        imag = cimagf(spectrum[i]);

        // set first values to max
        if (i == 0) {
            max_real = real;
            max_imag = imag;
            i_real = i;
            i_imag = i;

            continue;
        }

        if (real > max_real) {
            max_real = real;
            i_real = i;
        }

        if (imag > max_imag) {
            max_imag = imag;
            i_imag = i;
        }
    }

    return max_real > max_imag ? (float)(i_real * FFT_STEP) : (float)(i_imag * FFT_STEP);
}

float calculate_sfm(cplx *spectrum)
{
    float sum_ari = 0;
    float sum_geo = 0;
    float abs_signal = 0;

    for (int i = 0; i < FFT_POINTS; i++) {
        abs_signal = cabsf(spectrum[i]);
        sum_ari += abs_signal;
        sum_geo += logf(abs_signal);
    }

    sum_ari = sum_ari / FFT_POINTS;
    sum_geo = expf(sum_geo / FFT_POINTS);

    return -10 * log10f(sum_geo / sum_ari);
}

void set_minimum_feature(features *minimum, features *current, int i)
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

int calculate_counter(features *minimum, features *current, features *threshold)
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

int calculate_decision(vad *state, int counter)
{
    /* 3-6: mark the current frame as speech else mark it as silence */
    if (counter >= 2) {
        state->speech_run++;
        state->silence_run = 0;
    }
    else {
        state->silence_run++;
        state->speech_run = 0;
    }

    /* 4-0 ignore silence run if less than 10 frames */
    if (state->silence_run > SILENCE_RUN_MIN_FRAMES) {
        return DECISION_SILENCE;
    }

    /* 5-0 ignore speech run if less than 4 frames */
    if (state->speech_run > SPEECH_RUN_MIN_FRAMES)  {
        return DECISION_SPEECH;
    }


    /* return current state in case silence/speech run is not detected */
    return state->decision;
}

void *vad_moatt_thrd(void *args)
{
    struct application_attributes *attrs = args;

    int counter;

    vad state = {};
    features minimum = {};
    features current = {};
    features primary_threshold = {};
    features current_threshold = {};

    short real_signal[FFT_POINTS] = {};
    cplx fft_signal[FFT_POINTS] = {};

    /* based on Moatt */
    initialize_primary_thresholds(&primary_threshold);

    /* initial VAD decision */
    initialize_current_thresholds(&current_threshold, &primary_threshold); // moved from 3-4 for opt

    printf("[VAD] starting thread loop\r\n");
    while (true) {
        for (int i = 0; i < NUM_OF_FRAMES; i++) {
            sem_wait(&attrs->raw_buffer_ready);
            // TODO is is safe? is it fast enough? maybe move/swap/anything
            memcpy(real_signal, attrs->raw_samples, sizeof(short) * FFT_POINTS);
            sem_post(&attrs->raw_buffer_copied);

            calculate_fft(real_signal, fft_signal);

            /* 3-1, 3-2 calculate features */
            current.energy = calculate_energy(real_signal);
            current.F = calculate_dominant(fft_signal);
            current.SFM = calculate_sfm(fft_signal);

            /* 3-3 calculate minimum value for first 30 frames */
            set_minimum_feature(&minimum, &current, i);

            /* 3-4 set thresholds, only energy threshold is changing */
            current_threshold.energy = primary_threshold.energy * log10f(minimum.energy);

            /* 3-5 calculate counter */
            counter = calculate_counter(&minimum, &current, &current_threshold);

            /* 3-6, 4-0, 5-0: VAD */
            state.decision = calculate_decision(&state, counter);

            /* 3-7, 3-8: update minimum energy */
            if (state.decision == DECISION_SILENCE) {
                minimum.energy = ((state.silence_run * minimum.energy) + current.energy) / (state.silence_run + 1);
            }
        }
    }
}
