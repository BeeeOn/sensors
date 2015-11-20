#ifndef GLOBAL_STORAGE_H 
#define GLOBAL_STORAGE_H

#include "include.h"

//pre xc8
#ifndef X86
struct GLOBAL_storage_t {

    bool waiting_pair_response;
    bool pair_mode;                 // coordinator accept pair messages (nid = 0x00000000)
    bool routing_enabled;
    bool sleepy_device;
  
    uint8_t routing_tree[64];       //routing tree
    uint8_t nid[4];                        //my network id
    uint8_t cid;                             //my coord id
  
    uint8_t parent_cid;                // end device parent coordinator adress
    uint8_t edid[4];                     //end device id
    
    uint16_t refresh_time;  // refresh time
  
};

//pre x86
#else


#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

struct GLOBAL_storage_t {

    bool waiting_pair_response;
    bool pair_mode;                 // coordinator accept pair messages (nid = 0x00000000)
    bool routing_enabled;
    bool sleepy_device;
  
    uint8_t routing_tree[64];       //routing tree
    uint8_t nid[4];                        //my network id
    uint8_t cid;                             //my coord id
  
    uint8_t parent_cid;                // end device parent coordinator adress
    uint8_t edid[4];                     //end device id
    
    uint16_t refresh_time;  // refresh time
  
  //pre x86, pre simulator
  uint8_t id; //oznacenie v ramci simulatora
  		      //PID v hypervizore 
  uint8_t channel;
  uint8_t pid[4];
  
  uint8_t tocoord;
  uint8_t toed[4];
  uint8_t data_len;
  uint8_t data[63];
  uint8_t rssi;
  uint8_t bitrate;
  uint8_t band;
  
};
#endif

extern struct GLOBAL_storage_t GLOBAL_STORAGE; 

#endif
