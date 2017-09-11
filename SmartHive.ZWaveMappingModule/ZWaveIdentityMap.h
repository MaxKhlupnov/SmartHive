#pragma once

#ifndef ZWAVEIDENTITYMAP_H
#define ZWAVEIDENTITYMAP_H

#include "module.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define GW_ZWAVE_HOMEID_PROPERTY       "networkId"
#define GW_ZWAVE_NODEID_PROPERTY       "nodeId"

#define GW_ZWAVEIDMAP_MODULE  "mapping" //GW_ZWAVEIDMAP_MODULE -- should match to complain with other MS modules

	typedef struct ZWAVE_IDENTITY_MAP_CONFIG_TAG
	{
		unsigned int zwaveNetworkId; /* integer number of the network */
		const char* zwaveNodeId; /* can be a hex number of zwave node or star(*) match all nodes on the network*/
		const char* deviceId;
		const char* deviceKey;
	} ZWAVE_IDENTITY_MAP_CONFIG;

	MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(ZWAVE_IDENTITYMAP_MODULE)(MODULE_API_VERSION gateway_api_version);

#ifdef __cplusplus
}
#endif

#endif /*IDENTITYMAP_H*/
