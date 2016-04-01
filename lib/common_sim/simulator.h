#ifndef SIMULATOR_H
#define	SIMULATOR_H

#include <stdint.h>
#include <stdio.h>
#include "mqtt_iface.h"
#include <iostream>
#include <string>

#define CONFIG_TOPIC "BeeeOn/config"
#define DATA_TO_TOPIC "BeeeOn/data_to"
#define DATA_FROM_TOPIC "BeeeOn/data_from"

#define TOPIC_RX "BeeeOn/data_to_PAN"
#define TOPIC_TX "BeeeOn/data_from_PAN"

static uint8_t LED0 =1;
static uint8_t LED1 =1;

void StartTimer(uint16_t time);
void ds_prepare(void);

std::string create_head(void);
void set_config(uint8_t *msg, uint8_t len);

void gen_random(char *s, const int len);

void run_cmd(uint8_t cmd);

#define DEBUG_ARG(data, len, device_type, number, text) debug_arg( *data, len, device_type, number, text)
#define DEBUG(device_type, number, text) debug(device_type, number, text)

void debug_arg(uint8_t *data, uint8_t len, uint8_t device_type, uint8_t number, string text);
void debug(uint8_t device_type, uint8_t number, string text);

void euid_load(); // load euid from eeprom
void refresh_load_eeprom();



#endif
