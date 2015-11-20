#ifndef MQTT_IFACE_H
#define MQTT_IFACE_H

#include <mosquittopp.h>
#include <iostream>
#include <vector>
#include <functional>
#include <stdint.h>

#define CONFIG_TOPIC "BeeeOn/config"
#define DATA_TO_TOPIC "BeeeOn/data_to"
#define DATA_FROM_TOPIC "BeeeOn/data_from"

static int test = 5;

using namespace std;

class mqtt_iface : public mosqpp::mosquittopp
{
	public:
	mqtt_iface(const char *id, const char *host, int port);
	~mqtt_iface();

	void on_connect(int rc);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
	void on_message(const struct mosquitto_message *message);
	bool send_message(const char *topic, const char * msg, uint8_t len);

};
extern mqtt_iface *mq;
#endif
