/* PCM capture file */
#include "def.h"
#include "pcm_capture.h"

void* pcm_sampling_thrd() {
  int rc, dir;
	unsigned int sampling_rate;
	snd_pcm_t *pcm_hndl;
	snd_pcm_hw_params_t *pcm_params;
	snd_pcm_uframes_t pcm_frames;

	sampling_rate = SAMPLING_RATE;						    // set sampling rate
	pcm_frames = FFT_POINTS;					            // set number of frames
	real_buffer = (short *) malloc((int)pcm_frames * 2);    // frames * short size

	// rc = snd_pcm_open(&pcm_hndl, "default", SND_PCM_STREAM_CAPTURE, 0);
      /* in case of problem with hardware use this one "$ arecord -l" */
      rc = snd_pcm_open(&pcm_hndl, "hw:3,0", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		fprintf(stderr, "Unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_alloca(&pcm_params);
	snd_pcm_hw_params_any(pcm_hndl, pcm_params);
	snd_pcm_hw_params_set_access(pcm_hndl, pcm_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_hndl, pcm_params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(pcm_hndl, pcm_params, 1);
	snd_pcm_hw_params_set_rate_near(pcm_hndl, pcm_params, &sampling_rate, &dir);
	if(SAMPLING_RATE != sampling_rate) {
	    fprintf(stderr, "The rate %d Hz is not supported, using %d Hz instead\n", SAMPLING_RATE, sampling_rate);
	}
	snd_pcm_hw_params_set_period_size_near(pcm_hndl, pcm_params, &pcm_frames, &dir);
	rc = snd_pcm_hw_params(pcm_hndl, pcm_params);
	if (rc < 0) {
	    fprintf(stderr, "unable to set hw parameters: %s \n", snd_strerror(rc));
	    exit(1);
	}

	/* Main LOOP for data acquisition */
	while(1) {
        // pthread_mutex_lock(&mx_sync1);
        sem_wait(&sx_vadLock2);            // wait for semaphore
        rc = snd_pcm_readi(pcm_hndl, real_buffer, pcm_frames);
        if (rc == -EPIPE) {
            fprintf(stderr, "Overrun\n");
            snd_pcm_prepare(pcm_hndl);
        }
        else if (rc < 0) {
            fprintf(stderr, "Error while reading: %s\n", snd_strerror(rc));
        }
        else if (rc == pcm_frames){
          // pthread_mutex_unlock(&mx_sync1);
          sem_post(&sx_vadLock1);
        }
        else {

        }
	}
}
