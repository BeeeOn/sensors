#include "global.h"
#include "include.h"


void delay_ms(uint16_t t) {
	std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

// extern to NET layer - save/load NID, parent CID

  void save_configuration(uint8_t *buf, uint8_t len) {
   for (uint8_t i = 0; i < 4; i++)
   	 GLOBAL_STORAGE.nid[i] = buf[i];
   	
    GLOBAL_STORAGE.nid[4] = buf[4];
}

  void load_configuration(uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < 4; i++)
    	  buf[i] = GLOBAL_STORAGE.nid[i];
    
    buf[4] = GLOBAL_STORAGE.nid[4];
}

bool accel_int = false;
