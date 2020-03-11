# include <Arduino.h>
#include "../lib/WAdapter/WAdapter/WNetwork.h"
#include "WBecaDevice.h"
#include "WClock.h"

#define APPLICATION "Thermostat Beca"
#define VERSION "1.01beta"
#define DEBUG false

WNetwork* network;
WBecaDevice* becaDevice;
WClock* wClock;

void setup() {
	Serial.begin(9600);
	//Wifi and Mqtt connection
	network = new WNetwork(DEBUG, APPLICATION, VERSION, true, NO_LED);
	network->setOnNotify([]() {
		if (network->isWifiConnected()) {

		}
		if (network->isMqttConnected()) {
			becaDevice->queryState();
			if (becaDevice->isDeviceStateComplete()) {
				//sendMqttStatus();
			}
		}
	});
	network->setOnConfigurationFinished([]() {
		//Switch blinking thermostat in normal operating mode back
		becaDevice->cancelConfiguration();
	});
	//KaClock - time sync
	wClock = new WClock(network, APPLICATION);
	network->addDevice(wClock);
	wClock->setOnTimeUpdate([]() {
		becaDevice->sendActualTimeToBeca();
	});
	wClock->setOnError([](const char* error) {
		String t = (String)network->getMqttTopic()+ "/error" ;
		return network->publishMqtt( t.c_str(), "message", error);
	});
	//Communication between ESP and Beca-Mcu
	becaDevice = new WBecaDevice(network, wClock);
	network->addDevice(becaDevice);

	becaDevice->setOnSchedulesChange([]() {
		//Send schedules once at ESP start and at every change
		return true;// sendSchedulesViaMqtt();
	});
	becaDevice->setOnNotifyCommand([](const char* commandType) {
		String t = (String)network->getMqttTopic()+ "/mcucommand" ;
		return network->publishMqtt(t.c_str(), commandType, becaDevice->getCommandAsString().c_str());
	});
	becaDevice->setOnConfigurationRequest([]() {
		network->startWebServer();
		return true;
	});
}

void loop() {
	network->loop(millis());
	delay(50);
}

