#include "def.h"
#include <string.h>
#include <mqueue.h>	    // -lrt
#include "MQTTClient.h" // -lpaho-mqtt3c
#ifdef LEDBLINK
#include <wiringPi.h>
#endif

#ifndef MQTT_H_
#define MQTT_H_

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "VadClientID"
#define TOPIC       "/VAD"
#define PAYLOAD     "Test!"
#define QOS         2
#define TIMEOUT     10000L
#define MSG_SIZE    2

struct mq_attr attr;

#endif
