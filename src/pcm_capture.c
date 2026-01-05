#include <alsa/asoundlib.h>

#include "config.h"
#include "pcm_capture.h"

#define CHANNEL_COUNT 1

static snd_pcm_t* init_pcm_params(snd_pcm_uframes_t pcm_frames);

static int read_pcm_frames(snd_pcm_t *pcm_hndl,
                           snd_pcm_uframes_t pcm_frames,
                           short *raw_samples);

snd_pcm_t* init_pcm_params(snd_pcm_uframes_t pcm_frames)
{
    int rc;
    int dir;
    snd_pcm_t *pcm_hndl;
    snd_pcm_hw_params_t *pcm_params;
    unsigned int sampling_rate = SAMPLING_RATE;

    printf("[PCM] initializing...\r\n");

    /* in case of problem with hardware use this one "$ arecord -l" */
    // TODO configurable pcm source
    //      use "$ arecord -l" for device id
    rc = snd_pcm_open(&pcm_hndl, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_any failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params_malloc(&pcm_params);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_malloc failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params_any(pcm_hndl, pcm_params);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_any failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params_set_access(pcm_hndl, pcm_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_set_access failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params_set_format(pcm_hndl, pcm_params, SND_PCM_FORMAT_S16_LE);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_set_format failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params_set_rate_near(pcm_hndl, pcm_params, &sampling_rate, &dir);
    if (SAMPLING_RATE != sampling_rate) {
        fprintf(stderr, "The rate %d Hz is not supported, using %d Hz instead\n", SAMPLING_RATE, sampling_rate);
    }

    rc = snd_pcm_hw_params_set_channels(pcm_hndl, pcm_params, CHANNEL_COUNT);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params_set_channels failed: %s\n", snd_strerror(rc));
        exit(1);
    }

    rc = snd_pcm_hw_params(pcm_hndl, pcm_params);
    if (rc < 0) {
        fprintf(stderr, "snd_pcm_hw_params failed: %s \n", snd_strerror(rc));
        exit(1);
    }

    snd_pcm_hw_params_free(pcm_params);

    printf("[PCM] initialized...\r\n");
    return pcm_hndl;
}

int read_pcm_frames(snd_pcm_t *pcm_hndl,
                    snd_pcm_uframes_t pcm_frames,
                    short *raw_samples)
{
    int rc;

    rc = snd_pcm_readi(pcm_hndl, raw_samples, pcm_frames);
    if (rc == -EPIPE) {
        fprintf(stderr, "[PCM Read] Overrun\n");
        return 1;
    }
    else if (rc < 0) {
        fprintf(stderr, "[PCM Read] Error while reading: %s\n", snd_strerror(rc));
        return 1;
    }

    return rc;
}

void* pcm_sampling_thrd(void *args)
{
    int rc;
    struct application_attributes *attrs = args;

    snd_pcm_uframes_t pcm_frames = NUMBER_OF_SAMPLES;
    snd_pcm_t *pcm_hndl = init_pcm_params(pcm_frames);

    printf("[PCM] starting thread loop\r\n");
    while (true) {
        sem_wait(&attrs->raw_buffer_copied);
        rc = read_pcm_frames(pcm_hndl, pcm_frames, attrs->raw_samples);
        if (!rc) {
            /* in case error post semaphore to enable reading in next loop */
            sem_post(&attrs->raw_buffer_copied);
            continue;
        }

        sem_post(&attrs->raw_buffer_ready);
    }
}
