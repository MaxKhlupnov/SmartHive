#include <stdlib.h>
#include "MqttModule.h"
#include "mqtt/async_client.h"

class MqttAdapter
{
public:
	MqttAdapter(MQTT_CONFIG* config);
	~MqttAdapter();

private:
	MQTT_CONFIG * config = NULL;
	mqtt::async_client* client = NULL;
	char * serverURI = NULL;
	void InitMqttClient();
};
