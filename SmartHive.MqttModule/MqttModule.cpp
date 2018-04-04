#include <stdlib.h>
#include <parson.h>

#include "MqttModule.h"
#include "MqttAdapter.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"





static bool copyConfigValueAsString(const JSON_Object *jsonData, const char *propertyName, char** propertyValue) {

	const char* tmpValue = json_object_get_string(jsonData, propertyName);
	if (tmpValue == NULL)
	{
		LogError("call json_object_get_string for %s return NULL or error", propertyName);
		return false;
	}
	else if (mallocAndStrcpy_s(propertyValue, tmpValue) != 0) {
		LogError("Error allocating memory for property %s string value %s", propertyName, propertyValue);
		return false;
	}
	else {
		return true;
	}
}

static bool copyConfigValueAsInt(const JSON_Object *jsonData, const char *propertyName, unsigned int* propertyValue) {

	const char* tmpValue = json_object_get_string(jsonData, propertyName);
	if (tmpValue == NULL)
	{
		LogError("call json_object_get_string for {0} return NULL or error", propertyName);
		return false;
	}
	else if (sscanf(tmpValue, "%d", propertyValue) < 0)
	{
		LogError("Wrong parameter %s value %s(should be an integer number of port)", propertyName, tmpValue);
		return false;
	}
	else {
		return true;
	}
}


static void * MqttGateway_ParseConfigurationFromJson(const char* configuration)
{
	LogInfo("MqttGateway_ParseConfigurationFromJson call..");
	MQTT_CONFIG * result;
	
	if (configuration == NULL)
	{
		LogError("invalid module args.");
		result = NULL;
	}
	else
	{
		JSON_Value* json = json_parse_string((const char*)configuration);
		if (json == NULL)
		{
			LogError("unable to json_parse_string");
			result = NULL;
		}
		else
		{
			JSON_Object* root = json_value_get_object(json);
			if (root == NULL)
			{
				LogError("unable to json_value_get_object");
				result = NULL;
			}
			else
			{
				MQTT_CONFIG config;
				// Check if we can read all required properties first
				if (copyConfigValueAsString(root, "mqttBrokerAddress", &(config.mqttBrokerAddress)) &&
					copyConfigValueAsString(root, "clientId", &(config.clientId)) &&
					(copyConfigValueAsString(root, "topic2Publish", &(config.topic2Publish)) 
						|| copyConfigValueAsString(root, "topic2Subscribe", &(config.topic2Subscribe)))
					) {

					if (!copyConfigValueAsInt(root, "mqttBrokerPort", &config.mqttBrokerPort)) {
						//Set 1883 as a default MQTT port
						config.mqttBrokerPort = 1883;
					}

					result = (MQTT_CONFIG*)malloc(sizeof(MQTT_CONFIG));

					*result = config;
					LogInfo("MqttGateway config record: mqttBrokerAddress->%s mqttBrokerPort->%d",
						result->mqttBrokerAddress, result->mqttBrokerPort);
				}
				else
				{
					if (config.mqttBrokerAddress != NULL)
						free(config.mqttBrokerAddress);

					if (config.clientId != NULL)
						free(config.clientId);

					if (config.topic2Publish != NULL)
						free(config.topic2Publish);

					if (config.topic2Subscribe != NULL)
						free(config.topic2Subscribe);
				}				
			}
		}
	}

	return result;
}


void MqttGateway_FreeConfiguration(void * configuration)
{

}

static MODULE_HANDLE MqttGateway_Create(BROKER_HANDLE broker, const void* configuration)
{
	LogInfo("MqttGateway_Create_Create call..");
	MQTT_GATEWAY_DATA * result;

	if (broker == NULL || configuration == NULL)
	{
		LogError("invalid MqttModule args.");
		result = NULL;
	}
	else
	{
		/*allocate module data struct */
		result = (MQTT_GATEWAY_DATA*)malloc(sizeof(MQTT_GATEWAY_DATA));
		result->config = (MQTT_CONFIG *)configuration;
		result->mqttAdapter = new MqttAdapter(result->config);

	}
		

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

		if (module_data->mqttAdapter != NULL) {			
			free(module_data->mqttAdapter);
		}
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
	LogInfo("MqttGateway_Start call..");
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
