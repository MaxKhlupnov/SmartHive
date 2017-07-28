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

/*
*    Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_API_1 ZwaveDevice_APIS_all =
{
	{ MODULE_API_VERSION_1 },

	ZwaveDevice_ParseConfigurationFromJson //,
	//ZwaveDevice_FreeConfiguration,
	//ZwaveDevice_Create,
	//ZwaveDevice_Destroy,
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
