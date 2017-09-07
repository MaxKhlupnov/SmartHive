#pragma once

#ifndef IDENTITYMAP_H
#define IDENTITYMAP_H

#include "module.h"
#include "ZWaveModule.h"
#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct ZWAVE_IDENTITY_MAP_CONFIG_TAG
	{
		const char* zwaveNetworkId;
		const char* zwaveNodeId;
		const char* deviceId;
		const char* deviceKey;
	} ZWAVE_IDENTITY_MAP_CONFIG;

	MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(IDENTITYMAP_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*IDENTITYMAP_H*/
