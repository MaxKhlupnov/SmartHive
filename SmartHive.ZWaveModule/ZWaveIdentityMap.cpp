
#include <stddef.h>
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

DEFINE_ENUM(ZWAVE_IDENTITYMAP_RESULT, IDENTITYMAP_RESULT_VALUES)

DEFINE_ENUM_STRINGS(BROKER_RESULT, BROKER_RESULT_VALUES);


#define DEVICENAME "deviceId"
#define DEVICEKEY "deviceKey"

static ZWAVE_IDENTITYMAP_RESULT IdentityMapConfig_CopyDeep(ZWAVE_IDENTITY_MAP_CONFIG * dest, ZWAVE_IDENTITY_MAP_CONFIG * source);
static void IdentityMapConfig_Free(ZWAVE_IDENTITY_MAP_CONFIG * element);