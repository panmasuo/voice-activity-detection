#include "def.h"
#include "mqtt.h"
#ifdef LEDBLINK
#define LedPin 0
#endif

MQTTClient client;
mqd_t mq_read;

void *mqtt_pub_thrd() {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	char buffer[MSG_SIZE];
	int rc;

	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
		printf("Failed to connect, return code %d\n", rc);
		exit(-1);
	}

	mq_read = mq_open(TOPIC, O_RDWR);
	if(mq_read < 1) {
		perror("Error opening");
	}

	/* setup led blinking */
	#ifdef LEDBLINK
	if(wiringPiSetup() == -1) { //when initialize wiringPi failed, print message to screen
					printf("setup wiringPi failed !\n");
					return -1;
	}
	pinMode(LedPin, OUTPUT);
	#endif

	while(1) {
		rc = mq_receive(mq_read, buffer, MSG_SIZE, NULL);	// blocking until mq is not empty
		if(rc < 1) {
			perror("Error receiving");
		}
		pubmsg.payload = buffer;					// sending string
		pubmsg.payloadlen = strlen(buffer);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		printf("Message: %s "
		 		"on topic: %s for client with ID: %s\n",
		 		buffer, TOPIC, CLIENTID);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("\ttoken%d delivered\n", token);
		fflush(stdout);

		#ifdef LEDBLINK
		if (buffer[0] == 1) {
			digitalWrite(LedPin, LOW);
		} else if (buffer[0] == 0) {
			digitalWrite(LedPin, HIGH);
		} else {
			printf("Błąd w buforze\n");
		}
		#endif

	}
}
