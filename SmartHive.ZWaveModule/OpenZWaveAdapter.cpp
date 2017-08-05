#include "OpenZWaveAdapter.h"

#include <unistd.h>
#include <stdlib.h>

#include "Options.h"

#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"

#include "value_classes/ValueBool.h"
#include "platform/Log.h"

#include "broker.h"

using namespace OpenZWave;

/*All logic for communication with open zwave stack*/

OpenZWaveAdapter::OpenZWaveAdapter(ZWAVEDEVICE_DATA* module_data)
{
	this->module_handle = module_data;
}


OpenZWaveAdapter::~OpenZWaveAdapter()
{
}

void OpenZWaveAdapter::Start() {
	pthread_mutexattr_t mutexattr;

	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&this->g_criticalSection, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	pthread_mutex_lock(&this->initMutex);

	printf("Starting SmartHive.ZWaveModule with OpenZWave Version %s\n", Manager::getVersionAsString().c_str());

	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	Options::Create("../../../config/", "", "");
	Options::Get()->AddOptionInt("SaveLogLevel", LogLevel_Detail);
	Options::Get()->AddOptionInt("QueueLogLevel", LogLevel_Debug);
	Options::Get()->AddOptionInt("DumpTrigger", LogLevel_Error);
	Options::Get()->AddOptionInt("PollInterval", 500);
	Options::Get()->AddOptionBool("IntervalBetweenPolls", true);
	Options::Get()->AddOptionBool("ValidateValueChanges", true);
	Options::Get()->Lock();

	Manager::Create();

	// Add a callback handler to the manager.  The second argument is a context that
	// is passed to the OnNotification method.  If the OnNotification is a method of
	// a class, the context would usually be a pointer to that class object, to
	// avoid the need for the notification handler to be a static.
	Manager::Get()->AddWatcher(this->OnNotification, this);

	//TODO: read port from JSON config file
	// Add a Z-Wave Driver
	// Modify this line to set the correct serial port for your PC interface.

#ifdef DARWIN
	string port = "/dev/cu.usbserial";
#elif WIN32
	string port = "\\\\.\\COM6";
#else
	string port = this->module_handle->controllerPath;//"/dev/tty1";
#endif

	if (strcasecmp(port.c_str(), "usb") == 0)
	{
		Manager::Get()->AddDriver("HID Controller", Driver::ControllerInterface_Hid);
	}
	else
	{		
		Manager::Get()->AddDriver(port);
	}

	// Now we just wait for either the AwakeNodesQueried or AllNodesQueried notification,
	// then write out the config file.
	// In a normal app, we would be handling notifications and building a UI for the user.
	pthread_cond_wait(&initCond, &initMutex);

	// Since the configuration file contains command class information that is only 
	// known after the nodes on the network are queried, wait until all of the nodes 
	// on the network have been queried (at least the "listening" ones) before
	// writing the configuration file.  (Maybe write again after sleeping nodes have
	// been queried as well.)
	if (!g_initFailed)
	{

		// The section below demonstrates setting up polling for a variable.  In this simple
		// example, it has been hardwired to poll COMMAND_CLASS_BASIC on the each node that 
		// supports this setting.
		pthread_mutex_lock(&g_criticalSection);
		for (list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it)
		{
			NodeInfo* nodeInfo = *it;

			// skip the controller (most likely node 1)
			if (nodeInfo->m_nodeId == 1) continue;

			printf("NodeID: %d \n ", nodeInfo->m_nodeId);
			printf("\t NodeName: %s \n ", Manager::Get()->GetNodeName(nodeInfo->m_homeId, nodeInfo->m_nodeId).c_str());
			printf("\t ManufacturerName: %s \n ", Manager::Get()->GetNodeManufacturerName(nodeInfo->m_homeId, nodeInfo->m_nodeId).c_str());
			printf("\t NodeProductName: %s \n ", Manager::Get()->GetNodeProductName(nodeInfo->m_homeId, nodeInfo->m_nodeId).c_str());

			printf("Values announced by the nodes without polling: \n");
			for (list<ValueID>::iterator it2 = nodeInfo->m_values.begin(); it2 != nodeInfo->m_values.end(); ++it2)
			{
				ValueID v = *it2;
				printf("\t ValueLabel: %s \n", Manager::Get()->GetValueLabel(v).c_str());
				printf("\t\t ValueType: %d \n", v.GetType());
				printf("\t\t ValueHelp: %s \n", Manager::Get()->GetValueHelp(v).c_str());
				printf("\t\t ValueUnits: %s \n", Manager::Get()->GetValueUnits(v).c_str());
				printf("\t\t ValueMin: %d \n", Manager::Get()->GetValueMin(v));
				printf("\t\t ValueMax: %d \n", Manager::Get()->GetValueMax(v));

				if (v.GetCommandClassId() == COMMAND_CLASS_BASIC)
				{
					//					Manager::Get()->EnablePoll( v, 2 );		// enables polling with "intensity" of 2, though this is irrelevant with only one value polled
					break;
				}
			}
		}
		pthread_mutex_unlock(&g_criticalSection);

		// If we want to access our NodeInfo list, that has been built from all the
		// notification callbacks we received from the library, we have to do so
		// from inside a Critical Section.  This is because the callbacks occur on other 
		// threads, and we cannot risk the list being changed while we are using it.  
		// We must hold the critical section for as short a time as possible, to avoid
		// stalling the OpenZWave drivers.
		// At this point, the program just waits for 3 minutes (to demonstrate polling),
		// then exits
		for (int i = 0; i < 60 * 3; i++)
		{
			pthread_mutex_lock(&g_criticalSection);
			// but NodeInfo list and similar data should be inside critical section
			pthread_mutex_unlock(&g_criticalSection);
			sleep(1);
		}

		Driver::DriverData data;
		Manager::Get()->GetDriverStatistics(g_homeId, &data);
		printf("SOF: %d ACK Waiting: %d Read Aborts: %d Bad Checksums: %d\n", data.m_SOFCnt, data.m_ACKWaiting, data.m_readAborts, data.m_badChecksum);
		printf("Reads: %d Writes: %d CAN: %d NAK: %d ACK: %d Out of Frame: %d\n", data.m_readCnt, data.m_writeCnt, data.m_CANCnt, data.m_NAKCnt, data.m_ACKCnt, data.m_OOFCnt);
		printf("Dropped: %d Retries: %d\n", data.m_dropped, data.m_retries);
	}

	// program exit (clean up)
	if (strcasecmp(port.c_str(), "usb") == 0)
	{
		Manager::Get()->RemoveDriver("HID Controller");
	}
	else
	{
		Manager::Get()->RemoveDriver(port);
	}
	Manager::Get()->RemoveWatcher(OnNotification, NULL);
	Manager::Destroy();
	Options::Destroy();
	pthread_mutex_destroy(&g_criticalSection);
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------
void OpenZWaveAdapter::OnNotification
(
	Notification const* _notification,
	void* _context
)
{


}
