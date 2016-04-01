#ifndef GLOBAL_STORAGE_H
#define GLOBAL_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <string>

#define MAX_DEVICES 32
/**
 * @def device table
 */
typedef struct{
    bool coord;
    bool valid;
    bool sleepy;
    uint8_t edid[4];
    uint8_t cid;
	uint8_t parent_cid;
} device_table_record_t;


//pre arm
#ifndef X86
struct GLOBAL_storage_t {

  bool    pair_mode;                 // coordinator accept pair messages (nid = 0x00000000)
  bool    waiting_pair_response;
  bool    routing_enabled;

  uint8_t routing_tree[64];       //routing tree
  uint8_t nid[4];                        //my network id
  uint8_t cid;                             //my coord id

  uint8_t parent_cid;                // end device parent coordinator adress
  uint8_t edid[4];                     //end device id

  device_table_record_t devices[MAX_DEVICES];  // table devices connected to the network
  std::string device_table_path;  // path to device table file

};

//pre x86
#else
struct GLOBAL_storage_t {

  bool    pair_mode;                 // coordinator accept pair messages (nid = 0x00000000)
  bool    waiting_pair_response;
  bool    routing_enabled;

  uint8_t routing_tree[64];       //routing tree
  uint8_t nid[4];                        //my network id
  uint8_t cid;                             //my coord id

  uint8_t parent_cid;                // end device parent coordinator adress
  uint8_t edid[4];                     //end device id

  device_table_record_t devices[MAX_DEVICES];  // table devices connected to the network
  std::string device_table_path;  // path to device table file

  //pre x86, pre simulator
  uint8_t pid[4];		      //PID v hypervizore
  uint8_t channel;

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
