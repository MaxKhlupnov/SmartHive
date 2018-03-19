#include <stdlib.h>

#include "MqttModule.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"

#include <parson.h>

static void * MqttGateway_ParseConfigurationFromJson(const char* configuration)
{
	MQTT_CONFIG * result;

	return result;
}

void MqttGateway_FreeConfiguration(void * configuration)
{

}

static MODULE_HANDLE MqttGateway_Create(BROKER_HANDLE broker, const void* configuration)
{
	LogInfo("MqttGateway_Create_Create call..");
	MQTT_GATEWAY_DATA * result;
	MQTT_CONFIG * config = (MQTT_CONFIG *)configuration;


	return result;
}

static void MqttGateway_Destroy(MODULE_HANDLE moduleHandle)
{
	LogInfo("MqttGateway_Destroy call..");
	if (moduleHandle == NULL)
	{
		LogError("Attempt to destroy NULL module");
	}
	else
	{
		MQTT_GATEWAY_DATA* module_data = (MQTT_GATEWAY_DATA*)moduleHandle;

		/* Tell thread to stop */
		module_data->gatewaysRunning = 0;
		
		free(module_data);
	}
}

static void MqttGateway_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	// Print the properties & content of the received message
	CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
}


static void MqttGateway_Start(MODULE_HANDLE moduleHandle)
{
	LogInfo("ZwaveDevice_Start call..");
	if (moduleHandle == NULL)
	{
		LogError("Attempt to start NULL module");
	}
	else
	{
		MESSAGE_CONFIG newMessageCfg;
		MAP_HANDLE newProperties = Map_Create(NULL);
		if (newProperties == NULL)
		{
			LogError("Failed to create message properties");
		}
		else
		{
			MQTT_GATEWAY_DATA* module_data = (MQTT_GATEWAY_DATA*)moduleHandle;
		}
	}
}


/*
*    Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_API_1 MqttGateway_APIS_all =
{
	{ MODULE_API_VERSION_1 },

	MqttGateway_ParseConfigurationFromJson,
	MqttGateway_FreeConfiguration,
	MqttGateway_Create,
	MqttGateway_Destroy,
	MqttGateway_Receive,
	MqttGateway_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
	MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(MQTT_GATEWAY_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
	MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
	(void)gateway_api_version;
	return (const MODULE_API *)&MqttGateway_APIS_all;
}
