#include <stdlib.h>

#include "ZWaveModule.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "module.h"
#include "broker.h"

#include <parson.h>

typedef struct ZWAVEDEVICE_DATA_TAG
{
	BROKER_HANDLE       broker;
	THREAD_HANDLE       simulatedDeviceThread;
	const char *        fakeMacAddress;
	unsigned int        messagePeriod;
	unsigned int        simulatedDeviceRunning : 1;
} ZWAVEDEVICE_DATA;


typedef struct ZWAVEDEVICE_CONFIG_TAG
{
	char *              macAddress;
	unsigned int        messagePeriod;
} ZWAVEDEVICE_CONFIG;

/*Sergei's add module*/
static void ZwaveDevice_Destroy(MODULE_HANDLE moduleHandle)
{
	if (moduleHandle == NULL)
	{
		LogError("Attempt to destroy NULL module");
	}
	else
	{
		ZWAVEDEVICE_DATA* module_data = (ZWAVEDEVICE_DATA*)moduleHandle;
		int result;

		/* Tell thread to stop */
		module_data->simulatedDeviceRunning = 0;
		/* join the thread */
		ThreadAPI_Join(module_data->simulatedDeviceThread, &result);
		/* free module data */
		free((void*)module_data->fakeMacAddress);
		free(module_data);
	}
}

/*Sergei's add module*/
static MODULE_HANDLE ZwaveDevice_Create(BROKER_HANDLE broker, const void* configuration)
{
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
			result->simulatedDeviceRunning = 1;
			/* save fake MacAddress */
			char * newFakeAddress;
			int status = mallocAndStrcpy_s(&newFakeAddress, config->macAddress);

			if (status != 0)
			{
				LogError("MacAddress did not copy");
			}
			else
			{
				result->fakeMacAddress = newFakeAddress;
				result->messagePeriod = config->messagePeriod;
				result->simulatedDeviceThread = NULL;

			}

		}
	}
	return result;
}

static void* ZwaveDevice_ParseConfigurationFromJson(const char* configuration)
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
				const char* macAddress = json_object_get_string(root, "macAddress");
				if (macAddress == NULL)
				{
					LogError("unable to json_object_get_string");
					result = NULL;
				}
				else
				{
					int period = (int)json_object_get_number(root, "messagePeriod");
					if (period <= 0)
					{
						LogError("Invalid period time specified");
						result = NULL;
					}
					else
					{
						if (mallocAndStrcpy_s(&(config.macAddress), macAddress) != 0)
						{
							result = NULL;
						}
						else
						{
							config.messagePeriod = period;
							result = (ZWAVEDEVICE_CONFIG*)malloc(sizeof(ZWAVEDEVICE_CONFIG));
							if (result == NULL) {
								free(config.macAddress);
								LogError("allocation of configuration failed");
							}
							else
							{
								*result = config;
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


/*Sergei's add module*/
void ZwaveDevice_FreeConfiguration(void * configuration)
{
	if (configuration != NULL)
	{
		ZWAVEDEVICE_CONFIG * config = (ZWAVEDEVICE_CONFIG *)configuration;
		free(config->macAddress);
		free(config);
	}
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
	//ZwaveDevice_Receive,
	//ZwaveDevice_Start
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
