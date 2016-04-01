/* 
 * File:   euid_config.h
 * Author: Panda
 *
 * Created on Streda, 2015, júl 22, 15:50
 */

#ifndef EDID_CONFIG_H
#define	EDID_CONFIG_H

#include <stdint.h>
#include "global.h"
#include "spieeprom.h"
#include <stdio.h>
#include "log.h"
#include "hw.h"

#define NID_ADRESS           0
#define NID_ADRESS_LEN       5
#define EUID_ADRESS          5
#define EUID_ADRESS_LEN      4

void load_euid_eeprom();
void save_refresh_eeprom(uint16_t val);



#endif	/* EDID_CONFIG */

