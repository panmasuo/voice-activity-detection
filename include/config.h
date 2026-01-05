/* file for project defines and includes */
#ifndef DEF_H_
#define DEF_H_

#include <semaphore.h>

#define NUMBER_OF_SAMPLES 128
#define SAMPLING_RATE     44100

struct application_attributes {
    short raw_samples[NUMBER_OF_SAMPLES];
    sem_t raw_buffer_ready;
    sem_t raw_buffer_copied;
};

enum {
    STATUS_SUCCESS,
    STATUS_FAILURE
};

#endif
