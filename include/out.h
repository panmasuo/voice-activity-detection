#include "def.h"
#include "MQTTClient.h" // -lpaho-mqtt3c

#ifndef MQTT_H_
#define MQTT_H_

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "VadClientID"
#define TOPIC       "/VAD"
#define PAYLOAD     "Test!"
#define QOS         2
#define TIMEOUT     10000L
#define MSG_SIZE    2

MQTTClient client;

#endif
