#include <stdlib.h>
#include <simulator.h>
#include <stdint.h>
#include <global.h>
#include <fitp.h>
#include <mqtt_iface.h>
#include <iostream>

#include <chrono>
#include <thread>
#include "net.h"



//Funkcia posle debug spravu s datami
void debug_arg(uint8_t *data, uint8_t len, uint8_t device_type, uint8_t number, string text) {
   string msg;
   msg.clear();

   //add PID
	for (uint8_t i = 0; i < 4; i++)
		msg += to_string(GLOBAL_STORAGE.pid[i]) + ',';

   msg += to_string(device_type) + ",";
   msg += to_string(number) + ",";

   for (uint8_t i = 0; i < len; i++) {
      msg += to_string(unsigned(data[i])) + ',';
   }

   //pridanie vlastneho textu
   if (text.length() >0)
      msg += text + ",";

   //mq->send_message("BeeeOn/debug", msg.c_str(), msg.length());
   //cout << msg << endl;
}

//funkcia posle debug spravu bez dat
void debug(uint8_t device_type, uint8_t number, string text) {
   string msg;
   msg.clear();

   //add PID
	for (uint8_t i = 0; i < 4; i++)
		msg += to_string(GLOBAL_STORAGE.pid[i]) + ',';

   msg += to_string(device_type) + ",";
   msg += to_string(number) + ",";

   //pridanie vlastneho textu
   if (text.length() >0)
      msg += text + ",";

   //mq->send_message("BeeeOn/debug", msg.c_str(), msg.length());
   //cout << msg << endl;
}

void StartTimer(uint16_t time) {

}
void ds_prepare(void) {

}

void HW_ReInit(){}
void HW_DeInit(){}
void goSleep(){}

void gen_random(char *s, const int len) {
     for (int i = 0; i < len; ++i) {
         int randomChar = rand()%(26+26+10);
         if (randomChar < 26)
             s[i] = 'a' + randomChar;
         else if (randomChar < 26+26)
             s[i] = 'A' + randomChar - 26;
         else
             s[i] = '0' + randomChar - 26 - 26;
     }
     s[len] = 0;
}

std::string create_head(void) {
	string msg  = "";
	//add PID
	for (uint8_t i = 0; i < 4; i++)
		msg += to_string(GLOBAL_STORAGE.pid[i]) + ',';

	msg += to_string(GLOBAL_STORAGE.channel) + ',';
	msg += to_string(GLOBAL_STORAGE.bitrate) + ',';
	msg += to_string(GLOBAL_STORAGE.band) + ',';
	msg += to_string(GLOBAL_STORAGE.rssi) + ',';
	//cout << msg << endl;
	return msg;
}
