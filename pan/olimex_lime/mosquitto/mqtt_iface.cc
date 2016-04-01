#include "mqtt_iface.h"
#include <mosquittopp.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <functional>
#include <chrono>
#include <thread>
#include "net.h"

#define RX_BUFFER_SIZE 128
#define TIMEOUT 60
#define TOPIC_RX "BeeeOn/data_to_PAN"
#define TOPIC_TX "BeeeOn/data_from_PAN"

#define FROM_SENSOR_MSG 0x92
#define SET_JOIN_MODE 0x93
#define UNPAIR_SENSOR 0xA1
#define MAX_MSG_LEN 32
#define SET_ACTORS 0xB0
#define VPT_ANNOUNCEMENT 0x1A


//mqtt_iface::mqtt_iface(const char *id, const char *host, int port, function<void(void)> send_cb) : mosquittopp(id) {
mqtt_iface::mqtt_iface(const char *id, const char *host, int port, function<void(const uint8_t*, const uint8_t*, uint8_t len)> send_cb) : mosquittopp(id) {
	mosqpp::lib_init();        // Mandatory initialization for mosquitto library
	cout << "Mosquito initialization " << host << ":" << port << endl;
	int rc = connect(host, port, TIMEOUT);
	cout << "RC connect: " << rc << endl;
	send_cbp = send_cb;
	//loop_start();
}

mqtt_iface::~mqtt_iface() {
}

void mqtt_iface::print_values(vector<int> v) {
	int pairs = v[0];
	int byteIndex = 0;
	cout << "DATA PAIRS: " << pairs << endl;
	for (int i=0; i<pairs; i++) {
		cout << "TYPE: " << (v[byteIndex+1] << 8 | v[byteIndex]);
		byteIndex += 2;
		cout << " OFFSET: " << v[++byteIndex];
		int dataSize = v[++byteIndex];
		cout << " SIZE: " << dataSize;
		cout << " DATA: ";
		cout << "0x";
		for (int j=0; j<dataSize; j++) {
			cout << setfill('0') << setw(2) << hex << v[++byteIndex];
		}
		cout << endl;
	}
}

void mqtt_iface::process_sensor_msg(const uint8_t from_edid[4], const uint8_t* data, const uint8_t len) {
	vector<int> v(data, data+len);
	uint8_t src_addr[4];
	int i;
	cout << "SRC ADDRESS: ";
	for(i=0; i<4; i++) {
		src_addr[i] = from_edid[i];
		cout << "0x" << setfill('0') << setw(2) << hex << unsigned(src_addr[i]) << " ";
	}
	cout << endl << "Raw:: ";
	for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
			cout << to_string(*it) + ',';
		}

	cout << endl;
	if (v[0] == FROM_SENSOR_MSG) {
		cout << "FROM_SENSOR_MSG" << endl;
		cout << "PROTO VER: " << (v[1]) << endl;
		cout << "DEVICE ID: " << (v[2] << 8 | v[3]) << endl;
		cout << "PAIRS: " << v[4] << endl;

		vector<int> vecValues = v;
		vecValues.erase(vecValues.begin(), vecValues.begin() + 5);

		string msg2ada;
		msg2ada.clear();
		msg2ada += to_string(FROM_SENSOR_MSG) + ',';  // command
		msg2ada += to_string(v[1]) + ',';  // proto version
		for(i=0; i<4; i++) {
			msg2ada += to_string(src_addr[i]) + ',';  // sensor address , EUID
		}
		msg2ada += to_string(0x64) + ',';  // rssi (hack)
		msg2ada += to_string(v[2]) + ',' + to_string(v[3]) + ','; // Device ID
		msg2ada += to_string(v[4]) + ','; //Pairs
		for(std::vector<int>::iterator it = vecValues.begin(); it != vecValues.end(); it++) {
			msg2ada += to_string(*it) + ',';
		}
		cout << "Length:: " << msg2ada.length() << endl;
		cout << hex << msg2ada << endl;
		publish(NULL, TOPIC_TX, msg2ada.length(), msg2ada.c_str());

	}
	else if (v[0] == VPT_ANNOUNCEMENT) {
		// temporary workaround will be solved by new protocol
		// with device id
		cout << "VPT_ANNOUNCEMENT" << endl;
		string msg2ada;
		msg2ada.clear();
		msg2ada += to_string(FROM_SENSOR_MSG) + ',';  // command
		msg2ada += to_string(0) + ',';  // reserved
		msg2ada += to_string(0) + ',' + to_string(1) + ',';  // proto version
		for(i=0; i<4; i++) {
			msg2ada += to_string(src_addr[i]) + ',';  // sensor address
		}
		msg2ada += to_string(29) + ',' + to_string(3) + ',';  // battery
		msg2ada += to_string(40) + ',' + to_string(35) + ',';  // rssi (hack)
		msg2ada += "17,0,166,0,1,0,0,167,0,1,0,0,165,0,4,0,0,0,0,0,10,0,4,0,0,0,0,0,166,1,1,0,0,167,1,1,0,0,165,1,4,0,0,0,0,0,10,1,4,0,0,0,0,0,166,2,1,0,0,167,2,1,0,0,165,2,4,0,0,0,0,0,10,2,4,0,0,0,0,0,166,3,1,0,0,167,3,1,0,0,165,3,4,0,0,0,0,0,10,3,4,0,0,0,0,0,11,0,1,0,";
		cout << msg2ada << endl;
		publish(NULL, TOPIC_TX, msg2ada.length(), msg2ada.c_str());

	}
	else {
		cout << "UNKNOWN DATA: ";
		for(i=0; i<len; i++) {
			cout << "0x" << setfill('0') << setw(2) << hex << unsigned(data[i]) << " ";
		}
		cout << endl;
	}

//	for(std::vector<int>::iterator it = v.begin(); it != v.end(); it++) {
//              cout << *it << endl;
//	}
}


//extern bool joining_enabled;
void joining_timer_f() {
	cout << "Starting joining timer" << endl;
	NET_joining_enable();
	std::this_thread::sleep_for(std::chrono::milliseconds(30000));
	NET_joining_disable();
	cout << "Stopping joining timer" << endl;
}

void mqtt_iface::proccess_adapter_msg(string msg) {

	vector<int> vec;
	string s(msg);
	size_t pos = 0;
	string token;
	string delimiter = ",";

	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		vec.push_back(stoi(token));
		s.erase(0, pos + delimiter.length());
	}

	if(vec[0] == SET_ACTORS) {
		cout << "COMMAND: SET ACTORS" << endl;
		cout << "DST ADDRESS: ";
		unsigned int byte_index, i, j, proto_ver, value_count;
		uint8_t dst_addr[4];
		for(i=0; i<4; i++) {
			dst_addr[i] = vec[i+2];
			cout << "0x" << setfill('0') << setw(2) << hex << unsigned(dst_addr[i]) << " ";
		}
		cout << endl;
        proto_ver = vec[1];
        value_count = vec[6];
        cout << "PROTO VER: " << proto_ver << endl;
        cout << "NUMBER OF VALUES: " << value_count << endl;

        byte_index = 7;
        for(i=0; i<value_count; i++) {
            cout << "VALUE[" << i << "]:";
            cout << " module_id " << vec[byte_index++];
            int len = vec[byte_index++];
            cout << " LEN " << len;
            cout << " DATA ";

            for (j=0; j<len; j++) {
                cout << "0x" << setfill('0') << setw(2) << hex << vec[byte_index] << " ";
                byte_index++;
            }
            cout << endl;
        }

        uint8_t sensor_msg[MAX_MSG_LEN];
		uint8_t len;

        sensor_msg[0] = SET_ACTORS;
        sensor_msg[1] = 0x01;
        len = 2;

        for (i=6; i<vec.size(); i++) {
			sensor_msg[i-4] = vec[i];
            len++;
		}

        cout << "TO SENSOR (" << dec << unsigned(len) << "): ";
		for(i=0; i<len; i++) {
			cout << "0x" << setfill('0') << setw(2) << hex << unsigned(sensor_msg[i]) << " ";
		}
        cout << endl;

        send_cbp(dst_addr, sensor_msg, len);
	}
	else if (vec[0] == SET_JOIN_MODE) {
		cout << "COMMAND: SET JOIN MODE" << endl;
//		joining_enabled = true;
		std::thread joining_timer = std::thread(joining_timer_f);
		joining_timer.detach();
	}
	else if (vec[0] == UNPAIR_SENSOR) {
		cout << "COMMAND: UNPAIR_SENSOR" << endl;
		cout << "ADDRESS: ";
		unsigned int i;
		uint8_t dst_addr[4];
		for(i=0; i<4; i++) {
			dst_addr[i] = vec[i+1];
			cout << "0x" << setfill('0') << setw(2) << hex << unsigned(dst_addr[i]) << " ";
		}
		cout << endl;
        NET_unjoin(dst_addr);
		//uint8_t sensor_msg[1];
		//sensor_msg[0] = UNPAIR_SENSOR;
		//send_cbp(dst_addr, sensor_msg, 1);
	}

//	for(std::vector<int>::iterator it = vec.begin(); it != vec.end(); it++) {
//		cout << *it << endl;
//	}

}

void mqtt_iface::on_message(const struct mosquitto_message *message) {
	std::string msg_topic(message->topic);
	std::string msg_text((const char*)message->payload);
	if(msg_topic.compare(TOPIC_RX) == 0){
		cout << "-------------------------------------------------------------" << endl;
		cout << "Message received on " << TOPIC_RX << ": " << msg_text << endl;
		proccess_adapter_msg(msg_text);
	}
}

void mqtt_iface::on_connect(int rc) {
	printf("Mosquito connecting with code %d.\n", rc);
	if (rc == 0) {
		subscribe(NULL, TOPIC_RX);
	}
}


void mqtt_iface::on_subscribe(int mid, int qos_count, const int *granted_qos) {
	printf("Subscription succeeded.\n");
}
