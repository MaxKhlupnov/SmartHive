#include "OpenZWaveAdapter.h"
#include "azure_c_shared_utility/threadapi.h"
#include <unistd.h>
#include <stdlib.h>

#include "Options.h"

#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"

#include "value_classes/ValueBool.h"
#include "platform/Log.h"
#include "azure_c_shared_utility/xlogging.h"
#include "broker.h"
#include "messageproperties.h"

using namespace OpenZWave;

/*All logic for communication with open zwave stack*/

OpenZWaveAdapter::OpenZWaveAdapter(ZWAVEDEVICE_DATA* module_data)
{
	this->module_handle = module_data;
}


OpenZWaveAdapter::~OpenZWaveAdapter()
{
	string port = this->module_handle->controllerPath;//"/dev/tty1";
	// program exit (clean up)
	if (strcasecmp(port.c_str(), "usb") == 0)
	{
		Manager::Get()->RemoveDriver("HID Controller");
	}
	else
	{
		Manager::Get()->RemoveDriver(port);
	}
	Manager::Get()->RemoveWatcher(this->OnNotification, this);
	Manager::Destroy();
	Options::Destroy();
}

void OpenZWaveAdapter::Start() {

	//pthread_mutex_lock(g_criticalSection);

	printf("Starting SmartHive.ZWaveModule with OpenZWave Version %s\n", Manager::getVersionAsString().c_str());

	// Create the OpenZWave Manager.
	// The first argument is the path to the config files (where the manufacturer_specific.xml file is located
	// The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL 
	// the log file will appear in the program's working directory.
	Options::Create("../../../../open-zwave/config/", "", "");
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

	// Add a Z-Wave Driver
	// Modify this line to set the correct serial port for your PC interface.
	string port = this->module_handle->controllerPath;//"/dev/tty1";

	if (strcasecmp(port.c_str(), "usb") == 0)
	{
		Manager::Get()->AddDriver("HID Controller", Driver::ControllerInterface_Hid);
	}
	else
	{		
		Manager::Get()->AddDriver(port);
	}
	
	//pthread_mutex_unlock(g_criticalSection);
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

	LogInfo("New notification %s from node: %d", _notification->GetAsString().c_str(), _notification->GetNodeId());

	OpenZWaveAdapter* context = (OpenZWaveAdapter*)_context;
		// Must do this inside a critical section to avoid conflicts with the main thread
		pthread_mutex_lock(context->g_criticalSection);
		sent_message(context->module_handle, _notification);
		pthread_mutex_unlock(context->g_criticalSection);
	
}


static int sent_message(ZWAVEDEVICE_DATA* handleData, Notification const* _notification)
{
	
	MESSAGE_CONFIG msgConfig;
	MAP_HANDLE propertiesMap = Map_Create(NULL);
	if (propertiesMap == NULL)
	{
		LogError("unable to create a Map");
	}
	else
	{
		if (Map_AddOrUpdate(propertiesMap, "helloWorld", _notification->GetAsString().c_str()) != MAP_OK)
		{
			LogError("unable to Map_AddOrUpdate");
		}
		else
		{
			const char* msgText = _notification->GetAsString().c_str();

			msgConfig.size = (size_t)strlen(msgText);
			msgConfig.source = (unsigned char*)msgText;

			msgConfig.sourceProperties = propertiesMap;

			MESSAGE_HANDLE helloWorldMessage = Message_Create(&msgConfig);
			if (helloWorldMessage == NULL)
			{
				LogError("unable to create \"hello world\" message");
			}
			else
			{
				
				(void)Broker_Publish(handleData->broker, (MODULE_HANDLE)handleData, helloWorldMessage);
				//(void)Unlock(handleData->lockHandle);			
				Message_Destroy(helloWorldMessage);
			}
		}
	}
	return 0;
}
