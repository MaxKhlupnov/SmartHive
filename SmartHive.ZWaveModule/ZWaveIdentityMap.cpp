
#include <stddef.h>
#include <stdlib.h>
#include <azure_c_shared_utility/strings.h>
#include <ctype.h>

#include "azure_c_shared_utility/crt_abstractions.h"
#include "messageproperties.h"
#include "message.h"
#include "broker.h"
#include "ZWaveIdentityMap.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"

#include <parson.h>

typedef struct ZWAVE_IDENTITY_MAP_DATA_TAG
{
	BROKER_HANDLE broker;
	size_t mappingSize;
	ZWAVE_IDENTITY_MAP_CONFIG * zwaveToDevIdArray;
	ZWAVE_IDENTITY_MAP_CONFIG * devIdToZWaveArray;
} IDENTITY_MAP_DATA;

#define ZWAVE_IDENTITYMAP_RESULT_VALUES \
    IDENTITYMAP_OK, \
    IDENTITYMAP_ERROR, \
    IDENTITYMAP_MEMORY

DEFINE_ENUM(ZWAVE_IDENTITYMAP_RESULT, ZWAVE_IDENTITYMAP_RESULT_VALUES)

DEFINE_ENUM_STRINGS(BROKER_RESULT, BROKER_RESULT_VALUES);


#define DEVICENAME "deviceId"
#define DEVICEKEY "deviceKey"

static ZWAVE_IDENTITYMAP_RESULT IdentityMapConfig_CopyDeep(ZWAVE_IDENTITY_MAP_CONFIG * dest, ZWAVE_IDENTITY_MAP_CONFIG * source);
static void IdentityMapConfig_Free(ZWAVE_IDENTITY_MAP_CONFIG * element);

static bool addOneRecord(VECTOR_HANDLE inputVector, JSON_Object * record)
{
	bool success;
	if (record == NULL)
	{
		/*Codes_SRS_IDMAP_05_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
		success = false;
	}
	else
	{
		const char * networkId;
		const char * nodeId;
		const char * deviceId;
		const char * deviceKey;
		if ((networkId = json_object_get_string(record, GW_ZWAVE_HOMEID_PROPERTY)) == NULL)
		{
			/*Codes_SRS_IDMAP_05_008: [ If the array object does not contain a value named "networkId" then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", GW_ZWAVE_HOMEID_PROPERTY);
			success = false;
		}
		if ((nodeId = json_object_get_string(record, GW_ZWAVE_NODEID_PROPERTY)) == NULL)
		{
			/*Codes_SRS_IDMAP_05_008: [ If the array object does not contain a value named "nodeId" then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", GW_ZWAVE_NODEID_PROPERTY);
			success = false;
		}

		else if ((deviceId = json_object_get_string(record, DEVICENAME)) == NULL)
		{
			/*Codes_SRS_IDMAP_05_010: [ If the array object does not contain a value named "deviceId" then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", DEVICENAME);
			success = false;
		}
		else if ((deviceKey = json_object_get_string(record, DEVICEKEY)) == NULL)
		{
			/*Codes_SRS_IDMAP_05_011: [ If the array object does not contain a value named "deviceKey" then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", DEVICEKEY);
			success = false;
		}
		else
		{
			/*Codes_SRS_IDMAP_05_012: [ IdentityMap_ParseConfigurationFromJson shall use "macAddress", "deviceId", and "deviceKey" values as the fields for an IDENTITY_MAP_CONFIG structure and call VECTOR_push_back to add this element to the vector. ]*/
			ZWAVE_IDENTITY_MAP_CONFIG config, newrecord;
			config.zwaveNetworkId = networkId;
			config.zwaveNodeId = nodeId;
			config.deviceId = deviceId;
			config.deviceKey = deviceKey;
			if (IdentityMapConfig_CopyDeep(&newrecord, &config) != IDENTITYMAP_OK)
			{
				LogError("Could not copy map configuration strings");
				success = false;
			}
			else
			{
				if (VECTOR_push_back(inputVector, &newrecord, 1) != 0)
				{
					IdentityMapConfig_Free(&newrecord);
					/*Codes_SRS_IDMAP_05_020: [ If pushing into the vector is not successful, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
					LogError("Did not push vector");
					success = false;
				}
				else
				{
					success = true;
				}
			}
		}
	}
	return success;
}


/*
* @brief    duplicate a string and convert to upper case. New string must be released
*            no longer needed.
*/
static const char * IdentityMapConfig_ToUpperCase(const char * string)
{
	char * result;
	int status;
	status = mallocAndStrcpy_s(&result, string);
	if (status != 0) // failure
	{
		result = NULL;
	}
	else
	{
		char * temp = result;
		while (*temp)
		{
			*temp = toupper(*temp);
			temp++;
		}
	}

	return result;
}

/*
* @brief    Duplicate and copy all fields in the IDENTITY_MAP_CONFIG struct.
*            Forces MAC address to be all upper case (to guarantee comparisons are consistent).
*/
static ZWAVE_IDENTITYMAP_RESULT IdentityMapConfig_CopyDeep(ZWAVE_IDENTITY_MAP_CONFIG * dest, ZWAVE_IDENTITY_MAP_CONFIG * source)
{
	ZWAVE_IDENTITYMAP_RESULT result;
	int status;

	return result;
}

/*
* @brief    Free strings associated with IDENTITY_MAP_CONFIG structure
*/
static void IdentityMapConfig_Free(ZWAVE_IDENTITY_MAP_CONFIG * element)
{	
	free((void*)element->zwaveNetworkId);
	free((void*)element->zwaveNodeId);
	free((void*)element->deviceId);
	free((void*)element->deviceKey);
}
