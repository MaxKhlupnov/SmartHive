#ifndef MQTT_DEVICE_H

#define MQTT_DEVICE_H

#include "module.h"
#include "broker.h"
#include "azure_c_shared_utility/threadapi.h"


typedef struct MQTT_CONFIG_TAG
{	
	char*			mqttBrokerAddress= NULL;
	unsigned int	mqttBrokerPort;
	char*			clientId;
	char*			topic2Publish = NULL;
	char*			topic2Subscribe = NULL; 
	unsigned int	QoS;
	char*			login;
	char*			password;
	char*			caCertFile;
	char*			clientCertFile;
	
} MQTT_CONFIG;

typedef struct MQTT_GATEWAY_DATA_TAG
{
	BROKER_HANDLE       broker;
	void *       mqttAdapter;
	MQTT_CONFIG*		config;
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