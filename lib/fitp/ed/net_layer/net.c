#include "net.h"
#include "net_common.h"
#include "link.h"                          
#include "log.h"


typedef struct{
  uint8_t type;
  uint8_t scid;
  uint8_t sedid[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE];
  uint8_t len;
  
} NET_current_processing_packet_t;

struct NET_storage_t {
    uint8_t dr_state;  // data request state
  NET_current_processing_packet_t processing_packet;
  uint8_t state;
  uint8_t user_state;
} NET_STORAGE;

enum LINK_packet_type {
    DR_ACK_WAITING=0,   // waiting for data request ack 
    DR_DATA_WAIT=1,       // received data request wait 
    DR_GO_SLEEP=2,      // received data request sleep
    DR_DATA_RECEIVED=3        // received dr data
};


/*
 * Supporting function exists also on link or phy layers
 */
extern bool equal_arrays(uint8_t* addr1,uint8_t* addr2, uint8_t size);
extern bool zero_address(uint8_t* addr);
extern bool array_cmp(uint8_t *array_a, uint8_t *array_b, uint8_t len);
extern void array_copy(uint8_t *src, uint8_t *dst, uint8_t len);
  

bool send(uint8_t message_type, uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len, uint8_t transfer_type){
    uint8_t tmp[53];
    uint8_t index=0;
    
    tmp[index++]=(message_type << 4) | (tocoord >> 4);
    tmp[index++]=(tocoord << 4) | (GLOBAL_STORAGE.parent_cid & 0x3f);
    tmp[index++]=toed[0];
    tmp[index++]=toed[1];
    tmp[index++]=toed[2];
    tmp[index++]=toed[3];
    tmp[index++]=GLOBAL_STORAGE.edid[0];
    tmp[index++]=GLOBAL_STORAGE.edid[1];
    tmp[index++]=GLOBAL_STORAGE.edid[2];
    tmp[index++]=GLOBAL_STORAGE.edid[3];
    
    for(uint8_t i=0;i<len && i<53;i++){
      tmp[index++]=data[i];
    }
    
    LINK_send(tmp, index, transfer_type); 
    return false;
      
    //return LINK_send_without_ack(tmp,index,0);
    //return LINK_send(tmp,index);
    //return LINK_send(tmp, index, LINK_NOACK);  
    //return LINK_send_broadcast(tmp, index);
    //GLOBAL_STORAGE.waiting_pair_response = true;
    //LINK_send_join_request(tmp, index));    
}


/*
 * Join response received also on net layer
 */
void LINK_join_response_recived(uint8_t* payload, uint8_t len){
  
    if (len < 15) return;  // check valid lenght           
    if (!equal_arrays(GLOBAL_STORAGE.edid, payload+2, 4)) return;  // not for me
  
    if (NET_accept_join(payload + 10, payload[14])) {
        GLOBAL_STORAGE.nid[0] = payload[10]; 
        GLOBAL_STORAGE.nid[1] = payload[11]; 
        GLOBAL_STORAGE.nid[2] = payload[12]; 
        GLOBAL_STORAGE.nid[3] = payload[13];    
        GLOBAL_STORAGE.parent_cid = payload[14];
        D_NET printf("LINK_join_response_recived(): NID %02x %02x %02x %02x parent %02x\n", \
                       GLOBAL_STORAGE.nid[0], GLOBAL_STORAGE.nid[1], \
                       GLOBAL_STORAGE.nid[2], GLOBAL_STORAGE.nid[3], \
                       GLOBAL_STORAGE.parent_cid);
        
        //save new configuration to eeprom
        uint8_t cfg[5];
        cfg[0] = GLOBAL_STORAGE.nid[0];
        cfg[1] = GLOBAL_STORAGE.nid[1];
        cfg[2] = GLOBAL_STORAGE.nid[2];
        cfg[3] = GLOBAL_STORAGE.nid[3];
        cfg[4] = GLOBAL_STORAGE.parent_cid;    
        save_configuration(cfg, 5);    
    }
}

void LINK_move_response_recived(uint8_t* payload, uint8_t len){
  
  GLOBAL_STORAGE.parent_cid = LINK_cid_mask(payload[0]);
  printf("moved to [%d] ... \n",GLOBAL_STORAGE.parent_cid);
  //process rest of payload if needed
}


void LINK_error_handler(bool rx, uint8_t* data, uint8_t len){
  if(rx){
    printf("LINK error during reciving\n");
  } else {
    printf("LINK error during transmiting\n");
  }
}



void LINK_notify_send_done(){
  //process();
}

/*bool LINK_is_device_busy(){
  return NET_STORAGE.state != SED_READY_STATE;
}
*/

void LINK_process_packet(uint8_t* payload, uint8_t len){
    if(len < 10) return; // invalif net layer packet
    
    uint8_t type = payload[0] >> 4;  
    uint8_t dcid = ((payload[0] << 2) & 0x3C) | ((payload[1] >> 6) & 0x03); 
    uint8_t scid = payload[1] & 0x3f;
    uint8_t dedid[4];
    uint8_t sedid[4];
    uint8_t net_payload[MAX_NET_PAYLOAD_SIZE];
    
    array_copy(payload+2, dedid, 4);
    array_copy(payload+6, sedid, 4);
    
    D_NET printf("LINK_process_packet(): type %02x dcid %02x scid %02x\n", \
                  type, dcid, scid); 
    D_NET printf("LINK_process_packet(): sedid %02x %02x %02x %02x dedid %02x %02x %02x %02x\n", \
                 sedid[0], sedid[1], sedid[2], sedid[3], dedid[0], dedid[1], dedid[2], dedid[3]);
    
    switch (type) {        
        case PT_DATA_ACK_DR_WAIT:
            NET_STORAGE.dr_state = DR_DATA_WAIT;
            break;
        case PT_DATA_ACK_DR_SLEEP:
            NET_STORAGE.dr_state = DR_GO_SLEEP;
            break;
        case PT_DATA:
            if (GLOBAL_STORAGE.sleepy_device && NET_STORAGE.dr_state == DR_DATA_WAIT) {
                NET_STORAGE.dr_state = DR_DATA_RECEIVED;
            }
            NET_recived(scid, sedid, payload+10, len-10, 0);
            break;
        default:
            break;
    }
}


void NET_init() {
    //TODO load it from eeprom if have valid
    uint8_t cfg[5];
    load_configuration(cfg, 5);    
   
    if (cfg[0]==0 && cfg[1]==0 && cfg[2]==0 && cfg[3]==0) {
        GLOBAL_STORAGE.nid[0] = 1;  // undefined edid
    } else {
        GLOBAL_STORAGE.nid[0] = cfg[0];
        GLOBAL_STORAGE.nid[1] = cfg[1];
        GLOBAL_STORAGE.nid[2] = cfg[2];
        GLOBAL_STORAGE.nid[3] = cfg[3];
        GLOBAL_STORAGE.parent_cid = cfg[4];
    }

    NET_STORAGE.user_state = 0;
    NET_STORAGE.state = SED_READY_STATE;
}

bool NET_joined(){
  return ! zero_address(GLOBAL_STORAGE.nid);
}

/*
 * Send message depending if it is sleepy device.
 * @tocoord - coordinator id
 * @toed - end device id, if NULL send directly tocoord
 */
bool NET_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len){
    // @TODO use #define to choose link_transfer
    uint8_t toedid[4];
    uint8_t i;
    
    if (toed != NULL) array_copy(toed, toedid, 4);
    else array_copy(NET_DIRECT_COORD, toedid, 4);
    
    D_NET printf("NET_send(): tocoord %d toedid %02x %02x %02x %02x\n", \
                  tocoord, toedid[0], toedid[1], toedid[2], toedid[3]);
                      
    if (GLOBAL_STORAGE.sleepy_device) {
        NET_STORAGE.dr_state = DR_ACK_WAITING;
        send(PT_DATA_DR, tocoord, toedid, data, len, LINK_NOACK);
        
        for(i=0; i<10 && NET_STORAGE.dr_state == DR_ACK_WAITING; i++) {  
            // wait i * delay_ms() for ack
            delay_ms(10);
        }
        if(NET_STORAGE.dr_state == DR_ACK_WAITING) {            
            D_NET printf("NET_send(): DR_ACK_WAITING timeout\n");
            return false;
        }
        if(NET_STORAGE.dr_state == DR_GO_SLEEP) {
            D_NET printf("NET_send(): DR_GO_SLEEP received\n");
            return true;
        }
        NET_STORAGE.dr_state = DR_DATA_WAIT;
        for (i=0; i<100 && NET_STORAGE.dr_state == DR_DATA_WAIT; i++) {  
            // wait i * delay_ms() for sleepy messages data
            delay_ms(10);
        }
        if(NET_STORAGE.dr_state == DR_DATA_RECEIVED) {
            D_NET printf("NET_send(): sleepy data received\n");
            return true;
        }
        else {
            D_NET printf("NET_send(): sleepy data timeout\n");
            return false;
        }
    }
    else {
        return send(PT_DATA, tocoord, toedid, data, len, LINK_NOACK);
    }                              
}
     
/*
 * Send join request
 */
bool NET_join(){    
    uint8_t tmp[53];
    uint8_t index=0;
    
    if (GLOBAL_STORAGE.waiting_pair_response) return false;
    
    tmp[index++]=(PT_DATA_JOIN_REQUEST << 4) & 0xF0;
    if (GLOBAL_STORAGE.sleepy_device) {  // second byte in join request packet 
        tmp[index++]=0xFF;               // is used as a sleepy flag
    } 
    else {
        tmp[index++]=0x00;
    }
    tmp[index++]=0;  // dest ed == {0,0,0,0}
    tmp[index++]=0;
    tmp[index++]=0;
    tmp[index++]=0;
    tmp[index++]=GLOBAL_STORAGE.edid[0];
    tmp[index++]=GLOBAL_STORAGE.edid[1];
    tmp[index++]=GLOBAL_STORAGE.edid[2];
    tmp[index++]=GLOBAL_STORAGE.edid[3];
    
     
    GLOBAL_STORAGE.waiting_pair_response = true;    
    if (LINK_send_join_request(tmp, index)) {
        D_NET printf("NET_join(): link ack request received\n");
        for (uint8_t i=0; i<100; i++) {  // wait for i * delay_ms()
            delay_ms(10);
            if (GLOBAL_STORAGE.waiting_pair_response == false) {
                break;
            }                        
        }
    }    
    
    if (GLOBAL_STORAGE.waiting_pair_response) {
        D_NET printf("NET_join(): timeout\n");
        GLOBAL_STORAGE.waiting_pair_response = false;
        return false;
    }     
    
    D_NET printf("NET_join(): success\n");
    return true;
}
