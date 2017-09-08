#include <stdlib.h>

#include "ZWaveModule.h"
#include "OpenZWaveAdapter.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"

#include <parson.h>

/* Azur eiot-edge module definition functions*/
static void ZwaveDevice_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	// Print the properties & content of the received message
	CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
	if (properties != NULL)
	{
		const char* addr = ((ZWAVEDEVICE_DATA*)moduleHandle)->sendNotificationOfType;

		// We're only interested in cloud-to-device (C2D) messages addressed to
		// this device
		if (ConstMap_ContainsKey(properties, GW_MAC_ADDRESS_PROPERTY) == true &&
			strcmp(addr, ConstMap_GetValue(properties, GW_MAC_ADDRESS_PROPERTY)) == 0)
		{
			const char* const * keys;
			const char* const * values;
			size_t count;

			if (ConstMap_GetInternals(properties, &keys, &values, &count) == CONSTMAP_OK)
			{
				const CONSTBUFFER* content = Message_GetContent(messageHandle);
				if (content != NULL)
				{
					(void)printf(
						"Received a message\r\n"
						"Properties:\r\n"
					);

					for (size_t i = 0; i < count; ++i)
					{
						(void)printf("  %s = %s\r\n", keys[i], values[i]);
					}

					(void)printf("Content:\r\n");
					(void)printf("  %.*s\r\n", (int)content->size, content->buffer);
					(void)fflush(stdout);
				}
			}
		}

		ConstMap_Destroy(properties);
	}

	return;
}

static void ZwaveDevice_Destroy(MODULE_HANDLE moduleHandle)
{
	LogInfo("ZwaveDevice_Destroy call..");
	if (moduleHandle == NULL)
	{
		LogError("Attempt to destroy NULL module");
	}
	else
	{
		ZWAVEDEVICE_DATA* module_data = (ZWAVEDEVICE_DATA*)moduleHandle;
		int result;

		/* Tell thread to stop */
		module_data->zwaveDeviceRunning = 0;		
		/* join the thread */
		//ThreadAPI_Join(module_data->zwaveAdapter, &result);
		/* free module data */
//		free((void*)module_data->sendNotificationOfType);
		free(module_data);
	}
}

static void ZwaveDevice_Start(MODULE_HANDLE moduleHandle)
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
			ZWAVEDEVICE_DATA* module_data = (ZWAVEDEVICE_DATA*)moduleHandle;

			if (Map_Add(newProperties, GW_SOURCE_PROPERTY, GW_SOURCE_ZWAVE_TELEMETRY) != MAP_OK)
			{
				LogError("Failed to set source property");
			}
			/*else if (Map_Add(newProperties, GW_ZWAVE_HOMEID_PROPERTY, ) != MAP_OK)
			{
				LogError("Failed to set address property");
			}*/
			else if (Map_Add(newProperties, "deviceFunction", module_data->sendNotificationOfType) != MAP_OK)
			{
				LogError("Failed to set deviceFunction property");
			}

			else
			{
				newMessageCfg.size = 0;
				newMessageCfg.source = NULL;
				newMessageCfg.sourceProperties = newProperties;

				MESSAGE_HANDLE newMessage = Message_Create(&newMessageCfg);
				if (newMessage == NULL)
				{
					LogError("Failed to create register message");
				}
				else
				{
					if (Broker_Publish(module_data->broker, (MODULE_HANDLE)module_data, newMessage) != BROKER_OK)
					{
						LogError("Failed to publish register message");
					}

					Message_Destroy(newMessage);
				}
			}
		}
		Map_Destroy(newProperties);

	ZWAVEDEVICE_DATA* module_data = (ZWAVEDEVICE_DATA*)moduleHandle;

		OpenZWaveAdapter* adapter = new OpenZWaveAdapter(module_data);
		adapter->Start();	

	}
}

static MODULE_HANDLE ZwaveDevice_Create(BROKER_HANDLE broker, const void* configuration)
{
	LogInfo("ZwaveDevice_Create call..");
	ZWAVEDEVICE_DATA * result;
	ZWAVEDEVICE_CONFIG * config = (ZWAVEDEVICE_CONFIG *)configuration;
	if (broker == NULL || config == NULL)
	{
		LogError("invalid SIMULATED DEVICE module args.");
		result = NULL;
	}
	else
	{

		/* allocate module data struct */
		result = (ZWAVEDEVICE_DATA*)malloc(sizeof(ZWAVEDEVICE_DATA));
		if (result == NULL)
		{
			LogError("couldn't allocate memory for BLE Module");
		}
		else
		{

			/* save the message broker */
			result->broker = broker;
			/* set module is running to true */
			result->zwaveDeviceRunning = 1;

			result->sendNotificationOfType = config->sendNotificationOfType;
			result->controllerPath = config->controllerPath;
			result->zwaveConfigPath = config->zwaveConfigPath;
			//result->zwaveDeviceThread = NULL;
			LogInfo("Created ZWave Device: controllerPath->%s sendNotificationOfType->%s\n", result->controllerPath, result->sendNotificationOfType);
		}
	}



	return result;
}

void ZwaveDevice_FreeConfiguration(void * configuration)
{
	if (configuration != NULL)
	{
		ZWAVEDEVICE_CONFIG_TAG * config = (ZWAVEDEVICE_CONFIG_TAG *)configuration;
		free(config->sendNotificationOfType);
		free(config);
	}
}

static void * ZwaveDevice_ParseConfigurationFromJson(const char* configuration)
{
	ZWAVEDEVICE_CONFIG * result;
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
				ZWAVEDEVICE_CONFIG config;
				const char* controllerPath = json_object_get_string(root, "controllerPath");
				if (controllerPath == NULL)
				{
					LogError("unable to json_object_get_string for controllerPath");
					result = NULL;
				}
				else
				{

					if (mallocAndStrcpy_s(&(config.controllerPath), controllerPath) != 0)
					{
						LogError("Error allocating memory for controllerPath string");
						result = NULL;
					}
					else {


						const char* sendNotificationOfType = json_object_get_string(root, "sendNotificationOfType");
						if (sendNotificationOfType == NULL)
						{
							LogError("unable to json_object_get_string for sendNotificationOfType");
							result = NULL;
						}
						else if (mallocAndStrcpy_s(&(config.sendNotificationOfType), sendNotificationOfType) != 0)
						{
							result = NULL;
						}else 
						{
							const char * zwaveConfigPath = json_object_get_string(root, "zwaveConfigPath");
							if (zwaveConfigPath == NULL)
							{
								LogError("unable to json_object_get_string for zwaveConfigPath");
								result = NULL;
							}
							else
							{
								if (mallocAndStrcpy_s(&(config.zwaveConfigPath), zwaveConfigPath) != 0) {
										LogError("Error allocating memory for controllerPath string");
										result = NULL;
								}
								else 
								{									
									result = (ZWAVEDEVICE_CONFIG*)malloc(sizeof(ZWAVEDEVICE_CONFIG));
									if (result == NULL) {
										free(config.sendNotificationOfType);
										free(config.controllerPath);
										free(config.zwaveConfigPath);
										LogError("allocation of configuration failed");
									}
									else
									{
										*result = config;
										LogInfo("Zwave device config record: controllerPath->%s sendNotificationOfType->%s config.zwaveConfigPath->%s \n", config.controllerPath, config.sendNotificationOfType, config.zwaveConfigPath);
									}
								}
							}
						}
					}
				}
			}
			json_value_free(json);
		}
	}
	return result;
}

/*
*    Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_API_1 ZwaveDevice_APIS_all =
{
	{ MODULE_API_VERSION_1 },

	ZwaveDevice_ParseConfigurationFromJson,
	ZwaveDevice_FreeConfiguration,
	ZwaveDevice_Create,
	ZwaveDevice_Destroy,
	ZwaveDevice_Receive,
	ZwaveDevice_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(ZWAVE_DEVICE_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
	(void)gateway_api_version;
	return (const MODULE_API *)&ZwaveDevice_APIS_all;
}
