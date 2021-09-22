#include "def.h"
#include "pcm_capture.h"
#include "vad_moatt.h"
#include "fft.h"
#include "mqtt.h"
#include <signal.h>

/* global variables */
FILE *f, *spectrum;

/* rtos global variables */
pthread_t pcm_sampling_hndl;
pthread_t vad_hndl;
pthread_t mqtt_hndl;

extern void* pcm_sampling_thrd();
extern void* vad_thrd();
extern void* mqtt_pub_thrd();
pthread_mutexattr_t mx_sync1_attr, mx_sync2_attr;

/* function prototypes */
void sig_handler(int signum);

extern mqd_t mq_write, mq_read;
extern MQTTClient client;

/* signal handler for ^C: closing all files, windows, mutexes, etc. */
void sig_handler(int signum) {
	if(signum != SIGINT) {
		printf("Invalid signum %d\n", signum);
	}

	pthread_cancel(pcm_sampling_hndl);
	pthread_cancel(vad_hndl);
	pthread_cancel(mqtt_hndl);

	mq_unlink(TOPIC);

	pthread_mutex_destroy(&mx_sync1);
	pthread_mutex_destroy(&mx_sync2);
	sem_destroy(&sx_vadLock1);
	sem_destroy(&sx_vadLock2);

#ifdef FILES
	fclose(fFeatures);
	fclose(fSignal);
	fclose(fSpectrum);
	fclose(fVad);
#endif

#ifdef MQTT
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	mq_close(mq_read);
	mq_close(mq_write);
#endif

	printf("\nBye\n");
	exit(0);
}

int main() {
	printf("pzar 2019\n");
	signal(SIGINT, sig_handler);
	sem_init(&sx_vadLock1, 1, 0);
	sem_init(&sx_vadLock2, 1, 1);
	if (pthread_mutex_init(&mx_sync2, NULL) != 0) {
			printf("\nMutex init failed!\n");
			return 0;
	}
	if (pthread_mutex_init(&mx_sync1, NULL) != 0) {
			printf("\nMutex init failed!\n");
			return 0;
	}

	printf("Threads starting\n");
	pthread_create(&pcm_sampling_hndl, NULL, &pcm_sampling_thrd, NULL);
	pthread_create(&vad_hndl, NULL, &vad_thrd, NULL);
	#ifdef MQTT
	pthread_create(&mqtt_hndl, NULL, &mqtt_pub_thrd, NULL);
	#endif

	printf("Wait for join\n");
	while (1);

	pthread_join(pcm_sampling_hndl, NULL);
	pthread_join(vad_hndl, NULL);
	pthread_join(mqtt_hndl, NULL);

	return 0;
}
