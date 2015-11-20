#ifndef SIMULATOR_H
#define	SIMULATOR_H

#include <stdint.h>
#include <stdio.h>
#include "mqtt_iface.h"
#include <iostream>
#include <string>

#define DEBUG_ARG(data, len, device_type, number, text) debug_arg( *data, len, device_type, number, text)
#define DEBUG(device_type, number, text) debug(device_type, number, text)

void debug_arg(uint8_t *data, uint8_t len, uint8_t device_type, uint8_t number, string text);
void debug(uint8_t device_type, uint8_t number, string text);

static uint8_t LED0 =1;
static uint8_t LED1 =1;

enum Edevices{
	PAN = 0,
	COORD,
	ED
};

void StartTimer(uint16_t time);
void ds_prepare(void);
void sendValues(void);


struct GLOBAL_CONFIG_t {
	uint8_t id[2];		      //ID v hypervizore 
	uint8_t edid[4];			 //EDID zariadeni
	uint8_t cid; 			 //CID 
	uint8_t nid[4]; 			 //NID siete, kde patri
	uint8_t refresh_time[2];	 //RESRESH TIME, interval posielania dat
	uint8_t temp[4];			 //Teplota
	uint8_t ex_temp[4];		 //Externa teplota
	uint8_t humidity[4];		 //HUMIDITY
  	bool pair_mode; 		 //PAIR MODE, 
	bool sleep;			 //SLEEP, ci je zariadenie uspate
};
string create_head(void);

void set_config(uint8_t *msg, uint8_t len);

void gen_random(char *s, const int len);

void run_cmd(uint8_t cmd);

#endif
