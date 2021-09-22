/* VAD algorithm file */
#include "def.h"
#include "vad_moatt.h"

#ifdef MQTT
	mqd_t mq_write;
#endif

float calculate_sfm(cplx *spectrum) {
	int i;
	float sum_ari;
	float sum_geo;
	float sig;

	sum_ari = 0;
	sum_geo = 0;
	for(i = 0; i < FFT_POINTS; i++) {
		sig = cabsf(spectrum[i]);
		sum_ari += sig;
		sum_geo += logf(sig);
	}
	sum_ari = sum_ari/FFT_POINTS;
	sum_geo = expf(sum_geo / FFT_POINTS);

	return -10 * log10f(sum_geo/sum_ari);
}

float calculate_dominant(cplx *spectrum) {
	int i;
	int i_real, i_imag;
	float real, imag;
	float max_real, max_imag;

	for(i = 0; i < FFT_POINTS/2; i++) {
		real = crealf(spectrum[i]);
		imag = cimagf(spectrum[i]);

		if(i == 0) {
			max_real = real;
			max_imag = imag;
			i_real = i;
			i_imag = i;
		}
		else {
			if(real > max_real) {
				max_real = real;
				i_real = i;
			}

			if(imag > max_imag) {
				max_imag = imag;
				i_imag = i;
			}
		}
	}

	if(max_real > max_imag) {
		return i_real * FFT_STEP;
	}
	else {
		return i_imag * FFT_STEP;
	}
}

float calculate_energy(short *signal) {
	int i;
	long double sum;

	sum = 0;
	for(i = 0; i < FFT_POINTS; i++) {
		sum += powl(signal[i], 2);
	}

	return sqrtl(sum/FFT_POINTS);
}

void *vad_thrd() {
	int i, j;
	int counter;
	int silence_count;
	int silence_run, speech_run;
	int vad_dec;
	cplx *fft_signal;

	/* init features structures for
	 * Features:	 minimum and current
	 * Thresholds: primary and current */
	Features minFeature;
	Features currFeature;
	Features primThresh;
	Features currThresh;

	fft_signal = (cplx *) malloc(sizeof(cplx) * FFT_POINTS);

#ifdef MQTT
	char buffer[MSG_SIZE];
	int rc = 0;
	int vad_dec_old = 0;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MSG_SIZE;
	attr.mq_curmsgs = 0;
	mq_write = mq_open(TOPIC, O_CREAT | O_RDWR, 0644, &attr);
	if(mq_write < 1) {
		perror("Error opening");
	}
	else {
			printf("Opened message queue!\n");
	}
#endif


#ifdef FILES
	// f = fopen("files/test.csv", "w");

fFeatures = fopen("files/Features.csv", "w");
fSignal = fopen("files/Signal.csv", "w");
fSpectrum = fopen("files/Spectrum.csv", "w");
fVad = fopen("files/Vad.csv", "w");
#endif

	/* based on Moatt */
	primThresh.energy = 40;
	primThresh.F			= 185;
	primThresh.SFM		= 5;

	/* initial VAD decision */
	vad_dec = 0;
	silence_count = 0;							// TODO: check this (where to put this?)
	currThresh.F = primThresh.F;			// moved from 3-4 for opt
	currThresh.SFM = primThresh.SFM;
//	extern double total_time;
//	extern clock_t start, end;


	/* MAIN VAD LOOP */
	while(1) {
		for(i = 0; i < NUM_OF_FRAMES; i++) {
//		start = clock();
			sem_wait(&sx_vadLock1);						 // wait for semaphore
			pthread_mutex_lock(&mx_sync1);		// if ready, lock shared resources


			/* 3-2 calculate FFT */
			for(j = 0; j < FFT_POINTS; j++) {
				fft_signal[j] = (real_buffer[j] + 0.0f*_Complex_I);
			}
			fft(fft_signal, FFT_POINTS);

			#ifdef FILES
			for(j = 0; j < FFT_POINTS; j++) {
				fprintf(fSignal,"%hi\n", real_buffer[j]);
				if(j < FFT_POINTS - 1) {
					fprintf(fSpectrum,"%f,", cabsf(fft_signal[i]/FFT_POINTS)*2);
				}
				else if(j == FFT_POINTS - 1) {
					fprintf(fSpectrum,"%f\n", cabsf(fft_signal[i]/FFT_POINTS)*2);
				}
			}
			#endif

			/* 3-1 + 3-2 calculate features */
			currFeature.energy = calculate_energy(real_buffer);
			pthread_mutex_unlock(&mx_sync1);

			currFeature.F = calculate_dominant(fft_signal);
			currFeature.SFM = calculate_sfm(fft_signal);

			/* 3-3 calculate minimum value for first 30 frames */
			if(i == 0) {
				minFeature.energy		= currFeature.energy;
				minFeature.F				= currFeature.F;
				minFeature.SFM			= currFeature.SFM;
			}
			else if(i < 30) {
				minFeature.energy = (currFeature.energy > minFeature.energy) ? minFeature.energy : currFeature.energy;
				minFeature.F			= (currFeature.F			> minFeature.F)			 ? minFeature.F			 : currFeature.F;
				minFeature.SFM		= (currFeature.SFM		> minFeature.SFM)		 ? minFeature.SFM		 : currFeature.SFM;
			}

			/* 3-4 set thresholds */
			currThresh.energy = primThresh.energy * log10f(minFeature.energy);

			/* 3-5 calculate decision */
			counter = 0;
			if((currFeature.energy - minFeature.energy) >= currThresh.energy) {
				counter++;
				#ifdef FILES
					fprintf(fVad,"1,");
				#endif
			} else {
				#ifdef FILES
					fprintf(fVad,"0,");
				#endif
			}
			if((currFeature.F - minFeature.F) >= currThresh.F) {
				counter++;
				#ifdef FILES
					fprintf(fVad,"1,");
				#endif
			} else {
				#ifdef FILES
					fprintf(fVad,"0,");
				#endif
			}
			if((currFeature.SFM - minFeature.SFM) >= currThresh.SFM) {
				counter++;
				#ifdef FILES
					fprintf(fVad,"1,");
				#endif
			} else {
				#ifdef FILES
					fprintf(fVad,"0,");
				#endif
			}

			/* 3-6, 3-7, 3-8: VAD */
			if(counter > 1) {
				speech_run++;
				silence_run = 0;
				#ifdef FILES
				fprintf(fVad,"1,");
				#endif
			}
			else {
					/* silence run or silence count? */
				silence_run++;
				silence_count++;
				minFeature.energy = ((silence_run * minFeature.energy) + currFeature.energy) / (silence_run + 1);
				speech_run = 0;
				#ifdef FILES
				fprintf(fVad,"0,");
				#endif
			}

			currThresh.energy = primThresh.energy * log10f(minFeature.energy);

			/* 4-0 ignore silence run if less than 10 frames*/
			if(silence_run > 10) {
				vad_dec = 0;
			}

			/* 5-0 ignore speech run if less than 5 frames */
			if(speech_run > 4) {
				vad_dec = 1;
			}

#ifdef MQTT
			if(vad_dec != vad_dec_old) {
				snprintf(buffer, sizeof(buffer), "%d", vad_dec);
				rc = mq_send(mq_write, buffer, strlen(buffer), 1);
				if(rc == -1) {
					perror("Error sending");
				}
				fflush(stdout);
			}
			vad_dec_old = vad_dec;
#endif

#ifdef PRINT
			printf("Feature:\n\t%Lf\n\t%d\n\t%f\n", currFeature.energy, currFeature.F, currFeature.SFM);
			printf("Min Feature:\n\t%Lf\n\t%d\n\t%f\n",	 minFeature.energy, minFeature.F, minFeature.SFM);
			printf("Thresh Feature:\n\t%Lf\n\t%d\n\t%f\n\n",	currThresh.energy, currThresh.F, currThresh.SFM);
			printf("VAD:%d\t%d\n", vad_dec,i);
#endif

#ifdef FILES
			fprintf(fFeatures,"%Lf,", currFeature.energy);
			fprintf(fFeatures,"%d,", currFeature.F);
			fprintf(fFeatures,"%f,", currFeature.SFM);

			fprintf(fFeatures,"%Lf,", currThresh.energy);
			fprintf(fFeatures,"%d,", currThresh.F);
			fprintf(fFeatures,"%f,", currThresh.SFM);

			fprintf(fFeatures,"%Lf,", minFeature.energy);
			fprintf(fFeatures,"%d,", minFeature.F);
			fprintf(fFeatures,"%f\n", minFeature.SFM);

			fprintf(fVad,"%d\n", vad_dec);
#endif
			// printf("VAD: %d [%d]\n", vad_dec, i);
			sem_post(&sx_vadLock2);
//			end = clock();
//			total_time = ((double)(end-start))/CLOCKS_PER_SEC;
//			printf("Czas: %.8f\n", total_time);
		}
	}
	return 0;
}
