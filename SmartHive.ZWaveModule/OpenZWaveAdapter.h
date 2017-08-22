#pragma once
//#include <pthread.h>
#include "Manager.h"
#include "Defs.h"
#include "value_classes/ValueID.h"
#include "ZWaveModule.h"

using namespace OpenZWave;
/*All logic for communication with open zwave stack*/
typedef struct
{
	uint32			m_homeId;
	uint8			m_nodeId;
	bool			m_polled;
	list<ValueID>	m_values;
}NodeInfo;

//static pthread_cond_t  initCond = PTHREAD_COND_INITIALIZER;
static uint32 g_homeId = 0;
static bool   g_initFailed = false;
static list<NodeInfo*> g_nodes;

//pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

class OpenZWaveAdapter
{
	pthread_mutex_t* g_criticalSection;

public:
	OpenZWaveAdapter(ZWAVEDEVICE_DATA* module_data);
	~OpenZWaveAdapter();
	static void OnNotification(Notification const* _notification, void* _context);
	void Start();	
private:
	ZWAVEDEVICE_DATA* module_handle;	
	
};

