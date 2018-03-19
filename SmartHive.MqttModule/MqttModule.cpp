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
		//		int result;

		/* Tell thread to stop */
		module_data->gatewaysRunning = 0;
		/* join the thread */
		//ThreadAPI_Join(module_data->zwaveAdapter, &result);
		/* free module data */
		//		free((void*)module_data->sendNotificationOfType);
		free(module_data);
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
