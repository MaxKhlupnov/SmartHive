#ifndef ZWAVE_DEVICE_H
#define ZWAVE_DEVICE_H

#include "module.h"
#include "broker.h"
#include "azure_c_shared_utility/threadapi.h"

#define GW_SOURCE_ZWAVE_TELEMETRY     "zwaveTelemetry"

#define GW_ZWAVE_HOMEID_PROPERTY       "networkId"
#define GW_ZWAVE_NODEID_PROPERTY       "nodeId"
#define GW_ZWAVE_NOTIFICATION_TYPE_PROPERTY   "type"

typedef struct ZWAVEDEVICE_DATA_TAG 
{
	BROKER_HANDLE       broker;
	THREAD_HANDLE       zwaveDeviceThread;
	const char *        sendNotificationOfType;
	const char *        controllerPath;
	const char *        zwaveConfigPath;
	unsigned int        zwaveDeviceRunning : 1;
} ZWAVEDEVICE_DATA;


typedef struct ZWAVEDEVICE_CONFIG_TAG
{
	char *              sendNotificationOfType;
	char *              controllerPath;
	char *				zwaveConfigPath;
} ZWAVEDEVICE_CONFIG;

#ifdef __cplusplus
extern "C"
{  
#endif

	MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(ZWAVE_DEVICE_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*ZWAVE_DEVICE_H*/

