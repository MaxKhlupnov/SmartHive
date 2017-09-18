#include "OpenZWaveAdapter.h"
#include "azure_c_shared_utility/threadapi.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "Options.h"

#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"
#include "Utils.h"

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
	Options::Create(this->module_handle->zwaveConfigPath, "", "");
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

		if (_notification == NULL || context->module_handle == NULL) {
			LogError("Incorrect sent_message function call parameter values");
			return;
		}

		//if (_notification->GetType() != OpenZWave::Notification::NotificationType::Type_ValueChanged) {
		if (allowSendNotificationOfThisType(context->module_handle, _notification)) {
			LogInfo("Notification type %s skipped for sending", _notification->GetAsString().c_str());
			return;
		}

		// Must do this inside a critical section to avoid conflicts with the main thread
		pthread_mutex_lock(context->g_criticalSection);		

			SentMessage(context->module_handle, _notification);

		pthread_mutex_unlock(context->g_criticalSection);
	
}

bool OpenZWaveAdapter::allowSendNotificationOfThisType(ZWAVEDEVICE_DATA* handleData, Notification const* _notification) {
		
	// TODO: FILTER NOTIFICATION FOR SENDING BASED IN CONFIG VALUE handleData->sendNotificationOfType
	return (_notification->GetType() != OpenZWave::Notification::NotificationType::Type_ValueChanged);

}

 int OpenZWaveAdapter::SentMessage(ZWAVEDEVICE_DATA* handleData, Notification const* _notification)
{

	MESSAGE_CONFIG msgConfig;
	MAP_HANDLE propertiesMap = Map_Create(NULL);
	if (propertiesMap == NULL)
	{
		LogError("unable create a Map");
	}
	else
	{
		if (Map_AddOrUpdate(propertiesMap, GW_SOURCE_PROPERTY, GW_SOURCE_ZWAVE_TELEMETRY) != MAP_OK)
		{
			LogError("unable to Map_AddOrUpdate %s", GW_SOURCE_PROPERTY);
		}		
		else
		{
			char homeIdText[10];
			char nodeIdText[3];
			if (sprintf_s(homeIdText, sizeof(homeIdText), "%08x", _notification->GetHomeId()) < 0)
			{
				LogError("Can't read ZWave HomeID");
			}
			else if (Map_AddOrUpdate(propertiesMap, GW_ZWAVE_HOMEID_PROPERTY, homeIdText) != MAP_OK)
			{
				LogError("unable to Map_AddOrUpdate %s", GW_ZWAVE_HOMEID_PROPERTY);
			}
			else if (sprintf_s(nodeIdText, sizeof(nodeIdText), "%02x", _notification->GetNodeId()) < 0)
			{
				LogError("Can't read ZWave NodeID");
			}
			else if (Map_AddOrUpdate(propertiesMap, GW_ZWAVE_NODEID_PROPERTY, nodeIdText) != MAP_OK)
			{
				LogError("unable to Map_AddOrUpdate %s", GW_ZWAVE_NODEID_PROPERTY);
			}
			/*
			Useless until we add other type than NotificationType::Type_ValueChanged
			else if (Map_AddOrUpdate(propertiesMap, GW_ZWAVE_NOTIFICATION_TYPE_PROPERTY, _notification->GetAsString().c_str()) != MAP_OK)
			{
				LogError("unable to Map_AddOrUpdate %s", GW_ZWAVE_NOTIFICATION_TYPE_PROPERTY);
			}*/
			else{

				//["DeviceId":"7_3454969560", "Time": "2016-10-03T05:18:22.5572932", "ValueLabel": "Battery Level", "Type": "Byte", "ValueUnits": "%", "Value": 67.0]

				ValueID const& valId = _notification->GetValueID();


				auto t = std::time(nullptr);
				struct tm * tm = std::localtime(&t);
				
				char cTimeBuffer[80];
				strftime(cTimeBuffer, 80, "%Y-%m-%dT%H:%M:%S", tm);				
				string sTimeBuffer(cTimeBuffer);

				char msgText[500];
				


				if (sprintf_s(msgText, sizeof(msgText), "{\"DeviceId\":\"%d_%u\",\"Time\":\"%s\",\"ValueLabel\":\"%s\",\"Type\":\"%s\",\"ValueUnits\":\"%s\",\"Value\":%s}",
					_notification->GetNodeId(), _notification->GetHomeId(),
					sTimeBuffer.c_str(),
					Manager::Get()->GetValueLabel(valId).c_str(), ValueTypeToString(valId).c_str(), 
					Manager::Get()->GetValueUnits(valId).c_str(), ValueToString(valId).c_str()
					) < 0)
				{
					LogError("Failed to set message text");
				}
				else {
						LogInfo(msgText);			
						msgConfig.size = (size_t)strlen(msgText);
						msgConfig.source = (unsigned char*)msgText;
				}
				msgConfig.sourceProperties = propertiesMap;

				MESSAGE_HANDLE zWaveMessage = Message_Create(&msgConfig);
				if (zWaveMessage == NULL)
				{
					LogError("unable to create \"zWave Notification\" message");
				}
				else
				{

					(void)Broker_Publish(handleData->broker, (MODULE_HANDLE)handleData, zWaveMessage);
					//(void)Unlock(handleData->lockHandle);			
					Message_Destroy(zWaveMessage);
				}
			}
		}
	}
	Map_Destroy(propertiesMap);
	return 0;
}

string OpenZWaveAdapter::ValueToString(ValueID const& valueID) {
	 string str;	
	 if (!Manager::Get()->GetValueAsString(valueID, &str)) {
		 LogError("Error reading value type of String");
		 return "";
	 }
	 else {
		 if (valueID.GetType() == ValueID::ValueType::ValueType_String) {
			 return '\"' + str + '\"';
		 }
		 else {
			 return str;
		 }
	 }
	 
 }


string OpenZWaveAdapter::ValueTypeToString(ValueID const& valueID) {
	 
		 string str;
		 switch (valueID.GetType()) {
			case ValueID::ValueType::ValueType_Bool:
				str = "Bool";
				break;
			case ValueID::ValueType::ValueType_Byte:
				str = "Byte";
				break;
			case ValueID::ValueType::ValueType_Decimal:
				str = "Decimal";
				break;
			case 	ValueID::ValueType::ValueType_Int:
				str = "Int";
				break;
			case 	ValueID::ValueType::ValueType_List:
				str = "List";
				break;
			case 	ValueID::ValueType::ValueType_Schedule:
				str = "Schedule";
				break;
			case 	ValueID::ValueType::ValueType_Short:
				str = "Short";
				break;
			case 	ValueID::ValueType::ValueType_Button:
					str = "Button";
					break;
			case ValueID::ValueType::ValueType_String:
				 str = "String";
				 break;
			case ValueID::ValueType::ValueType_Raw:
				str = "Raw";
				break;
		 }
		 return str;
 }