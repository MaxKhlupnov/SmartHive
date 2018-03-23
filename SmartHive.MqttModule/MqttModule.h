#ifndef MQTT_DEVICE_H

#define MQTT_DEVICE_H

#include "module.h"
#include "broker.h"
#include "azure_c_shared_utility/threadapi.h"

typedef struct MQTT_CONFIG_TAG
{	
	const char*			mqttBrokerAddress;
	unsigned int		mqttBrokerPort;
	const char*			clientId;
	const char*			topic;
	unsigned int		QoS;
	const char*			caCertFile;
	const char*			clientCertFile;
	
} MQTT_CONFIG;

typedef struct MQTT_GATEWAY_DATA_TAG
{
	BROKER_HANDLE       broker;
	const void *        mqttConnectionHandle;
	MQTT_CONFIG			config;
	unsigned int        gatewaysRunning : 1;
} MQTT_GATEWAY_DATA;

#ifdef __cplusplus
extern "C"
{
#endif

	MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(MQTT_GATEWAY_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*MQTT_DEVICE_H*/