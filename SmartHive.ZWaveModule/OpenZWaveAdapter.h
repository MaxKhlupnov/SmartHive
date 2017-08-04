#pragma once
#include <pthread.h>
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

class OpenZWaveAdapter
{
public:
	OpenZWaveAdapter(ZWAVEDEVICE_DATA* module_data);
	~OpenZWaveAdapter();
	void OnNotification(Notification const* _notification, void* _context);
	void Start();
private:
	ZWAVEDEVICE_DATA* module_handle;
	pthread_mutex_t g_criticalSection;
	pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;
};

