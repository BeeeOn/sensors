#include "net.h"
#include "net_common.h"

#include "log.h"
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

using namespace std;

#define NETROUTER_EP_BYPASS_PIZE 4
#define MAX_ROUTING_DATA 40

typedef struct {
  uint8_t signal_strenght;
  uint8_t from[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE]; //message in join and move can be bigger on link layer but they are send trought network protocol(eg. to PAN) too so we can have shorter buffer
  uint8_t len:7;
  uint8_t empty:1;
} NET_movejoin_request_t;

typedef struct{
  uint8_t address[4];
  uint8_t data[MAX_NET_PAYLOAD_SIZE];
  uint8_t len:6;
  uint8_t is_set:1;
} NET_direct_join_struct_t;

//TODO Bypass flag?
typedef struct{
  uint8_t type;
  uint8_t scid;
  uint8_t sedid[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE];
  uint8_t len;

} NET_current_processing_packet_t;

// table of sleepy messages
typedef struct{
  uint8_t toed[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE];
  uint8_t len;
  bool valid;
} NET_sleepy_messages_t;


struct NET_storage_t {

  /* this values are meneged by circular buffer functions dont touch them*/
  uint8_t cb_data[NETROUTER_EP_BYPASS_PIZE][MAX_NET_PAYLOAD_SIZE+1];
  uint8_t cb_start;
  uint8_t cb_end;
  bool cb_full;
  /* end of circular values*/

  NET_direct_join_struct_t direct_join;

  uint8_t state;
  uint8_t state_stash;
  NET_current_processing_packet_t processing_packet;

  NET_movejoin_request_t move_request;
  NET_movejoin_request_t join_request;

  NET_sleepy_messages_t sleepy_messages[MAX_SLEEPY_MESSAGES];

} NET_STORAGE;

uint8_t cid=0;

/*
 * Supporting function exists also on link or phy layers
 */
extern bool equal_arrays(uint8_t* addr1,uint8_t* addr2, uint8_t size);
extern bool zero_address(uint8_t* addr);
extern bool array_cmp(uint8_t *array_a, uint8_t *array_b, uint8_t len);
extern void array_copy(uint8_t *src, uint8_t *dst, uint8_t len);

extern void delay_ms(uint16_t t);

/**
 * @section utils_circular_buffer
 * @brief functions to work with fifo front using circular buffer
 * used for coord -> endpoint bypass
 */

void cb_init(){
  NET_STORAGE.cb_start = 0;
  NET_STORAGE.cb_end = 0;
  NET_STORAGE.cb_full = false;
}

// indexy 0-3
uint8_t _cb_next_index(uint8_t actual){
  return (actual + 1)  % NETROUTER_EP_BYPASS_PIZE;
}

bool cb_empty(){
  return (NET_STORAGE.cb_start == NET_STORAGE.cb_end) && (! NET_STORAGE.cb_full);
}

bool cb_full(){
  return ((NET_STORAGE.cb_start == NET_STORAGE.cb_end) && NET_STORAGE.cb_full);
}

bool cb_push(uint8_t *data,uint8_t len){

  if( cb_full() ) return false;
  int i=0;
  for(;i<len && i < MAX_NET_PAYLOAD_SIZE;i++){
    NET_STORAGE.cb_data[NET_STORAGE.cb_end][i] = data[i];
  }
  NET_STORAGE.cb_data[NET_STORAGE.cb_end][MAX_NET_PAYLOAD_SIZE] = i;

  NET_STORAGE.cb_end = _cb_next_index(NET_STORAGE.cb_end);

  if(NET_STORAGE.cb_end == NET_STORAGE.cb_start){
    NET_STORAGE.cb_full = true;
  }

  return true;
}

bool cb_pop(uint8_t *data, uint8_t* len){

  if( cb_empty() ) return false;

  (*len) = NET_STORAGE.cb_data[NET_STORAGE.cb_start][MAX_NET_PAYLOAD_SIZE];
  for(int i=0;i<(*len);i++){
    data[i] = NET_STORAGE.cb_data[NET_STORAGE.cb_start][i];
  }

  NET_STORAGE.cb_data[NET_STORAGE.cb_start][MAX_NET_PAYLOAD_SIZE] = 0;
  NET_STORAGE.cb_start = _cb_next_index(NET_STORAGE.cb_start);
  NET_STORAGE.cb_full = false;
  return true;
}

uint8_t ROUTING_get_next(uint8_t destination_cid){
  /*uint8_t address = destination_cid;

  while(address !=0 && GLOBAL_STORAGE.routing_tree[address] != GLOBAL_STORAGE.cid){
          address=GLOBAL_STORAGE.routing_tree[address];
  }

  if(address == 0){
        return GLOBAL_STORAGE.routing_tree[GLOBAL_STORAGE.cid];
  }

  return address;*/
   // printf("===============ROUTIIIIIIIIING\n=======");
      uint8_t address;
      uint8_t previous_address;
      address = destination_cid;
      while(1)
      {
          if(address == GLOBAL_STORAGE.parent_cid)
          {
          	  address = previous_address;
          	   //printf("add ret: %02x\n");
              return address;
          }
          previous_address = address;
           //printf("Prev_addr: %02x\n", previous_address);
          address = GLOBAL_STORAGE.routing_tree[address];
           //printf("Address: %02x\n", address);
      }
}

/**
 * @brief send using net frame
 */
bool send(uint8_t message_type, uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len, bool with_ack){
  uint8_t tmp[MAX_NET_PAYLOAD_SIZE];
  uint8_t index=0;
  uint8_t address_coord;

  printf("PAN IN SEND\n");
  tmp[index++]=(message_type << 4) | ((tocoord >> 2) & 0x0F);
  printf("message_type|tocoord: %d\n", tmp[0]);
  tmp[index++]=((tocoord << 6) & 0xC0) | (GLOBAL_STORAGE.cid & 0x3f);
  printf("tocoord|cid: %d\n", tmp[1]);
  tmp[index++]=toed[0];
  tmp[index++]=toed[1];
  tmp[index++]=toed[2];
  tmp[index++]=toed[3];
  tmp[index++]=GLOBAL_STORAGE.edid[0];
  tmp[index++]=GLOBAL_STORAGE.edid[1];
  tmp[index++]=GLOBAL_STORAGE.edid[2];
  tmp[index++]=GLOBAL_STORAGE.edid[3];

  for(uint8_t i=0;i<len && index<MAX_NET_PAYLOAD_SIZE;i++){
    tmp[index++]=data[i];
  }
  address_coord = ROUTING_get_next(tocoord);

  if(with_ack)
  {
     //return LINK_send_to_ed(toed, tmp,index);
     //return LINK_send(tmp,index);
     return LINKROUTER_send(true, toed, tmp, index, true);
  }
  else
  {
      return LINK_send_without_ack(tmp,index, toed);
  }
}


///// BEGIN: DEVICE TABLE SUPPORT FUNCTIONS ////////////////////////////////////
/*
 * Print device table for debug purposes
 */
void print_device_table(void) {
    printf("\nEDID            CID     sleepy\n");
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid) {
            printf("%02x %02x %02x %02x\t%d\t%d\n", \
                    GLOBAL_STORAGE.devices[i].edid[0], \
                    GLOBAL_STORAGE.devices[i].edid[1], \
                    GLOBAL_STORAGE.devices[i].edid[2], \
                    GLOBAL_STORAGE.devices[i].edid[3], \
                    GLOBAL_STORAGE.devices[i].cid, \
                    GLOBAL_STORAGE.devices[i].sleepy);
        }
    }
    printf("---------------------------------\n");
}

/*
 * Check if device is in my device table
 */
bool is_my_device(uint8_t* edid) {
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid &&
            array_cmp(GLOBAL_STORAGE.devices[i].edid, edid, 4))
        {
            return true;
        }
    }
    return false;
}

/*
 * find out if edid device is sleepy or not
 */
bool is_sleepy_device(uint8_t* edid) {
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid &&
            array_cmp(GLOBAL_STORAGE.devices[i].edid, edid, 4) &&
            GLOBAL_STORAGE.devices[i].sleepy)
        {
            return true;
        }
    }
}

/*
 * Check is device is COORD
 */
 bool is_coord_device(uint8_t* edid) {
     for (uint8_t i=0; i<MAX_DEVICES; i++) {
         if (GLOBAL_STORAGE.devices[i].valid &&
             array_cmp(GLOBAL_STORAGE.devices[i].edid, edid, 4) &&
             GLOBAL_STORAGE.devices[i].coord)
         {
             return true;
         }
     }
 }

/*
 * @return device's parent_cid or it's own cid in case of coordinator. If device
 *         si not in the table, return 0xFF;
 *
 */
bool get_cid(uint8_t* edid) {
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid &&
            array_cmp(GLOBAL_STORAGE.devices[i].edid, edid, 4))
        {
            return GLOBAL_STORAGE.devices[i].cid;
        }
    }
    return 0xFF;
}

/*
 * Store joining device to table of devices.
 * Return true/false depending on success.
 */
bool add_device(uint8_t *edid, uint8_t cid, bool sleepy, bool coord) {
// EDID CID SLEEPY COORD VALID
    if (is_my_device(edid)) return false;
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid == false) {
            array_copy(edid, GLOBAL_STORAGE.devices[i].edid, 4);
            GLOBAL_STORAGE.devices[i].cid = cid;
            GLOBAL_STORAGE.devices[i].sleepy = sleepy;
            GLOBAL_STORAGE.devices[i].valid = true;
            GLOBAL_STORAGE.devices[i].coord = coord;
            return true;
        }
    }
    return false;
}

/*
 * Remove joined device from table of devices.
 * Return true/false depending on success.
 */
bool remove_device(uint8_t *edid) {
    for (uint8_t i=0; i<MAX_DEVICES; i++) {
        if (GLOBAL_STORAGE.devices[i].valid &&
            array_cmp(GLOBAL_STORAGE.devices[i].edid, edid, 4))
        {
            GLOBAL_STORAGE.devices[i].valid = false;
            return true;
        }
    }
    return false;
}

/*
 * Save device table to a file specified by string in
 * GLOBAL_STORAGE.device_table_path veriable
 * EDID | PARRENT_CID | CID | SLEEPY | COORD --- COORD => COORD=0;END=1
 */
bool save_device_table(void) {
    ofstream fs(GLOBAL_STORAGE.device_table_path);
    if (!fs) return false;
    for (uint8_t i=0; i<MAX_DEVICES;i++) {
        if (GLOBAL_STORAGE.devices[i].valid) {
            fs << setfill('0') << hex
               << setw(2) << unsigned(GLOBAL_STORAGE.devices[i].edid[0]) << " "
               << setw(2) << unsigned(GLOBAL_STORAGE.devices[i].edid[1]) << " "
               << setw(2) << unsigned(GLOBAL_STORAGE.devices[i].edid[2]) << " "
               << setw(2) << unsigned(GLOBAL_STORAGE.devices[i].edid[3]) << " "
               << "| " <<  setfill('0') << setw(2)
               << unsigned(GLOBAL_STORAGE.devices[i].parent_cid) << " | "
			   << unsigned(GLOBAL_STORAGE.devices[i].cid) << " | "
               << GLOBAL_STORAGE.devices[i].sleepy << " | "
			   << GLOBAL_STORAGE.devices[i].coord << endl;
        }
    }
    fs.close();
    if (!fs) return false;
    return true;
}

/*
 * Load device table from a file specified by string in
 * GLOBAL_STORAGE.device_table_path veriable
 * EDID | PARRENT_CID | CID | SLEEPY | COORD --- COORD => COORD=1;END=0
 */
bool load_device_table(void) {
    int i=0;
    int edid[4];
	int parent_cid;
    int cid;
    char delimiter;
    int sleepy;
	  int coord;
    ifstream fs(GLOBAL_STORAGE.device_table_path);
    string line;

    if (!fs) return false;
    while (getline(fs, line)) {
        istringstream iss(line);
        if (!(iss >> hex >> edid[0] >> edid[1] >> edid[2] >> edid[3] >>
              delimiter >> parent_cid >> delimiter >> cid >> delimiter >> sleepy >> delimiter >> coord)) continue;
        GLOBAL_STORAGE.devices[i].valid = true;
        (sleepy)?GLOBAL_STORAGE.devices[i].sleepy = true:GLOBAL_STORAGE.devices[i].sleepy = false;
        GLOBAL_STORAGE.devices[i].edid[0] = edid[0];
        GLOBAL_STORAGE.devices[i].edid[1] = edid[1];
        GLOBAL_STORAGE.devices[i].edid[2] = edid[2];
        GLOBAL_STORAGE.devices[i].edid[3] = edid[3];
		GLOBAL_STORAGE.devices[i].parent_cid = parent_cid;
        GLOBAL_STORAGE.devices[i].cid = cid;
		GLOBAL_STORAGE.devices[i].coord = coord;
        i++;
    }
    fs.close();

    return true;
}
///// END: DEVICE TABLE SUPPORT FUNCTIONS //////////////////////////////////////

/**
 * select routing record
 */
 bool select_routing_record(uint8_t from, uint8_t to) {
	 int i = 0;
	 while (i < 64) {
		 if (from == to) return true;
		 from = GLOBAL_STORAGE.routing_tree[from];
		 i++;
	 }
	 return false;
 }

/**
 * Send routing table
 */
void send_routing_table(uint8_t tocoord, uint8_t* euid, uint8_t* data, uint8_t len) {
	uint8_t config_packet = 0;
	uint8_t packet_count = 0;

	//total packet
	if (len%MAX_ROUTING_DATA != 0)
		packet_count++;

	packet_count += len/MAX_ROUTING_DATA;
	config_packet = packet_count << 4;
	cout << "TOTAL PACKET: " << unsigned(packet_count) << endl;
	cout << "DATA LEN: " << unsigned(len) << endl;

	//send
	int data_index = 0;
	uint8_t send_data[MAX_ROUTING_DATA+1];
	for (uint8_t j = 0; MAX_ROUTING_DATA*j <len; j++) {
		config_packet++;

		// Create part of packet, config byte + MAX_ROUTING_DATA
		send_data[0] = config_packet;
		uint8_t i;
		for (i = 1; i <= MAX_ROUTING_DATA && data_index < len; i += 2) {
			// remove pan record
			if (send_data[i-2] == 0 && send_data[i-1] == 0) {i -= 2; break;}

			//send to coord only child coord
			if (data_index%2 == 0 && select_routing_record(data[data_index], tocoord)) {
				//cout << "TMP::" << unsigned(data[data_index]) << endl;
				send_data[i] = data[data_index++];
				send_data[i+1] = data[data_index++];
				continue;
			}
			data_index += 2;
			i -= 2;
		}

		cout << "CONFIG PACKET: " << hex << unsigned(config_packet) << endl;

		send(PT_NETWORK_ROUTING_DATA, tocoord, euid, send_data, i, true);
	}
 }

/*
 * Loading routing table
 */
bool load_routing_table() {

	uint8_t r_table[128];
	int k = 0;


	//show device in fitprocold.devices and CREATING ROUTING TREE
	cout << "\nDevices IN ROUTING TABLE:\n";
	cout << "EDID | PCID | CID | SLEEPY | IS_COORD" << endl;
	for (uint8_t i = 0; i < MAX_DEVICES; i++) {
		if (GLOBAL_STORAGE.devices[i].valid) {
			for (uint8_t j = 0; j < 4; j++)
				cout << setfill('0') << setw(2) << hex << unsigned(GLOBAL_STORAGE.devices[i].edid[j]) << " ";

			cout << unsigned(GLOBAL_STORAGE.devices[i].parent_cid) << " ";
			cout << unsigned(GLOBAL_STORAGE.devices[i].cid) << " ";
			cout << unsigned(GLOBAL_STORAGE.devices[i].sleepy) << " ";
			cout << unsigned(GLOBAL_STORAGE.devices[i].coord) << " ";
			cout << endl;

		//creating routing tree, only COORD = 1
		if (unsigned(GLOBAL_STORAGE.devices[i].coord) == 1)
			GLOBAL_STORAGE.routing_tree[i] = unsigned(GLOBAL_STORAGE.devices[i].parent_cid);
		}
	}

	//Routing table
	cout << "\nROUTING TABLE:" << endl;
	for (uint8_t i = 0; i < MAX_DEVICES; i++) {
		if (GLOBAL_STORAGE.devices[i].valid && GLOBAL_STORAGE.devices[i].coord == 1) {
			cout << unsigned(i) << " : " << unsigned(GLOBAL_STORAGE.routing_tree[i]) << endl;

			r_table[k] = i;
			r_table[k+1] = unsigned(GLOBAL_STORAGE.devices[i].parent_cid);
			k += 2;
		}
	}

	//send routing table to connect devices
	for (uint8_t i = 0; i < MAX_DEVICES; i++) {
		if (GLOBAL_STORAGE.devices[i].valid &&
			GLOBAL_STORAGE.devices[i].parent_cid == 0 && GLOBAL_STORAGE.devices[i].cid != 0) {
			cout << "SEND ROUTING TABLE TO COORD: "<<setfill('0') << setw(2) << hex << unsigned(GLOBAL_STORAGE.devices[i].cid) << "\n";
			uint8_t euid[4];
			euid[0] = GLOBAL_STORAGE.devices[i].edid[0];
			euid[1] = GLOBAL_STORAGE.devices[i].edid[1];
			euid[2] = GLOBAL_STORAGE.devices[i].edid[2];
			euid[3] = GLOBAL_STORAGE.devices[i].edid[3];

			send_routing_table(GLOBAL_STORAGE.devices[i].cid, euid, r_table, k);
		}
	}
}




///// BEGIN: SLEEPY MESSAGES TABLE SUPPORT FUNCTIONS ///////////////////////////
/*
 * Print sleepy messages table for debug purposes
 */
void print_sleepy_message_table() {
    printf("\nEDID            payload\n");
    for (uint8_t i=0; i<MAX_SLEEPY_MESSAGES; i++) {
        if (NET_STORAGE.sleepy_messages[i].valid) {
            printf("%02x %02x %02x %02x\t",
                NET_STORAGE.sleepy_messages[i].toed[0],
                NET_STORAGE.sleepy_messages[i].toed[1],
                NET_STORAGE.sleepy_messages[i].toed[2],
                NET_STORAGE.sleepy_messages[i].toed[3]
            );
            for(uint8_t j=0; j<NET_STORAGE.sleepy_messages[i].len; j++) {
                printf("%02x ", NET_STORAGE.sleepy_messages[i].payload[j]);
            }
            printf("\n");
        }
    }
    printf("---------------------------------\n");
}

/*
 * Push messages for sleepy sensors, only one message is allowed for ED at once
 * @return true/false depending if push was successful
 */
bool push_sleepy_message(uint8_t *toed, uint8_t *payload, uint8_t len) {
    uint8_t i;
    // overwrite current message for toed if exists
    for (i=0; i<MAX_SLEEPY_MESSAGES; i++) {
        if (NET_STORAGE.sleepy_messages[i].valid == true &&
            array_cmp(NET_STORAGE.sleepy_messages[i].toed, toed, 4) == true)
        {
            for (uint8_t j=0; j<len; j++) {
                NET_STORAGE.sleepy_messages[i].payload[j] = payload[j];
            }
            NET_STORAGE.sleepy_messages[i].len = len;
            return true;
        }
    }

    for (i=0; i<MAX_SLEEPY_MESSAGES; i++) {
        if (NET_STORAGE.sleepy_messages[i].valid == false) {
            uint8_t j;
            for (j=0; j<len; j++) {
                NET_STORAGE.sleepy_messages[i].payload[j] = payload[j];
            }
            for (j=0; j<4; j++) {
                NET_STORAGE.sleepy_messages[i].toed[j] = toed[j];
            }
            NET_STORAGE.sleepy_messages[i].len = len;
            NET_STORAGE.sleepy_messages[i].valid = true;
            return true;
        }
    }
    return false;
}

/*
 * Get stored sleepy message for edid sensor
 * @return pointer to message record in NET_STORAGE.sleepy_messages,
 *         if not found return NULL
 */
NET_sleepy_messages_t * get_sleepy_message(uint8_t *edid) {
    for (uint8_t i=0; i<MAX_SLEEPY_MESSAGES; i++) {
        if (NET_STORAGE.sleepy_messages[i].valid == true) {
            if (array_cmp(NET_STORAGE.sleepy_messages[i].toed, edid, 4) == true) {
                D_NET printf ("get_sleepy_message(): found\n");
                return &NET_STORAGE.sleepy_messages[i];
            }
        }
    }
    return NULL;
}
///// END: SLEEPY MESSAGES TABLE SUPPORT FUNCTIONS /////////////////////////////


/*
 * Process packet for local device
 */
void local_process_packet(uint8_t* payload, uint8_t len){

    uint8_t type = payload[0] >> 4;
    uint8_t dcid = ((payload[0] << 2) & 0x3C) | ((payload[1] >> 6) & 0x03);
    uint8_t scid = payload[1] & 0x3f;
    uint8_t dedid[4];
    uint8_t sedid[4];
    NET_sleepy_messages_t * p_sleepy_message;
    uint8_t net_payload[MAX_NET_PAYLOAD_SIZE];
    uint8_t net_payload_len=len-10;


    array_copy(payload+2, dedid, 4);
    array_copy(payload+6, sedid, 4);
    array_copy(payload+10, net_payload, net_payload_len);

    D_NET printf("local_process_packet(): type %02x dcid %02x scid %02x\n", \
                  type, dcid, scid);
    D_NET printf("local_process_packet(): sedid %02x %02x %02x %02x dedid %02x %02x %02x %02x\n", \
                 sedid[0], sedid[1], sedid[2], sedid[3], dedid[0], dedid[1], dedid[2], dedid[3]);

    // refuse devices wchich are not in my device table
    if (!is_my_device(sedid)) {
        D_NET printf("local_process_packet(): packet refused, is not my device\n");
        return;
    }

    if (type == PT_DATA_DR) {  // received packet is data request
        if ((p_sleepy_message = get_sleepy_message(sedid)) !=  NULL) {
            // send first and then invalidate record
            p_sleepy_message->valid = false;
            send(PT_DATA_ACK_DR_WAIT, get_cid(sedid), sedid, NULL, 0, false);
            delay_ms(150);
            send(PT_DATA, get_cid(sedid), sedid, p_sleepy_message->payload,
                 p_sleepy_message->len, false);
        }
        else {
            send(PT_DATA_ACK_DR_SLEEP, get_cid(sedid), sedid, NULL, 0, false);
        }
        NET_received(scid, sedid, net_payload, len, 0);
    }
    else if (type == PT_DATA) {
        NET_received(scid, sedid, net_payload, len, 0);
    }
}


//forward declarations
void _net_send_joinreq_topan();
void _net_send_movereq_topan();
void _net_user_data();
void _net_user_ack();

/**
 * @brief load data from bypass circular buffer record into processing_packet
 */
void bypass_load(){
  uint8_t loaded_data[MAX_NET_PAYLOAD_SIZE+1];
  uint8_t loaded_len;

  //load data from bypass buffer
  cb_pop(loaded_data, &loaded_len);

  if(loaded_len < 10){
    printf("Invalid data from bypass, too short\n");
    return;
  }

  NET_STORAGE.processing_packet.type = loaded_data[0] >> 4;
  NET_STORAGE.processing_packet.scid = loaded_data[1] & 0x3f;

  for(uint8_t i=0;i<4;i++){
    NET_STORAGE.processing_packet.sedid[i] = loaded_data[i+6];
  }

  uint8_t i=0;
  for(;i < (loaded_len-10) && i<MAX_NET_PAYLOAD_SIZE; i++){
    NET_STORAGE.processing_packet.payload[i] = loaded_data[i+10];
  }
  NET_STORAGE.processing_packet.len = i;
}

/**
 * @brief function called to process packet
 */
//void process(){
  /*
  //if there is state stashed by higher priority message
  if(NET_STORAGE.state == SCD_SENDJOIN_TO_PAN || NET_STORAGE.state == SCD_SENDMOVE_TO_PAN){
    NET_STORAGE.state = NET_STORAGE.state_stash;
  }

  //if have join request send it
  if(!NET_STORAGE.join_request.empty){
    _net_send_joinreq_topan();
    NET_STORAGE.state_stash = NET_STORAGE.state;
    NET_STORAGE.state = SCD_SENDJOIN_TO_PAN;
  }

  //if have move request send it
  if(!NET_STORAGE.move_request.empty){
    _net_send_movereq_topan();
    NET_STORAGE.state_stash = NET_STORAGE.state;
    NET_STORAGE.state = SCD_SENDMOVE_TO_PAN;
  }

  // processing done check pakets from bypass(device is busy until all bypass messages arent processed)
  if(NET_STORAGE.state == SED_READY_STATE){

    if( !cb_empty() ){
      bypass_load();
      NET_STORAGE.state = SED_PREPROCES;
    } else {
      return;
    }
  }

  //switch by packet type if PREPROCES state is set;
  if(NET_STORAGE.state == SED_PREPROCES){
    switch(NET_STORAGE.processing_packet.type){
      case PT_DATA:
      case PT_DATA_NACK:
          NET_STORAGE.state = SED_USER_DATA;
        break;
      case PT_DATA_ACK:
      case PT_DATA_BOTH:
        NET_STORAGE.state = SED_USER_DATA_ACK;
        break;

      case PT_NETWORK_EXTENDED:
        //packet have extended type in first byte of payload
        switch (NET_STORAGE.processing_packet.payload[0]){
          case PTNETEXT_SET_CHANNEL:
              //format of this message is not finnal can be changed
              //TODO can be refused from PHY layer if no channel allowed
              PHY_set_channel(NET_STORAGE.processing_packet.payload[1]);
              NET_STORAGE.state = SED_READY_STATE;
            break;
          case PTNETEXT_JOIN_RESPONSE:
            //TODO verify constants
            LINK_send_join_response(NET_STORAGE.processing_packet.payload+1,NET_STORAGE.processing_packet.payload+5,NET_STORAGE.processing_packet.len-5);
            NET_STORAGE.state = SED_READY_STATE;
            break;
          case PTNETEXT_MOVE_RESPONSE:
            //TODO verify constants
            LINK_send_move_response(NET_STORAGE.processing_packet.payload+1,NET_STORAGE.processing_packet.payload+5,NET_STORAGE.processing_packet.len-5);
            NET_STORAGE.state = SED_READY_STATE;
            break;
          //ignore all other extended message types
          default:
            NET_STORAGE.state = SED_READY_STATE;
        }
        break;

      //ignore all other message types
      default:
        NET_STORAGE.state = SED_READY_STATE;
        break;
    }
  }

  switch(NET_STORAGE.state){
    case SED_USER_DATA_ACK:
      _net_user_ack();
      //-> SED_USER_DATA
      break;
    case SED_USER_DATA:
      _net_user_data();
      // -> SED_READY_STATE
      break;
  }
  */
//}


void LINK_error_handler(bool rx, uint8_t* data, uint8_t len){
  if(rx){
    printf("LINK error during reciving\n");
  } else {
    printf("LINK error during transmiting\n");
  }
}



bool LINK_is_device_busy(){
  return NET_STORAGE.state != SED_READY_STATE || (! cb_empty());
}


/*
 * Send join response directly to joining ed.
 */
void send_join_response(uint8_t *toed, uint8_t device, uint8_t len) {
    uint8_t tmp[MAX_NET_PAYLOAD_SIZE];
    uint8_t index=0;

    tmp[index++] = (PT_DATA_JOIN_RESPONSE << 4) & 0xF0;
    // pripadne prideleni cisla COORD
    if(device == 0xCC)
    {// cid - globalni pocitadlo? uint8_t cid = 1; (protoze 0 je PAN) v net.cc
     // fce na overeni poctu pripojenych COORD v tabulce pripojenych zarizeni?
       tmp[index++] = cid++;
    }
    else
    {// edid nema CID (CID se predava na sitove vrstve na 6 b -> 2^6 = 64), takze jej ani neuklada
       tmp[index++] = 65;
    }
    tmp[index++] = toed[0];
    tmp[index++] = toed[1];
    tmp[index++] = toed[2];
    tmp[index++] = toed[3];
    tmp[index++] = GLOBAL_STORAGE.edid[0];
    tmp[index++] = GLOBAL_STORAGE.edid[1];
    tmp[index++] = GLOBAL_STORAGE.edid[2];
    tmp[index++] = GLOBAL_STORAGE.edid[3];
    // payload with nid, cid information
    tmp[index++] = GLOBAL_STORAGE.nid[0];
    tmp[index++] = GLOBAL_STORAGE.nid[1];
    tmp[index++] = GLOBAL_STORAGE.nid[2];
    tmp[index++] = GLOBAL_STORAGE.nid[3];
    tmp[index++] = GLOBAL_STORAGE.cid; // parent cid for this ed
    /*for(uint8_t i=0;i<len && index<MAX_NET_PAYLOAD_SIZE;i++){
        tmp[index++]=payload[i];
    }*/

    LINK_send_join_response(toed, tmp, index);
}

/*
 * Unjoin device from the network.
 * @return true/false depending if device was removed
 */
bool NET_unjoin(uint8_t* ed) {
    if (remove_device(ed)) {
        save_device_table();
		load_routing_table();
        return true;
    }
    return false;
}


/*
 * Join request received on link layer
 */
void LINK_join_request_received(uint8_t signal_strenght, uint8_t* payload, uint8_t len) {
    // if(len < 15) return; -> otestovat
    if(len < 10) return;

    // PT_DATA_JOIN_REQUEST
    uint8_t type = (payload[0] >> 4);
    uint8_t sedid[4];
    array_copy(payload+6, sedid, 4);

    if(type == PT_DATA_JOIN_REQUEST) {
        D_NET printf("PT_DATA_JOIN_REQUEST from %02x %02x %02x %02x\n", \
                      sedid[0], sedid[1], sedid[2], sedid[3]);
        // if (GLOBAL_STORAGE.pair_mode) {
            // ZMENA! Nutno predat SLEEPY FLAG (payload[1]) aby se rozlisilo, zda byl JOIN REQUEST poslan od ED nebo COORD
            send_join_response(sedid, payload[1], 0);
            add_device(sedid, cid, payload[1] == 0xFF, payload[1] == 0xCC);
            if(!save_device_table()) {
                D_NET cout << "LINK_join_request_received(): cannot save device: " \
                           << GLOBAL_STORAGE.device_table_path << endl;
				load_routing_table();
            }
            D_NET print_device_table();
        }
    //}
    else {
        D_NET printf("Invalid join request\n");
    }
}


/*
 * Process packet incoming from link layer
 */
void LINK_process_packet(uint8_t* payload, uint8_t len){
/*
    if(len < 10){
        D_NET printf("Invalid net packet, too short\n");
        return;
    }

    // copy incoming packet into storage for processing
    NET_STORAGE.processing_packet.type = payload[0] >> 4;
    NET_STORAGE.processing_packet.scid = payload[1] & 0x3f;
    for(uint8_t i=0;i<4;i++){
        NET_STORAGE.processing_packet.sedid[i] = payload[i+6];
    }
    uint8_t i=0;
    for(;i < (len-10) && i<MAX_NET_PAYLOAD_SIZE; i++){
        NET_STORAGE.processing_packet.payload[i] = payload[i+10];
    }

  NET_STORAGE.processing_packet.type = payload[0] >> 4;
  NET_STORAGE.processing_packet.scid = payload[1] & 0x3f;

  for(uint8_t i=0;i<4;i++){
    NET_STORAGE.processing_packet.sedid[i] = payload[i+6];
  }

  uint8_t i=0;
  for(;i < (len-10) && i<MAX_NET_PAYLOAD_SIZE; i++){
    NET_STORAGE.processing_packet.payload[i] = payload[i+10];
  }
  NET_STORAGE.processing_packet.len = i;


  printf("Recived message type %d from %d need to run KA\n",NET_STORAGE.processing_packet.type, NET_STORAGE.processing_packet.scid);

  NET_STORAGE.state=SED_PREPROCES;
  process();
*/
}

/**
 * @ section routing
 * @brief routing inside NET layer
 */

void LINKROUTER_error_handler(bool rx, bool toEd, uint8_t* address, uint8_t*data , uint8_t len){
  if(rx){
    printf("LINK error during reciving\n");
  } else {
    printf("LINK error during transmiting\n");
  }
}

/*
 * return true/false depending if layer is busy
 */
bool LINKROUTER_route(uint8_t* payload , uint8_t len){
    if (len < 10) return true;  // or false ?

    //get destination coordinator id ccccdddd|ddssssss c=control, d=dcid, s=scid
    uint8_t dcid = ((payload[0] << 2) & 0b00111100) | ((payload[1] >> 6) & 0b00000011);
    if(dcid == GLOBAL_STORAGE.cid) {  // packet is for me or my childs
        if(zero_address(payload+2)){
            local_process_packet(payload, len);
            //bypass to end point
            return true;
            //return cb_push(payload,len);
        }
        else {
            LINKROUTER_send(true, payload+2, payload, len, true);
        }
    }
    else {  // packet is for different node
        uint8_t next_hop_coord_address = LINK_cid_mask(ROUTING_get_next(dcid));
        LINKROUTER_send(false, &next_hop_coord_address, payload, len, true);
    }
    return true;
}

/**
 * @ section net layer rest
 * @brief net layer functions
 */
void NET_init(){

    //TODO remove what is not used
    NET_STORAGE.direct_join.is_set = false;
    GLOBAL_STORAGE.pair_mode = false;

    cb_init();
    GLOBAL_STORAGE.routing_enabled = true;

    NET_STORAGE.state = SED_READY_STATE;
    NET_STORAGE.join_request.empty = 1;
    NET_STORAGE.move_request.empty = 1;

    for(int i=0; i<MAX_SLEEPY_MESSAGES; i++) {
      NET_STORAGE.sleepy_messages[i].valid = false;
    }
    for(int i=0; i<32; i++) {
      GLOBAL_STORAGE.devices[i].valid = false;
    }
    if(!load_device_table()) {
        D_NET cout << "NET_init(): cannot load device table from " \
                   << GLOBAL_STORAGE.device_table_path << endl;
    }
	load_routing_table();

  // test
  //uint8_t addr[] = {3,4,5,6};
  //uint8_t msg[] = {0,1,2,3,4,5,6,7,8,9};
  //push_sleepy_message(addr, msg, 10);
}

void NET_joining_enable(){

  GLOBAL_STORAGE.pair_mode = true;
}


void NET_joining_disable(){

  GLOBAL_STORAGE.pair_mode = false;
}

bool NET_joined(){
  for(uint8_t i=0;i<4;i++){
    if(GLOBAL_STORAGE.nid[i] != 0){
      return true;
    }
  }
  return false;
}

void NET_join(){
 // LINK_send_join_request(NULL,0);
}

/*
void _net_send_joinreq_topan(){

    uint8_t join_message[MAX_NET_PAYLOAD_SIZE];

    join_message[0] = PTNETEXT_JOIN_REQUEST;
    join_message[1] = NET_STORAGE.join_request.from[0];
    join_message[2] = NET_STORAGE.join_request.from[1];
    join_message[3] = NET_STORAGE.join_request.from[2];
    join_message[4] = NET_STORAGE.join_request.from[3];
    join_message[5] = NET_STORAGE.join_request.signal_strenght;
    //join_message[6] = NET_STORAGE.join_request.len; this value can be simply computed on reciver

    uint8_t join_message_position=6;
    for(uint8_t i=0; i<NET_STORAGE.join_request.len;i++){
      join_message[join_message_position++] = NET_STORAGE.join_request.payload[i];
    }

    NET_STORAGE.join_request.empty = 1;
    send(PT_NETWORK_EXTENDED,
         0,
         NET_DIRECT_COORD,
         join_message,
         join_message_position, true);
}


void _net_send_movereq_topan(){

    uint8_t move_message[MAX_NET_PAYLOAD_SIZE];

    move_message[0] = PTNETEXT_MOVE_REQUEST;
    move_message[1] = NET_STORAGE.move_request.from[0];
    move_message[2] = NET_STORAGE.move_request.from[1];
    move_message[3] = NET_STORAGE.move_request.from[2];
    move_message[4] = NET_STORAGE.move_request.from[3];
    move_message[5] = NET_STORAGE.move_request.signal_strenght;
    //move_message[6] = NET_STORAGE.move_request.len; this value can be simply computed on reciver

    uint8_t move_message_position=6;
    for(uint8_t i=0; i<NET_STORAGE.move_request.len;i++){
      move_message[move_message_position++] = NET_STORAGE.move_request.payload[i];
    }

    NET_STORAGE.move_request.empty = 1;
    send(PT_NETWORK_EXTENDED,
         0,
         NET_DIRECT_COORD,
         move_message,
         move_message_position,
         true);
}


void _net_user_data(){

    static uint8_t user_state = 0;

    user_state = NET_recived(NET_STORAGE.processing_packet.scid,
                                         NET_STORAGE.processing_packet.sedid,
                                         NET_STORAGE.processing_packet.payload,
                                         NET_STORAGE.processing_packet.len,
                                         user_state);

    if(user_state == 255) {
        user_state = 0;
        NET_STORAGE.state = SED_READY_STATE;
    }
}


void _net_user_ack() {

    NET_STORAGE.state = SED_USER_DATA;

    send(PT_NETWORK_ACK,
         NET_STORAGE.processing_packet.scid,
         NET_STORAGE.processing_packet.sedid,
         NULL,
         0,
         true);

}
*/

bool NET_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len, bool with_ack){
    //return send(NET_PACKET_TYPE_DATA ,tocoord, toed, data, len, with_ack);
    //return send(NET_PACKET_TYPE_DATA ,tocoord, toed, data, len, with_ack);
    for(uint8_t i=0; i<len; i++) {
        printf("%02x ", data[i]);
    }

	 send(PT_DATA, tocoord, toed, data, len, false);
	 return true;

    D_NET print_device_table();
    D_NET print_sleepy_message_table();
    //if (!is_my_device(toed) && !array_cmp(toed, NET_DIRECT_COORD, 4)) {

	// send dato to COORD
	if (is_coord_device(toed)) {
		send(PT_DATA, tocoord, toed, data, len, false);
		return true;
	}

    if (!is_my_device(toed)) {
        D_NET printf("NET_send(): device %02x %02x %02x %02x is not in my table, ignoring\n", \
                     toed[0], toed[1], toed[2], toed[3]);
        return false;
    }
    if(is_sleepy_device(toed)) {
        D_NET printf("NET_send(): to sleepy device %02x %02x %02x %02x\n", \
                     toed[0], toed[1], toed[2], toed[3]);
        push_sleepy_message(toed, data, len);
    }
}
