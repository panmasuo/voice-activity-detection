#include <signal.h>
#include "def.h"
#include "pcm_capture.h"
#include "vad_moatt.h"

#define MAX_SEM_COUNT 1

/* rtos global variables */
pthread_t pcm_sampling_hndl;
pthread_t vad_hndl;

/* function prototypes */
void sig_handler(int signum);
int  init_mutexes();

/* signal handler for ^C: closing all files, windows, mutexes, etc. */
void sig_handler(int signum) 
{
    if (signum != SIGINT) {
		printf("Invalid signum %d\n", signum);
	}

    pthread_cancel(pcm_sampling_hndl);
    pthread_cancel(vad_hndl);

    pthread_mutex_destroy(&mx_sync1);
    pthread_mutex_destroy(&signal_buffer_lock);

    sem_destroy(&sx_vadLock1);
    sem_destroy(&sx_vadLock2);

    printf("\nBye\n");
    exit(0);
}

int init_mutexes()
{
    printf("co?");
    pthread_mutexattr_t signal_buffer_lock_attr;

    sem_init(&sx_vadLock1, MAX_SEM_COUNT, 0);
    sem_init(&sx_vadLock2, MAX_SEM_COUNT, 1);

    if (pthread_mutex_init(&signal_buffer_lock, &signal_buffer_lock_attr) != 0) {
        printf("Mutex init failed!\n");
        return FAILURE;
    }

    if (pthread_mutex_init(&mx_sync1, NULL) != 0) {
        printf("Mutex init failed!\n");
        return FAILURE;
    }

    return SUCCESS;
}

int main() {
    signal(SIGINT, sig_handler);
    
    printf("Initializing semaphores and mutexes\r\n");
    if (init_mutexes() == FAILURE) {
        printf("Initialization failed\r\n");
        // todo: go to exit sequence
        return 0;
    }

    printf("Threads starting\n");
    pthread_create(&pcm_sampling_hndl, NULL, &pcm_sampling_thrd, NULL);
    pthread_create(&vad_hndl, NULL, &vad_moatt_thrd, NULL);

    while (1);

    pthread_join(pcm_sampling_hndl, NULL);
    pthread_join(vad_hndl, NULL);

    return 0;
}
