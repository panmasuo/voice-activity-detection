#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "pcm_capture.h"
#include "vad_moatt.h"

#define MAX_SEM_COUNT 1
#define INIT_DISABLE  0
#define INIT_ENABLE   1

static int init_mutexes(struct application_attributes* attrs);

int init_mutexes(struct application_attributes* attrs)
{
    if (sem_init(&attrs->raw_buffer_ready, MAX_SEM_COUNT, INIT_DISABLE)) {
        return STATUS_FAILURE;
    }

    if (sem_init(&attrs->raw_buffer_copied, MAX_SEM_COUNT, INIT_ENABLE)) {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

int main()
{
    printf("Starting application...");

    pthread_t pcm_sampling_hndl;
    pthread_t vad_hndl;

    struct application_attributes attrs;

    if (STATUS_FAILURE == init_mutexes(&attrs)) {
        printf("Initialization failed\r\n");
        exit(STATUS_FAILURE);
    }

    // TODO does it need to be casted to void?
    pthread_create(&pcm_sampling_hndl, NULL, &pcm_sampling_thrd, (void *)&attrs);
    pthread_create(&vad_hndl, NULL, &vad_moatt_thrd, (void *)&attrs);

    pthread_join(pcm_sampling_hndl, NULL);
    pthread_join(vad_hndl, NULL);

    return STATUS_SUCCESS;
}
