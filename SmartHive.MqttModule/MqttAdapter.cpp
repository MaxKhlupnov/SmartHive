#include "azure_c_shared_utility/xlogging.h"

#include "mqtt/async_client.h"
#include "MqttModule.h"
#include "MqttAdapter.h"

/**
* A callback class for use with the main MQTT client.
*/
class callback : public virtual mqtt::callback
{
public:
	void connected(const mqtt::string& cause)  override {
		LogInfo("Mqtt broker connected", cause);
	}
	void connection_lost(const mqtt::string& cause) override {

		LogError("Mqtt broker connection lost", cause);
	}

	void delivery_complete(mqtt::delivery_token_ptr tok) override {
	}

};


MqttAdapter::MqttAdapter(MQTT_CONFIG* config)
{
	this->config = config;
	int serverURILen = printf("%s:%d", this->config->mqttBrokerAddress, config->mqttBrokerPort);
	this->serverURI = new char[++serverURILen];
	
	if (sprintf_s(this->serverURI, sizeof(this->serverURI), "%s:%d", config->mqttBrokerAddress, config->mqttBrokerPort) < 0) {
		LogError("Error formatting server URI");
		delete [] this->serverURI;
	}

	InitMqttClient();
}


MqttAdapter::~MqttAdapter()
{
	if (client != NULL){
		if (client->is_connected()) {
			client->disconnect();			
		}
		free(client);
	}
}

void MqttAdapter::InitMqttClient()
{	
	LogInfo("Initializing for server %s", serverURI);

	client = new mqtt::async_client(serverURI, config->clientId);

	callback cb;
	client->set_callback(cb);

	mqtt::connect_options conopts;

}
