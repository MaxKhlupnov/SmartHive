#include <stdlib.h>
#include <parson.h>

#include "MqttModule.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"

#include "mqtt/async_client.h"

/**
* A callback class for use with the main MQTT client.
*/
class callback : public virtual mqtt::callback
{
public:
	void connected(const mqtt::string& cause)  override {
		LogInfo("Mqtt broker connected", cause);
	}
	void connection_lost(const mqtt::string& cause) override {

		LogError("Mqtt broker connection lost", cause);
	}

	void delivery_complete(mqtt::delivery_token_ptr tok) override {
	}

};


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

				const char* mqttBrokerAddress = json_object_get_string(root, "mqttBrokerAddress");
				if (mqttBrokerAddress == NULL)
				{
					LogError("unable to json_object_get_string for mqttBrokerAddress");
					result = NULL;
				}
				else if (mallocAndStrcpy_s(&(config.mqttBrokerAddress), mqttBrokerAddress) != 0)
				{
					LogError("Error allocating memory for mqttBrokerAddress string");
					result = NULL;
				}
				else
				{
					const char* sMqttBrokerPort = json_object_get_string(root, "mqttBrokerPort");					
					if (sMqttBrokerPort == NULL)
					{
						//Set 1883 as a default MQTT port
						config.mqttBrokerPort = 1883;
					}
					else if (sscanf(sMqttBrokerPort, "%d", &config.mqttBrokerPort) < 0)
					{
						LogError("Wrong parameter mqttBrokerPort value %s(should be an integer number of port)", sMqttBrokerPort);
						result = NULL;
					}
					else
					{
						const char* clientId = json_object_get_string(root, "clientId");
						if (clientId == NULL)
						{
							LogError("unable to json_object_get_string for clientId");
							result = NULL;
						}
						else if (mallocAndStrcpy_s(&(config.clientId), clientId) != 0) {
							LogError("Error allocating memory for clientId string");
							result = NULL;
						}
						else
						{
							/// TODO: Add other parameters parsing

							result = (MQTT_CONFIG*)malloc(sizeof(MQTT_CONFIG));
							if (result == NULL) {
								free(config.mqttBrokerAddress);
								free(config.clientId);
							}
							else {
								*result = config;
								LogInfo("MqttGateway config record: mqttBrokerAddress->%s mqttBrokerPort->%d",
									result->mqttBrokerAddress, result->mqttBrokerPort);
							}
						}
					}
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
	MQTT_CONFIG * config = (MQTT_CONFIG *)configuration;
	
	int serverURILen = printf("%s:%d", config->mqttBrokerAddress, config->mqttBrokerPort);
	char serverURI [serverURILen+1];
	
	if (sprintf_s(serverURI, sizeof(serverURI), "%s:%d", config->mqttBrokerAddress, config->mqttBrokerPort) < 0) {
		LogError("Error formatting server URI");
	}

	LogInfo("Initializing for server %s", serverURI);

	mqtt::async_client client(serverURI, config->clientId);
	
	callback cb;
	client.set_callback(cb);

	mqtt::connect_options conopts;
	
	

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
