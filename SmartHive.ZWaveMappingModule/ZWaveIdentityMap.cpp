
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <ctype.h>

#include <parson.h>
#include "messageproperties.h"
#include "message.h"
#include "broker.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/constmap.h"
#include "azure_c_shared_utility/constbuffer.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/vector.h"



#include "ZWaveIdentityMap.h"

typedef struct ZWAVE_IDENTITY_MAP_DATA_TAG
{
	BROKER_HANDLE broker;
	size_t mappingSize;
	ZWAVE_IDENTITY_MAP_CONFIG * zwaveToDevIdArray;
	ZWAVE_IDENTITY_MAP_CONFIG * devIdToZWaveArray;
} ZWAVE_IDENTITY_MAP_DATA;

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

		const char * nodeId;
		const char * deviceId;
		const char * deviceKey;
		const char * sNetworkId;
		unsigned int networkId = -1;


		if ((sNetworkId = json_object_get_string(record, GW_ZWAVE_HOMEID_PROPERTY)) == NULL)		
		{
			/*Codes_SRS_IDMAP_05_008: [ If the array object does not contain a value named "networkId" then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Did not find expected %s configuration", GW_ZWAVE_HOMEID_PROPERTY);
			success = false;
		}
		else if ( sscanf(sNetworkId, "%08x", &networkId) < 0) {

			LogError("Wrong parameter %s value %s (should be an integer number of the zwave network)", GW_ZWAVE_HOMEID_PROPERTY, sNetworkId);
			success = false;

		}else if ((nodeId = json_object_get_string(record, GW_ZWAVE_NODEID_PROPERTY)) == NULL)
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
			ZWAVE_IDENTITY_MAP_CONFIG  config, newrecord;
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

	dest->zwaveNetworkId = source->zwaveNetworkId;

	dest->zwaveNodeId = IdentityMapConfig_ToUpperCase(source->zwaveNodeId);
	if (dest->zwaveNodeId == NULL)
	{
		LogError("Unable to allocate macAddress");
		result = IDENTITYMAP_MEMORY;
	}
	else
	{
		char * temp;
		status = mallocAndStrcpy_s(&temp, source->deviceId);
		if (status != 0)
		{
			LogError("Unable to allocate deviceId");
			free((void*)dest->zwaveNodeId);
			dest->zwaveNodeId = NULL;
			result = IDENTITYMAP_MEMORY;
		}
		else
		{
			dest->deviceId = temp;
			status = mallocAndStrcpy_s(&temp, source->deviceKey);
			if (status != 0)
			{
				LogError("Unable to allocate deviceKey");
				free((void*)dest->deviceId);
				dest->deviceId = NULL;
				free((void*)dest->zwaveNodeId);
				dest->zwaveNodeId = NULL;
				result = IDENTITYMAP_MEMORY;
			}
			else
			{
				dest->deviceKey = temp;
				result = IDENTITYMAP_OK;
			}
		}
	}

	return result;
}

/*
* @brief    Free strings associated with IDENTITY_MAP_CONFIG structure
*/
static void IdentityMapConfig_Free(ZWAVE_IDENTITY_MAP_CONFIG * element)
{		
	free((void*)element->zwaveNodeId);
	free((void*)element->deviceId);
	free((void*)element->deviceKey);
}

/*
* @brief    Comparison function by deviceId for two IDENTITY_MAP_CONFIG structures
*/
static int IdentityMapConfig_IdCompare(const void * a, const void * b)
{
	const ZWAVE_IDENTITY_MAP_CONFIG * idA = (ZWAVE_IDENTITY_MAP_CONFIG *) a;
	const ZWAVE_IDENTITY_MAP_CONFIG * idB = (ZWAVE_IDENTITY_MAP_CONFIG *) b;
	return strcmp(idA->deviceId, idB->deviceId);
}

/*
* @brief    Walks through our mappingVector to ensure it is correct for our identity map module.
*/
static bool IdentityMap_ValidateConfig(const VECTOR_HANDLE mappingVector)
{
	size_t mappingSize = VECTOR_size(mappingVector);
	bool mappingOk;
	if (mappingSize == 0)
	{
		/*Codes_SRS_IDMAP_17_041: [If the configuration has no vector elements, this function shall fail and return NULL.*/
		LogError("nothing for this module to do, no mapping data");
		mappingOk = false;
	}
	else
	{
		mappingOk = true;
		size_t index;
		for (index = 0; (index < mappingSize) && (mappingOk != false); index++)
		{
			ZWAVE_IDENTITY_MAP_CONFIG * element = (ZWAVE_IDENTITY_MAP_CONFIG *)VECTOR_element(mappingVector, index);
			if ((element->deviceId == NULL) ||
				(element->deviceKey == NULL) ||
				(element->zwaveNodeId == NULL) ||
				(element->zwaveNetworkId <= 0))
			{
				/*Codes_SRS_IDMAP_17_019: [If any zwaveNodeId,zwaveNetworkId,  deviceId or deviceKey are NULL, this function shall fail and return NULL.]*/
				LogError("Empty mapping data values, zwaveNetworkId=%d zwaveNodeId=%p, ID=%p, key=%p",
					element->zwaveNetworkId, element->zwaveNodeId, element->deviceId, element->deviceKey);
				mappingOk = false;
				break;
			}			
		}
	}
	return mappingOk;
}

/*
* @brief    Comparison function by ZWave HomeId and NodeID address for two IDENTITY_MAP_CONFIG structures
*/
static int IdentityMapConfig_ZWaveAddressCompare(const void * a, const void * b) {

	const ZWAVE_IDENTITY_MAP_CONFIG * idA = (ZWAVE_IDENTITY_MAP_CONFIG *)a;
	const ZWAVE_IDENTITY_MAP_CONFIG * idB = (ZWAVE_IDENTITY_MAP_CONFIG *)b;

	if (idA->zwaveNetworkId != idB->zwaveNetworkId)
		return -1;
	else if (strcmp(idA->zwaveNodeId, "*") == 0)
		return 0;
	else if (strcmp(idB->zwaveNodeId, "*") == 0)
		return 0;
	else
		return strcmp(idA->zwaveNodeId, idB->zwaveNodeId);
}

/* returns true if the message should continue to be processed, sets direction */
static bool determine_message_direction(const char * source, bool * isC2DMessage)
{
	bool result;
	if (source != NULL)
	{
		if (strcmp(source, GW_IOTHUB_MODULE) == 0)
		{
			result = true;
			*isC2DMessage = true;
		}
		else if (strcmp(source, GW_ZWAVEIDMAP_MODULE) != 0)
		{
			result = true;
			*isC2DMessage = false;
		}
		else
		{
			/*Codes_SRS_IDMAP_17_047: [ If messageHandle property "source" is not equal to "iothub", then the message shall not be marked as a C2D message. ]*/
			/*Codes_SRS_IDMAP_17_044: [ If messageHandle properties contains a "source" property that is set to "mapping", the message shall not be marked as a D2C message. ]*/
			LogError("Message did not have a known source, message dropped");
			result = false;
			*isC2DMessage = false;
		}
	}
	else
	{
		/*Codes_SRS_IDMAP_17_046: [ If messageHandle properties does not contain a "source" property, then the message shall not be marked as a C2D message. ]*/
		LogError("Message did not contain a source, message dropped");
		result = false;
		*isC2DMessage = false;
	}
	return result;
}
static void publish_with_new_properties(MAP_HANDLE newProperties, MESSAGE_HANDLE messageHandle, ZWAVE_IDENTITY_MAP_DATA * idModule)
{
	/*Codes_SRS_IDMAP_17_034: [IdentityMap_Receive shall clone message content.] */
	CONSTBUFFER_HANDLE content = Message_GetContentHandle(messageHandle);
	if (content == NULL)
	{
		/*Codes_SRS_IDMAP_17_035: [If cloning message content fails, IdentityMap_Receive shall deallocate all resources and return.]*/
		LogError("Could not extract message content");
	}
	else
	{
		MESSAGE_BUFFER_CONFIG newMessageConfig =
		{
			content,
			newProperties
		};
		/*Codes_SRS_IDMAP_17_036: [IdentityMap_Receive shall create a new message by calling Message_CreateFromBuffer with new map and cloned content.]*/
		MESSAGE_HANDLE newMessage = Message_CreateFromBuffer(&newMessageConfig);
		if (newMessage == NULL)
		{
			/*Codes_SRS_IDMAP_17_037: [If creating new message fails, IdentityMap_Receive shall deallocate all resources and return.]*/
			LogError("Could not create new message to publish");
		}
		else
		{
			BROKER_RESULT brokerStatus;
			/*Codes_SRS_IDMAP_17_038: [IdentityMap_Receive shall call Broker_Publish with broker and new message.]*/
			brokerStatus = Broker_Publish(idModule->broker, (MODULE_HANDLE)idModule, newMessage);
			if (brokerStatus != BROKER_OK)
			{
				LogError("Message broker publish failure: %s", ENUM_TO_STRING(BROKER_RESULT, brokerStatus));
			}
			Message_Destroy(newMessage);
		}
		/*Codes_SRS_IDMAP_17_039: [IdentityMap_Receive will destroy all resources it created.]*/
		CONSTBUFFER_Destroy(content);
	}
}
/*
* @brief    Republish message with new data from our matching identities.
*/
static void IdentityMap_RepublishD2C(
	ZWAVE_IDENTITY_MAP_DATA * idModule,
	MESSAGE_HANDLE messageHandle,
	ZWAVE_IDENTITY_MAP_CONFIG * match)
{
	CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
	if (properties == NULL)
	{

		LogError("Could not extract message properties");
	}
	else
	{
		/*Codes_SRS_IDMAP_17_026: [On a message which passes all checks, IdentityMap_Receive shall call ConstMap_CloneWriteable on the message properties.]*/
		MAP_HANDLE newProperties = ConstMap_CloneWriteable(properties);
		if (newProperties == NULL)
		{
			/*Codes_SRS_IDMAP_17_027: [If ConstMap_CloneWriteable fails, IdentityMap_Receive shall deallocate any resources and return.] */
			LogError("Could not make writeable new properties map");
		}
		else
		{
			/*Codes_SRS_IDMAP_17_028: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "deviceName" and value of found deviceId.]*/
			if (Map_AddOrUpdate(newProperties, GW_DEVICENAME_PROPERTY, match->deviceId) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_029: [If adding deviceName fails,IdentityMap_Receive shall deallocate all resources and return.]*/
				LogError("Could not attach %s property to message", GW_DEVICENAME_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_030: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "deviceKey" and value of found deviceKey.]*/
			else if (Map_AddOrUpdate(newProperties, GW_DEVICEKEY_PROPERTY, match->deviceKey) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_031: [If adding deviceKey fails, IdentityMap_Receive shall deallocate all resources and return.]*/
				LogError("Could not attach %s property to message", GW_DEVICEKEY_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_032: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "source" and value of "mapping".]*/
			else if (Map_AddOrUpdate(newProperties, GW_SOURCE_PROPERTY, GW_ZWAVEIDMAP_MODULE) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_033: [If adding source fails, IdentityMap_Receive shall deallocate all resources and return.]*/
				LogError("Could not attach %s property to message", GW_SOURCE_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_053: [ IdentityMap_Receive shall call Map_Delete to remove the "networkId" property. ]*/
			else if (Map_Delete(newProperties, GW_ZWAVE_HOMEID_PROPERTY) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_054: [ If deleting the NetworkId fails, IdentityMap_Receive shall deallocate all resources and return. ]*/
				LogError("Could not remove %s property from message", GW_ZWAVE_HOMEID_PROPERTY);
			}
			else if (Map_Delete(newProperties, GW_ZWAVE_NODEID_PROPERTY) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_054: [ If deleting the NodeId fails, IdentityMap_Receive shall deallocate all resources and return. ]*/
				LogError("Could not remove %s property from message", GW_ZWAVE_NODEID_PROPERTY);
			}
			else
			{
				publish_with_new_properties(newProperties, messageHandle, idModule);
			}
			Map_Destroy(newProperties);
		}

	}
}

/*
* @brief    Republish message with new data from our matching identities.
*/
static void IdentityMap_RepublishC2D(
	ZWAVE_IDENTITY_MAP_DATA * idModule,
	MESSAGE_HANDLE messageHandle,
	ZWAVE_IDENTITY_MAP_CONFIG * match)
{
	CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
	if (properties == NULL)
	{

		LogError("Could not extract message properties");
	}
	else
	{
		/*Codes_SRS_IDMAP_17_049: [ On a C2D message received, IdentityMap_Receive shall call ConstMap_CloneWriteable on the message properties. ]*/
		MAP_HANDLE newProperties = ConstMap_CloneWriteable(properties);
		if (newProperties == NULL)
		{
			/*Codes_SRS_IDMAP_17_050: [ If ConstMap_CloneWriteable fails, IdentityMap_Receive shall deallocate any resources and return. ]*/
			LogError("Could not make writeable new properties map");
		}
		else
		{
			char homeIdText[10];
			/*Codes_SRS_IDMAP_17_051: [ IdentityMap_Receive shall call Map_AddOrUpdate with key of "networkId" "homeId" and value of found networkId / homeId. ]*/
			if (sprintf_s(homeIdText, sizeof(homeIdText), "%02x", match->zwaveNetworkId) < 0)
			{
				LogError("Can't read ZWave NodeID");
			}
			else if (Map_AddOrUpdate(newProperties, GW_ZWAVE_HOMEID_PROPERTY, homeIdText) != MAP_OK)
			{
				LogError("Could not attach %s property to message", GW_ZWAVE_HOMEID_PROPERTY);
			}
			else if (Map_AddOrUpdate(newProperties, GW_ZWAVE_NODEID_PROPERTY, match->zwaveNodeId) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_052: [ If adding nodeID fails, IdentityMap_Receive shall deallocate all resources and return. ]*/
				LogError("Could not attach %s property to message", GW_ZWAVE_NODEID_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_032: [IdentityMap_Receive shall call Map_AddOrUpdate with key of "source" and value of "mapping".]*/
			else if (Map_AddOrUpdate(newProperties, GW_SOURCE_PROPERTY, GW_ZWAVEIDMAP_MODULE) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_033: [If adding source fails, IdentityMap_Receive shall deallocate all resources and return.]*/
				LogError("Could not attach %s property to message", GW_SOURCE_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_055: [ IdentityMap_Receive shall call Map_Delete to remove the "deviceName" property. ]*/
			else if (Map_Delete(newProperties, GW_DEVICENAME_PROPERTY) != MAP_OK)
			{
				/*Codes_SRS_IDMAP_17_056: [ If deleting the device name fails, IdentityMap_Receive shall deallocate all resources and return. ]*/
				LogError("Could not remove %s property from message", GW_DEVICENAME_PROPERTY);
			}
			/*Codes_SRS_IDMAP_17_057: [ IdentityMap_Receive shall call Map_Delete to remove the "deviceKey" property. ]*/
			else if (Map_Delete(newProperties, GW_DEVICEKEY_PROPERTY) == MAP_INVALIDARG)
			{
				/*Codes_SRS_IDMAP_17_058: [ If deleting the device key does not return MAP_OK or MAP_KEYNOTFOUND, IdentityMap_Receive shall deallocate all resources and return. ]*/
				LogError("Could not remove %s property from message", GW_DEVICEKEY_PROPERTY);
			}
			else
			{
				publish_with_new_properties(newProperties, messageHandle, idModule);
			}
			Map_Destroy(newProperties);
		}
		ConstMap_Destroy(properties);
	}
}



int compareints(const void * a, const void * b)
{
	return (*(int*)a - *(int*)b);
}
/*
* @brief    Receive a message from the message broker.
*/
static void IdentityMap_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	if (moduleHandle == NULL || messageHandle == NULL)
	{
		/*Codes_SRS_IDMAP_17_020: [If moduleHandle or messageHandle is NULL, then the function shall return.]*/
		LogError("Received NULL arguments: module = %p, massage = %p", moduleHandle, messageHandle);
	}
	else
	{
		ZWAVE_IDENTITY_MAP_DATA * idModule = (ZWAVE_IDENTITY_MAP_DATA*)moduleHandle;

		CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);

		const char * source = ConstMap_GetValue(properties, GW_SOURCE_PROPERTY);
		bool isC2DMessage;
		if (determine_message_direction(source, &isC2DMessage))
		{
			if (isC2DMessage == true)
			{
				const char * deviceName = ConstMap_GetValue(properties, GW_DEVICENAME_PROPERTY);
				/*Codes_SRS_IDMAP_17_045: [ If messageHandle properties does not contain "deviceName" property, then the message shall not be marked as a C2D message. */
				if (deviceName != NULL)
				{
					const ZWAVE_IDENTITY_MAP_CONFIG key = {0,NULL,deviceName,NULL };
					
					ZWAVE_IDENTITY_MAP_CONFIG * match = (ZWAVE_IDENTITY_MAP_CONFIG *) bsearch(&key,
						idModule->devIdToZWaveArray, idModule->mappingSize,
						sizeof(ZWAVE_IDENTITY_MAP_CONFIG),
						IdentityMapConfig_IdCompare);

					if (match == NULL)
					{
						/*Codes_SRS_IDMAP_17_048: [ If the deviceName of the message is not found in deviceToMacArray, then the message shall not be marked as a C2D message. ]*/
						LogInfo("Did not find device Id [%s] of current message", deviceName);
					}
					else
					{
						IdentityMap_RepublishC2D(idModule, messageHandle, match);
					}
				}
			}
			else
			{
				unsigned int networkId = 0;
				
				
				
				const char * charMessageNetworkId = ConstMap_GetValue(properties, GW_ZWAVE_HOMEID_PROPERTY);				
				const char * messageNodeId = IdentityMapConfig_ToUpperCase(ConstMap_GetValue(properties, GW_ZWAVE_NODEID_PROPERTY));

				/*Codes_SRS_IDMAP_17_021: [If messageHandle properties does not contain "macAddress" property, then the function shall return.]*/
				if (charMessageNetworkId != NULL && messageNodeId != NULL)
				{

					if (sscanf(charMessageNetworkId, "%08x", &networkId) < 0) {
						LogInfo("Wrong zwave network ID [%s] in the message", charMessageNetworkId);
					} 
					else if ((ConstMap_GetValue(properties, GW_DEVICENAME_PROPERTY) == NULL ||
						ConstMap_GetValue(properties, GW_DEVICEKEY_PROPERTY) == NULL))
					{						

							ZWAVE_IDENTITY_MAP_CONFIG key = {networkId, messageNodeId,NULL,NULL };

							ZWAVE_IDENTITY_MAP_CONFIG * match = (ZWAVE_IDENTITY_MAP_CONFIG *) bsearch(&key,
								idModule->zwaveToDevIdArray, idModule->mappingSize,
								sizeof(ZWAVE_IDENTITY_MAP_CONFIG),
								IdentityMapConfig_ZWaveAddressCompare);

							if (match == NULL)
							{
								/*Codes_SRS_IDMAP_17_025: [If the macAddress of the message is not found in the macToDeviceArray list, then this function shall return.]*/
								LogInfo("Did not find message NetworkId %d,  NodeId: %s", networkId, messageNodeId);
							}
							else
							{
								IdentityMap_RepublishD2C(idModule, messageHandle, match);
							}						
					}
					/*Codes_SRS_IDMAP_17_039: [IdentityMap_Receive will destroy all resources it created.]*/
					free((void*)messageNodeId);
				}
			}
		}
		ConstMap_Destroy(properties);

	}
}

/*
* @brief    Create an identity map module.
*/
static MODULE_HANDLE IdentityMap_Create(BROKER_HANDLE broker, const void* configuration)
{
	ZWAVE_IDENTITY_MAP_DATA* result;
	if (broker == NULL || configuration == NULL)
	{
		/*Codes_SRS_IDMAP_17_004: [If the broker is NULL, this function shall fail and return NULL.]*/
		/*Codes_SRS_IDMAP_17_005: [If the configuration is NULL, this function shall fail and return NULL.]*/
		LogError("invalid parameter (NULL).");
		result = NULL;
	}
	else
	{
		VECTOR_HANDLE mappingVector = (VECTOR_HANDLE)configuration;
		if (IdentityMap_ValidateConfig(mappingVector) == false)
		{
			LogError("unable to validate mapping table");
			result = NULL;
		}
		else
		{
			result = (ZWAVE_IDENTITY_MAP_DATA*)malloc(sizeof(ZWAVE_IDENTITY_MAP_DATA));
			if (result == NULL)
			{
				/*Codes_SRS_IDMAP_17_010: [If IdentityMap_Create fails to allocate a new ZWAVE_IDENTITY_MAP_DATA structure, then this function shall fail, and return NULL.]*/
				LogError("Could not Allocate Module");
			}
			else
			{
				size_t mappingSize = VECTOR_size(mappingVector);
				/* validation ensures the vector is greater than zero */
				result->zwaveToDevIdArray = (ZWAVE_IDENTITY_MAP_CONFIG*)malloc(mappingSize * sizeof(ZWAVE_IDENTITY_MAP_CONFIG));
				if (result->zwaveToDevIdArray == NULL)
				{
					/*Codes_SRS_IDMAP_17_011: [If IdentityMap_Create fails to create memory for the macToDeviceArray, then this function shall fail and return NULL.*/
					LogError("Could not allocate mac to device mapping table");
					free(result);
					result = NULL;
				}
				else
				{
					result->devIdToZWaveArray = (ZWAVE_IDENTITY_MAP_CONFIG*)malloc(mappingSize * sizeof(ZWAVE_IDENTITY_MAP_CONFIG));
					if (result->devIdToZWaveArray == NULL)
					{
						/*Codes_SRS_IDMAP_17_042: [ If IdentityMap_Create fails to create memory for the deviceToMacArray, then this function shall fail and return NULL. */
						LogError("Could not allocate devicee to mac mapping table");
						free(result->zwaveToDevIdArray);
						free(result);
						result = NULL;
					}
					else
					{

						size_t index;
						size_t failureIndex = mappingSize;
						for (index = 0; index < mappingSize; index++)
						{
							ZWAVE_IDENTITY_MAP_CONFIG * element = (ZWAVE_IDENTITY_MAP_CONFIG *)VECTOR_element(mappingVector, index);
							ZWAVE_IDENTITY_MAP_CONFIG * dest = &(result->zwaveToDevIdArray[index]);
							ZWAVE_IDENTITYMAP_RESULT copyResult;
							copyResult = IdentityMapConfig_CopyDeep(dest, element);
							if (copyResult != IDENTITYMAP_OK)
							{
								failureIndex = index;
								break;
							}
							dest = &(result->devIdToZWaveArray[index]);
							copyResult = IdentityMapConfig_CopyDeep(dest, element);
							if (copyResult != IDENTITYMAP_OK)
							{
								IdentityMapConfig_Free(&(result->zwaveToDevIdArray[index]));
								failureIndex = index;
								break;
							}
						}
						if (failureIndex < mappingSize)
						{
							/*Codes_SRS_IDMAP_17_012: [If IdentityMap_Create fails to add a MAC address triplet to the macToDeviceArray, then this function shall fail, release all resources, and return NULL.]*/
							/*Codes_SRS_IDMAP_17_043: [ If IdentityMap_Create fails to add a MAC address triplet to the deviceToMacArray, then this function shall fail, release all resources, and return NULL. */
							for (index = 0; index < failureIndex; index++)
							{
								IdentityMapConfig_Free(&(result->zwaveToDevIdArray[index]));
								IdentityMapConfig_Free(&(result->devIdToZWaveArray[index]));
							}
							free(result->zwaveToDevIdArray);
							free(result->devIdToZWaveArray);
							free(result);
							result = NULL;
						}
						else
						{
							/*Codes_SRS_IDMAP_17_003: [Upon success, this function shall return a valid pointer to a MODULE_HANDLE.]*/
							qsort(result->zwaveToDevIdArray, mappingSize, sizeof(ZWAVE_IDENTITY_MAP_CONFIG),
								IdentityMapConfig_ZWaveAddressCompare);
							qsort(result->devIdToZWaveArray, mappingSize, sizeof(ZWAVE_IDENTITY_MAP_CONFIG),
								IdentityMapConfig_IdCompare);
							result->mappingSize = mappingSize;
							result->broker = broker;
						}
					}
				}
			}
		}
	}
	return result;
}

/*
* @brief    Parse configuration for identity map module.
*/
static void * IdentityMap_ParseConfigurationFromJson(const char* configuration)
{
	VECTOR_HANDLE result;
	if (configuration == NULL)
	{
		/*Codes_SRS_IDMAP_05_004: [ If configuration is NULL then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
		LogError("Invalid NULL configuration parameter");
		result = NULL;
	}
	else
	{
		/*Codes_SRS_IDMAP_05_006: [ IdentityMap_ParseConfigurationFromJson shall parse the configuration as a JSON array of objects. ]*/
		JSON_Value* json = json_parse_string((const char*)configuration);
		if (json == NULL)
		{
			/*Codes_SRS_IDMAP_05_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
			LogError("Unable to parse json string");
			result = NULL;
		}
		else
		{
			/*Codes_SRS_IDMAP_05_006: [ IdentityMap_ParseConfigurationFromJson shall parse the configuration as a JSON array of objects. ]*/
			JSON_Array *jsonArray = json_value_get_array(json);
			if (jsonArray == NULL)
			{
				/*Codes_SRS_IDMAP_05_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
				LogError("Expected a JSON Array in configuration");
				result = NULL;
			}
			else
			{
				/*Codes_SRS_IDMAP_17_060: [ IdentityMap_ParseConfigurationFromJson shall allocate memory for the configuration vector. ]*/
				/*Codes_SRS_IDMAP_05_007: [ IdentityMap_ParseConfigurationFromJson shall call VECTOR_create to make the identity map module input vector. ]*/
				result = VECTOR_create(sizeof(ZWAVE_IDENTITY_MAP_CONFIG));
				if (result == NULL)
				{
					//Codes_SRS_IDMAP_17_061: [ If allocation fails, IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]
					/*Codes_SRS_IDMAP_05_019: [ If creating the vector fails, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
					LogError("Failed to create the input vector");
					result = NULL;
				}
				else
				{
					size_t numberOfRecords = json_array_get_count(jsonArray);
					size_t record;
					bool arrayParsed = true;
					/*Codes_SRS_IDMAP_05_008: [ IdentityMap_ParseConfigurationFromJson shall walk through each object of the array. ]*/
					for (record = 0; record < numberOfRecords; record++)
					{
						/*Codes_SRS_IDMAP_05_006: [ IdentityMap_ParseConfigurationFromJson shall parse the configuration as a JSON array of objects. ]*/
						if (addOneRecord(result, json_array_get_object(jsonArray, record)) != true)
						{
							arrayParsed = false;
							break;
						}
					}
					if (arrayParsed != true)
					{
						numberOfRecords = VECTOR_size(result);
						for (record = 0; record < numberOfRecords; record++)
						{
							ZWAVE_IDENTITY_MAP_CONFIG *element = (ZWAVE_IDENTITY_MAP_CONFIG *)VECTOR_element(result, record);
							IdentityMapConfig_Free(element);
						}
						VECTOR_destroy(result);
						/*Codes_SRS_IDMAP_05_005: [ If configuration is not a JSON array of JSON objects, then IdentityMap_ParseConfigurationFromJson shall fail and return NULL. ]*/
						result = NULL;
					}
					else
					{
						/*Codes_SRS_IDMAP_17_062: [ IdentityMap_ParseConfigurationFromJson shall return the pointer to the configuration vector on success. ]*/
					}
				}
			}
			json_value_free(json);
		}
	}
	return result;
}


static void IdentityMap_FreeConfiguration(void * configuration)
{
	/*Codes_SRS_IDMAP_17_059: [ IdentityMap_FreeConfiguration shall do nothing if configuration is NULL. ]*/
	if (configuration != NULL)
	{
		VECTOR_HANDLE map_vector = (VECTOR_HANDLE)configuration;
		size_t map_size = VECTOR_size(map_vector);
		size_t record;
		for (record = 0; record < map_size; record++)
		{
			/*Codes_SRS_IDMAP_05_016: [ IdentityMap_FreeConfiguration shall release all data IdentityMap_ParseConfigurationFromJson allocated. ]*/
			ZWAVE_IDENTITY_MAP_CONFIG * element = (ZWAVE_IDENTITY_MAP_CONFIG *)VECTOR_element(map_vector, record);
			IdentityMapConfig_Free(element);
		}
		VECTOR_destroy(map_vector);
	}
}
/*
* @brief    Destroy an identity map module.
*/
static void IdentityMap_Destroy(MODULE_HANDLE moduleHandle)
{
	/*Codes_SRS_IDMAP_17_018: [If moduleHandle is NULL, IdentityMap_Destroy shall return.]*/
	if (moduleHandle != NULL)
	{
		/*Codes_SRS_IDMAP_17_015: [IdentityMap_Destroy shall release all resources allocated for the module.]*/
		ZWAVE_IDENTITY_MAP_DATA * idModule = (ZWAVE_IDENTITY_MAP_DATA*)moduleHandle;
		for (size_t index = 0; index < idModule->mappingSize; index++)
		{
			IdentityMapConfig_Free(&(idModule->zwaveToDevIdArray[index]));
			IdentityMapConfig_Free(&(idModule->devIdToZWaveArray[index]));
		}
		free(idModule->zwaveToDevIdArray);
		free(idModule->devIdToZWaveArray);
		free(idModule);
	}
}
/*
*    Required for all modules:  the public API and the designated implementation functions.
*/
static const MODULE_API_1 IdentityMap_APIS_all =
{
	{ MODULE_API_VERSION_1 },

	IdentityMap_ParseConfigurationFromJson,
	IdentityMap_FreeConfiguration,
	IdentityMap_Create,
	IdentityMap_Destroy,
	IdentityMap_Receive,
	NULL
};

/*Codes_SRS_IDMAP_26_001: [ `Module_GetApi` shall return a pointer to `MODULE_API` structure. ]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(ZWAVE_IDENTITYMAP_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
	(void)gateway_api_version;
	return (const MODULE_API *)&IdentityMap_APIS_all;
}
