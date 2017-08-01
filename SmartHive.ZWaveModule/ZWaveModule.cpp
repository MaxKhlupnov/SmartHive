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
	STRING_HANDLE IoTHubName;		//Sergei's comments
	STRING_HANDLE IoTHubSuffix;		//Sergei's comments
	char *              macAddress;
	unsigned int        messagePeriod;
} ZWAVEDEVICE_CONFIG;

#define SUFFIX "IoTHubSuffix"		//Sergei's comments
#define HUBNAME "IoTHubName"		//Sergei's comments


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
/*Sergei's comments*/
static void ZwaveDevice_FreeConfiguration(void* configuration)
{
	/*Codes_SRS_IOTHUBMODULE_05_014: [ If `configuration` is NULL then `IotHub_FreeConfiguration` shall do nothing. ]*/
	if (configuration != NULL)
	{
		ZWAVEDEVICE_CONFIG* config = (ZWAVEDEVICE_CONFIG*)configuration;

		/*Codes_SRS_IOTHUBMODULE_05_015: [ `IotHub_FreeConfiguration` shall free the strings referenced by the `IoTHubName` and `IoTHubSuffix` data members, and then free the `IOTHUB_CONFIG` structure itself. ]*/
		free((void*)config->IoTHubName);
		free((void*)config->IoTHubSuffix);
		free(config);
	}
}


/*Sergei's comments*/
//static MODULE_HANDLE ZwaveDevice_Create(BROKER_HANDLE broker, const void* configuration)
//{
//
//	ZWAVEDEVICE_CONFIG *result;
//	const ZWAVEDEVICE_CONFIG* config = configuration;
//
//	/*Codes_SRS_IOTHUBMODULE_02_001: [ If `broker` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
//	/*Codes_SRS_IOTHUBMODULE_02_002: [ If `configuration` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
//	/*Codes_SRS_IOTHUBMODULE_02_003: [ If `configuration->IoTHubName` is `NULL` then `IotHub_Create` shall and return `NULL`. ]*/
//	/*Codes_SRS_IOTHUBMODULE_02_004: [ If `configuration->IoTHubSuffix` is `NULL` then `IotHub_Create` shall fail and return `NULL`. ]*/
//	if (
//		(broker == NULL) ||
//		(configuration == NULL)
//		)
//	{
//		LogError("invalid arg broker=%p, configuration=%p", broker, configuration);
//		result = NULL;
//	}
//	else if (
//		(config->IoTHubName == NULL) ||
//		(config->IoTHubSuffix == NULL) ||
//		(config->transportProvider == NULL)
//		)
//	{
//		LogError("invalid configuration IoTHubName=%s IoTHubSuffix=%s transportProvider=%p",
//			(config != NULL) ? config->IoTHubName : "<null>",
//			(config != NULL) ? config->IoTHubSuffix : "<null>",
//			(config != NULL) ? config->transportProvider : 0);
//		result = NULL;
//	}
//	else
//	{
//		result = malloc(sizeof(IOTHUB_HANDLE_DATA));
//		/*Codes_SRS_IOTHUBMODULE_02_027: [ When `IotHub_Create` encounters an internal failure it shall fail and return `NULL`. ]*/
//		if (result == NULL)
//		{
//			LogError("malloc returned NULL");
//			/*return as is*/
//		}
//		else
//		{
//			/*Codes_SRS_IOTHUBMODULE_02_006: [ `IotHub_Create` shall create an empty `VECTOR` containing pointers to `PERSONALITY`s. ]*/
//			result->personalities = VECTOR_create(sizeof(PERSONALITY_PTR));
//			if (result->personalities == NULL)
//			{
//				/*Codes_SRS_IOTHUBMODULE_02_007: [ If creating the personality vector fails then `IotHub_Create` shall fail and return `NULL`. ]*/
//				free(result);
//				result = NULL;
//				LogError("VECTOR_create returned NULL");
//			}
//			else
//			{
//				result->transportProvider = config->transportProvider;
//				if (result->transportProvider == HTTP_Protocol ||
//					result->transportProvider == AMQP_Protocol)
//				{
//					/*Codes_SRS_IOTHUBMODULE_17_001: [ If `configuration->transportProvider` is `HTTP_Protocol` or `AMQP_Protocol`, `IotHub_Create` shall create a shared transport by calling `IoTHubTransport_Create`. ]*/
//					result->transportHandle = IoTHubTransport_Create(config->transportProvider, config->IoTHubName, config->IoTHubSuffix);
//					if (result->transportHandle == NULL)
//					{
//						/*Codes_SRS_IOTHUBMODULE_17_002: [ If creating the shared transport fails, `IotHub_Create` shall fail and return `NULL`. ]*/
//						VECTOR_destroy(result->personalities);
//						free(result);
//						result = NULL;
//						LogError("VECTOR_create returned NULL");
//					}
//				}
//				else
//				{
//					result->transportHandle = NULL;
//				}
//
//				if (result != NULL)
//				{
//					/*Codes_SRS_IOTHUBMODULE_02_028: [ `IotHub_Create` shall create a copy of `configuration->IoTHubName`. ]*/
//					/*Codes_SRS_IOTHUBMODULE_02_029: [ `IotHub_Create` shall create a copy of `configuration->IoTHubSuffix`. ]*/
//					if ((result->IoTHubName = STRING_construct(config->IoTHubName)) == NULL)
//					{
//						LogError("STRING_construct returned NULL");
//						IoTHubTransport_Destroy(result->transportHandle);
//						VECTOR_destroy(result->personalities);
//						free(result);
//						result = NULL;
//					}
//					else if ((result->IoTHubSuffix = STRING_construct(config->IoTHubSuffix)) == NULL)
//					{
//						LogError("STRING_construct returned NULL");
//						STRING_delete(result->IoTHubName);
//						IoTHubTransport_Destroy(result->transportHandle);
//						VECTOR_destroy(result->personalities);
//						free(result);
//						result = NULL;
//					}
//					else
//					{
//						/*Codes_SRS_IOTHUBMODULE_17_004: [ `IotHub_Create` shall store the broker. ]*/
//						result->broker = broker;
//						/*Codes_SRS_IOTHUBMODULE_02_008: [ Otherwise, `IotHub_Create` shall return a non-`NULL` handle. ]*/
//					}
//				}
//			}
//		}
//	}
//	return result;
//}


/*
*    Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_API_1 ZwaveDevice_APIS_all =
{
	{ MODULE_API_VERSION_1 },

	ZwaveDevice_ParseConfigurationFromJson,
	ZwaveDevice_FreeConfiguration,
	//ZwaveDevice_Create //,
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
