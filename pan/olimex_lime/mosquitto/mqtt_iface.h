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

using namespace std;

class mqtt_iface : public mosqpp::mosquittopp
{
	public:
	mqtt_iface(const char *id, const char *host, int port, function<void(const uint8_t*, const uint8_t*, uint8_t len)> send_cb);
	~mqtt_iface();

	void on_connect(int rc);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
	void on_message(const struct mosquitto_message *message);
	bool send_message(const char *topic, const char * msg, uint8_t len);
	void process_sensor_msg(const uint8_t from_edid[4], const uint8_t* data, const uint8_t len);

	private:
	void proccess_adapter_msg(string msg);
	function<void(const uint8_t*, const uint8_t*, uint8_t len)> send_cbp;
	void print_values(vector<int> v);
};

#ifdef X86
extern mqtt_iface *mq;
#endif

#endif
