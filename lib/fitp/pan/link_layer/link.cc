#include "link.h"
#include <stdio.h>
#define LINK_RX_BUFFER_SIZE 4
#define LINK_TX_BUFFER_SIZE 4

//Warning: binary constants are a GCC extension -> replaced

#define LINK_ACK_MASK          0xc0
#define LINK_ACK1_MASK         0x80
#define LINK_ACK2_MASK         0x40
#define LINK_ADDRESS_TYPE      0x20  // 1 - ED, 0 - COORD
#define LINK_ACK_ADDRESS_TYPE  0x10
#define LINK_BUSY_MASK         0x08
#define LINK_MOVE_MASK         0x04
#define LINK_DATA_WITHOUT_ACK 0b00000001

// new defines
// link data types
#define LINK_DATA_HS            0x00
#define LINK_DATA_WITHOUT_ACK   0x01
#define LINK_DATA_BROADCAST     0x02
#define LINK_DATA_JOIN_REQUEST  0x03
#define LINK_DATA_JOIN_RESPONSE 0x04
#define LINK_BUSY               0x08

// ack data types
#define LINK_ACK_JOIN_REQUEST   0x05

/*LINK_COMMAND_MASK is used only in move
1 response, 0 request
#define LINK_COMMAND_MASK      0b00000010 
*/
#define LINK_COMMAND_MASK      0x02

#define ADDRESS_COORD 0
#define ADDRESS_ED 1


#define LINK_JOIN_DEVICE_TYPE_ED 1
#define LINK_JOIN_DEVICE_TYPE_COORD 2

//#define LINK_JOIN_CONTROL_REQUEST  0b00000000
#define LINK_JOIN_CONTROL_REQUEST  0x00
//#define LINK_JOIN_CONTROL_RESPONSE 0b10000000 
#define LINK_JOIN_CONTROL_RESPONSE 0x80 
//#define LINK_JOIN_CONTROL_TYPE     0b10000000
#define LINK_JOIN_CONTROL_TYPE     0x80


#define LINK_RX_BUFFER_SIZE 4
#define LINK_TX_BUFFER_SIZE 4
#define MAX_CHANNEL 31

#include "log.h"

typedef struct {
  uint8_t data[MAX_PHY_PAYLOAD_SIZE];
  uint8_t address_type:1;
  uint8_t empty:1;
  uint8_t len:6;
  uint8_t expiration_time;
  uint8_t seq_number;
  union {
    uint8_t coord;
    uint8_t ed[4];
  } address;
}LINK_rx_buffer_record_t;

typedef struct {
  uint8_t data[MAX_PHY_PAYLOAD_SIZE];
  //uint8_t address_type:1;  allways ed accept only cid
  uint8_t empty:1;
  uint8_t len:6;
  uint8_t expiration_time;
//   union {                             //reciving message only from parent_cid & it is stored in global storage
//     uint8_t coord;
//     //uint8_t ed[4];               allways ed accept only cid
//   } address;
}LINK_rx_buffer_record_ed_t;

typedef struct {
  uint8_t data[MAX_PHY_PAYLOAD_SIZE];
  uint8_t address_type:1;
  uint8_t empty:1;
  uint8_t len:6;
  uint8_t state:1;                   //1 = commit message state, 0= data message state
  uint8_t expiration_time;
  uint8_t transmits_to_error;
  uint8_t seq_number;
  union {
    uint8_t coord;
    uint8_t ed[4];
  } address;
}LINK_tx_buffer_record_t;

typedef struct {
  uint8_t data[MAX_PHY_PAYLOAD_SIZE];
  //uint8_t address_type:1;  allways ed send only cid
  uint8_t empty:1;
  uint8_t len:6;
  uint8_t state:1;                   //1 = commit message state, 0= data message state
  uint8_t expiration_time;
  uint8_t transmits_to_error;
//   union {                             //allways send message to parent_cid & it is stored in global storage
//     uint8_t coord;
//     //uint8_t ed[4];                allways ed send only cid
//   } address;
}LINK_tx_buffer_record_ed_t;


struct LINK_storage_t {
  uint8_t tx_max_retries;
  uint8_t rx_data_commit_timeout;
  
  uint8_t timer_counter;
  LINK_rx_buffer_record_t rx_buffer[LINK_RX_BUFFER_SIZE];
  LINK_tx_buffer_record_t tx_buffer[LINK_TX_BUFFER_SIZE];
  uint8_t tx_seq_number;
  
  LINK_rx_buffer_record_ed_t ed_rx_buffer;
  LINK_tx_buffer_record_ed_t ed_tx_buffer;
} LINK_STORAGE;

extern uint8_t PHY_get_channel(void);
/*
 * support functions
 */
bool equal_arrays(uint8_t* addr1,uint8_t* addr2, uint8_t size){
    return addr1[0] == addr2[0] && addr1[1] == addr2[1] && \
           addr1[2] == addr2[2] && addr1[3] == addr2[3];
} 

bool zero_address(uint8_t* addr){
    // yes I listen about for but this get equal or less memory and less time
    return addr[0]==0 && addr[1]==0 && addr[2]==0 && addr[3]==0;
}

bool array_cmp(uint8_t *array_a, uint8_t *array_b, uint8_t len) {  // compare two uint8_t arrays  
    for(uint8_t i=0; i<len; i++) {
        if(array_a[i] != array_b[i]) return false;    
    }
    return true;
}

void array_copy(uint8_t *src, uint8_t *dst, uint8_t len) {
    for(uint8_t i=0; i<len; i++) {
        dst[i] = src[i];        
    }
}



  uint8_t LINK_cid_mask (uint8_t address);
bool equal_arrays (uint8_t* addr1,uint8_t* addr2,uint8_t size);
bool zero_address (uint8_t* adr1);

uint8_t get_free_index_tx(){
  uint8_t freeIndex = LINK_TX_BUFFER_SIZE;
  
  for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
    if(LINK_STORAGE.tx_buffer[i].empty){
      freeIndex = i;
      break;
    }
  }
  
  return freeIndex;
}

uint8_t get_free_index_rx(){
  uint8_t freeIndex = LINK_RX_BUFFER_SIZE;
  
  for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
    if(LINK_STORAGE.rx_buffer[i].empty){
      freeIndex = i;
      break;
    }
  }
  
  return freeIndex;
}



void gen_header(uint8_t *header_placement_pointer, bool asEd, bool toEd, uint8_t* address,  uint8_t packet_type, uint8_t headerPayload, uint8_t seq){

  if(asEd && toEd) return; //this cant be send LINK layer dosent allow sending ed to ed
  
  header_placement_pointer[0] =  packet_type << 6 | (toEd?1:0) << 5 | (asEd?1:0) << 4 | (headerPayload & 0x0f);
      
  uint8_t header_index=1;
  
  //copy nid
  for(uint8_t i=0;i<4;i++){
    header_placement_pointer[header_index++] = GLOBAL_STORAGE.nid[i];
  }
  

  // sending to end device
  if(toEd){  
    for(uint8_t i=0;i<4;i++){
      //printf("SSSSSSSSSSSSSSSSSSS %d\n", address[i]);
      header_placement_pointer[header_index++] = address[i];
    }    
    //if sending to ed my cid is ack_address
    header_placement_pointer[header_index++] = GLOBAL_STORAGE.cid;
    
  }//sending to coord
  else {

     if(asEd){
       //as ed can send packet only to parent_cid
       header_placement_pointer[header_index++] = GLOBAL_STORAGE.parent_cid;
       
       for(uint8_t i=0;i<4;i++){
        header_placement_pointer[header_index++] = GLOBAL_STORAGE.edid[i];
      }
    }// as coord
     else {
       header_placement_pointer[header_index++] = *address;
       header_placement_pointer[header_index++] = GLOBAL_STORAGE.cid;
       header_placement_pointer[header_index++] = seq;
    }
  }  
}

void send_data(bool asEd, bool toEd, uint8_t* address, uint8_t seq, uint8_t* payload, uint8_t payload_len){
  printf("===== PAN SEND_DATA =====\n");
  uint8_t packet[63];  
  gen_header(packet, asEd, toEd, address,  LINK_DATA_TYPE, 0, seq);     
  
  uint8_t packet_index=10; //size of header
  for(uint8_t index=0;index<payload_len;index++){
    packet[packet_index++] = payload[index];
  }
  /*
  printf("SEND_DATA:\n");
  for(uint8_t i = 0; i < packet_index; i++)
  {
       printf("0x%02x ", packet[i]);
  }
  printf("\n"); */
  PHY_send_with_cca(packet,packet_index);
}

  void send_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq){
  uint8_t ack_packet[10];
  gen_header(ack_packet, asEd, toEd, address,  LINK_ACK_TYPE, 0, seq);
  printf("PAN: send_ack -> send_with_cca\n");
  PHY_send_with_cca(ack_packet,10);
}

  void send_commit(bool asEd, bool toEd, uint8_t* address, uint8_t seq){
  uint8_t commit_packet[10];
  gen_header(commit_packet, asEd, toEd, address,  LINK_COMMIT_TYPE, 0, seq);
  printf("PAN: send_commit\n");
  PHY_send_with_cca(commit_packet,10);
}

  void send_commit_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq){
  uint8_t commit_ack_packet[10];
  gen_header(commit_ack_packet, asEd, toEd, address,  LINK_COMMIT_ACK_TYPE, 0, seq);
  printf("PAN: send_commit_ack -> send_with_cca\n");
  PHY_send_with_cca(commit_ack_packet,10);
}

  void send_busy_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq){
  uint8_t commit_ack_packet[10];
  gen_header(commit_ack_packet, asEd, toEd, address,  LINK_ACK_TYPE, 0x08, seq);
  PHY_send_with_cca(commit_ack_packet,10);
}

  void send_busy_commit_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq){
  uint8_t commit_ack_packet[10];
  gen_header(commit_ack_packet, asEd, toEd, address,  LINK_COMMIT_ACK_TYPE, 0x08, seq);
  PHY_send_with_cca(commit_ack_packet,10);
}



void router_packet_proccess(uint8_t* data,uint8_t len){
  
  printf("router_packet_proccess\n");
  /*for(uint8_t i = 0; i < len; i++)
  {
      printf("%02x ", data[i]);
  }
  printf("\n");*/
    
  //packet type ack & commit_ack
  if(data[0] & LINK_ACK1_MASK){

    printf("  -- ack/commit_ack\n");

    
    // packet is ack
    if(! (data[0] & LINK_ACK2_MASK)){
      printf("  -- ack\n");
      if(data[0] & LINK_ACK_ADDRESS_TYPE){
         printf("  -- ack ed\n");
         for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
           //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
           if(!LINK_STORAGE.tx_buffer[i].empty){
             if(LINK_STORAGE.tx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.tx_buffer[i].address.ed,data+6,4)){
               LINK_STORAGE.tx_buffer[i].state = 1;
               LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 3;//2 +1 actual can arrive immediately, biger waiting is not problem
               printf("  -- ack ed send_commit\n");
               send_commit(false, true, LINK_STORAGE.tx_buffer[i].address.ed, 0);
               break;
             }
           }
         }
      }//else addressed by cid
      else {
        printf("  -- ack coord\n");
        for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
          //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
          if(!LINK_STORAGE.tx_buffer[i].empty) {
            if(LINK_STORAGE.tx_buffer[i].address_type == 0 && LINK_STORAGE.tx_buffer[i].address.coord == data[6] && LINK_STORAGE.tx_buffer[i].seq_number == data[7]){
              LINK_STORAGE.tx_buffer[i].state = 1;
              LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 3; //2 +1 actual can arrive immediately, biger waiting is not problem
               printf("  -- ack coord send_commit\n");
              send_commit(false, false, &LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number);
              break;
            }
          }
        }
      }
      
    }//packet is commit_ack
    else {
      if(data[0] & LINK_ACK_ADDRESS_TYPE){
        for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
          //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
          if(!LINK_STORAGE.tx_buffer[i].empty){
            if(LINK_STORAGE.tx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.tx_buffer[i].address.ed,data+6,4)){
              LINK_STORAGE.tx_buffer[i].empty = 1;
              break;
            }
          }
        }
      }//else addressed by cid
      else {
        for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
         //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
          if(!LINK_STORAGE.tx_buffer[i].empty) {
            if(LINK_STORAGE.tx_buffer[i].address_type == 0 && LINK_STORAGE.tx_buffer[i].address.coord == data[6] && LINK_STORAGE.tx_buffer[i].seq_number == data[7]){
              LINK_STORAGE.tx_buffer[i].empty = 1;
              break;
            }
          }
        }
      }
      printf("  -- packet was commit ack\n");
    }
    
  }//packet type data & commit
  else {
    
    printf("  -- data or commit \n");
    
    //if packet is of type DATA
    if(! (data[0] & LINK_ACK2_MASK)){
      printf("  -- data %x\n", data[0] & 0x0F);
      if((data[0] & 0x0F) == LINK_DATA_WITHOUT_ACK) {
          printf("  -- LINK_DATA_WITHOUT_ACK\n");
          LINKROUTER_route(data+10, len-10);
          return;
      }
      

      //we need have normalized value not only true/false(true can be from N not only 1)
      uint8_t sender_address_type = (data[0] & LINK_ACK_ADDRESS_TYPE)?1:0;
      bool need_save = true;
      // ========================== PRIDANO - OPRAVA C. 1!!! ===================
      uint8_t empty_index=get_free_index_rx();
      //printf("empty_index: %d\n", empty_index);      

      printf("  -- data saving\n");
      //if there is some free space in buffer
      if( empty_index != LINK_RX_BUFFER_SIZE){
        printf("  -- have space\n");
        //save 
        //data with header
        //uint8_t data_index=0,index=0;
        uint8_t data_index=0;
        //for(;data_index<len && index<MAX_PHY_PAYLOAD_SIZE;data_index++){
          //LINK_STORAGE.rx_buffer[empty_index].data[index++]=data[data_index];
          for(;data_index<len && data_index<MAX_PHY_PAYLOAD_SIZE;data_index++){
            LINK_STORAGE.rx_buffer[empty_index].data[data_index]=data[data_index];
        }
        //LINK_STORAGE.rx_buffer[empty_index].len=index;
				LINK_STORAGE.rx_buffer[empty_index].len=data_index;
        LINK_STORAGE.rx_buffer[empty_index].address_type=sender_address_type;
        LINK_STORAGE.rx_buffer[empty_index].empty=0;
        LINK_STORAGE.rx_buffer[empty_index].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
        if(sender_address_type){
          for(uint8_t i=0;i<4;i++){
            LINK_STORAGE.rx_buffer[empty_index].address.ed[i] = data[6+i];
          }
        } else {
          LINK_STORAGE.rx_buffer[empty_index].address.coord = LINK_cid_mask(data[6]);
          LINK_STORAGE.rx_buffer[empty_index].seq_number = data[7];
        }
	// send ack
	//send_ack(false, sender_address_type,data+6,0);
      }else{
        send_busy_ack(false, sender_address_type,data+6,data[7]);
      }
      // ======================================================================
      //addressed by edid, try to find it
      if(sender_address_type){
         for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
           //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
           if(!LINK_STORAGE.rx_buffer[i].empty){
             if(LINK_STORAGE.rx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.rx_buffer[i].address.ed,data+6,4)){
               need_save = false;
               printf("  -- packet found -> ack\n");
               send_ack(false, sender_address_type,data+6,0);
               LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
               return;
             }
           }
         }
      }//else addressed by cid
       else {
         for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
           //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
           if(!LINK_STORAGE.rx_buffer[i].empty) {
             if(LINK_STORAGE.rx_buffer[i].address_type == 0 && LINK_STORAGE.rx_buffer[i].address.coord == data[6] && LINK_STORAGE.rx_buffer[i].seq_number == data[7]){
               need_save = false;
               printf("  -- packet found -> ack coord\n");
               send_ack(false, sender_address_type,data+6,data[7]);
               LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
               return;
             }
           }
         }
       }
       
      //if message allredy in buffer function return alredy 
      /*uint8_t empty_index=get_free_index_rx();
      //printf("empty_index: %d\n", empty_index);      

      printf("  -- data saving\n");
      //if there is some free space in buffer
      if( empty_index != LINK_RX_BUFFER_SIZE){
        printf("  -- have space\n");
        //save 
        //data with header
        uint8_t data_index=0,index=0;
        for(;data_index<len && index<MAX_PHY_PAYLOAD_SIZE;data_index++){
          LINK_STORAGE.rx_buffer[empty_index].data[index++]=data[data_index];
        }
        LINK_STORAGE.rx_buffer[empty_index].len=index;

        LINK_STORAGE.rx_buffer[empty_index].address_type=sender_address_type;
        LINK_STORAGE.rx_buffer[empty_index].empty=0;
        LINK_STORAGE.rx_buffer[empty_index].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
        if(sender_address_type){
          for(uint8_t i=0;i<4;i++){
            LINK_STORAGE.rx_buffer[empty_index].address.ed[i] = data[6+i];
          }
        } else {
          LINK_STORAGE.rx_buffer[empty_index].address.coord = LINK_cid_mask(data[6]);
          LINK_STORAGE.rx_buffer[empty_index].seq_number    = data[7];
        }*/
    }//packet is commit message
    else{
      bool not_busy = true;
      printf("  -- commit\n");
      //addressed by edid, try to find it
      if(data[0] & LINK_ACK_ADDRESS_TYPE){
         for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
           //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
           printf("    -- for %d\n",LINK_STORAGE.rx_buffer[i].empty);
           if(!LINK_STORAGE.rx_buffer[i].empty){
             printf("      -- not empty\n");
             if(LINK_STORAGE.rx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.rx_buffer[i].address.ed,data+6,4)){ 
               if ( get_free_index_tx() == LINK_TX_BUFFER_SIZE ) {
                //send busy and renew expiration time 
                printf("  -- commit send busy back\n");
                send_busy_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
              } else {
                //send packet to next coordinator commit and set packet state to processed(empty)
                printf("  -- commit routing, addressed by edid\n");
                // SMEROVANI KLARA
		            /*send_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                LINKROUTER_route(LINK_STORAGE.rx_buffer[i].data+10,LINK_STORAGE.rx_buffer[i].len-10);
                LINK_STORAGE.rx_buffer[i].empty=1;
                ===============================================================*/
                // SMEROVANI KUBA
                if (LINKROUTER_route(LINK_STORAGE.rx_buffer[i].data+10,LINK_STORAGE.rx_buffer[i].len-10)){
                  send_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                  LINK_STORAGE.rx_buffer[i].empty=1;
                } else {
                  send_busy_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                  LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
                }
                // ================================================================== 
              }
              printf("    -- break\n");
              break;
             }
           }
         }
      }//else addressed by cid
      else {
        for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
          printf("    -- for %d\n",LINK_STORAGE.rx_buffer[i].empty);
          //if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
          if(!LINK_STORAGE.rx_buffer[i].empty) {
            printf("      -- not empty\n");
            if(LINK_STORAGE.rx_buffer[i].address_type == 0 && LINK_STORAGE.rx_buffer[i].address.coord == LINK_cid_mask(data[6]) && LINK_STORAGE.rx_buffer[i].seq_number == data[7]){
              if ( get_free_index_tx() == LINK_TX_BUFFER_SIZE ) {
                //send busy and renew expiration time 
                printf("  -- commit send busy back\n");
                send_busy_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
              } else {
                //send packet to next coordinator commit and set packet state to processed(empty)
                printf("  -- commit routing, addressed by cid\n");
                // SMEROVANI KUBA
                if (LINKROUTER_route(LINK_STORAGE.rx_buffer[i].data+10,LINK_STORAGE.rx_buffer[i].len-10)){
                   send_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                   LINK_STORAGE.rx_buffer[i].empty=1;
                } else {
                  send_busy_commit_ack(false, data[0] & LINK_ACK_ADDRESS_TYPE,data+6,data[7]);
                  LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
                }
                // ============================================================
              }
              printf("    -- break\n");
              break;
            }
          }
        }
      }
    }
  }
}



  void check_buffers_state (){
  //check ed buffers
  if((!LINK_STORAGE.ed_rx_buffer.empty) && LINK_STORAGE.ed_rx_buffer.expiration_time == LINK_STORAGE.timer_counter){
      LINK_STORAGE.ed_rx_buffer.empty = 1;
      LINK_error_handler(true,LINK_STORAGE.ed_rx_buffer.data,LINK_STORAGE.ed_rx_buffer.len);
  }
  
  if((!LINK_STORAGE.ed_tx_buffer.empty) && LINK_STORAGE.ed_tx_buffer.expiration_time == LINK_STORAGE.timer_counter){
    //if commitment message state
    printf("to tx erriii %d\n",LINK_STORAGE.ed_tx_buffer.transmits_to_error);
    if((LINK_STORAGE.ed_tx_buffer.transmits_to_error--) == 0){
      LINK_error_handler(false,LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
      LINK_STORAGE.ed_tx_buffer.empty = 1;
    } else {
      if (LINK_STORAGE.ed_tx_buffer.state){      
        send_commit(true, false, NULL,0);
      }//else data message state
      else{
        send_data(true, false,NULL,0,LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
      }
      //reshedule transmit
      LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 2;
    }
  }

  
  
  //check coord buffers
  for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
    //is record after expiration 
    if((!LINK_STORAGE.rx_buffer[i].empty) && LINK_STORAGE.rx_buffer[i].expiration_time == LINK_STORAGE.timer_counter){
      //report rx error
      if (LINK_STORAGE.rx_buffer[i].address_type){
          LINKROUTER_error_handler(true, true,
                                  LINK_STORAGE.rx_buffer[i].address.ed,
                                  LINK_STORAGE.rx_buffer[i].data,
                                  LINK_STORAGE.rx_buffer[i].len);
      }else{
          LINKROUTER_error_handler(true, false,
                                  &LINK_STORAGE.rx_buffer[i].address.coord,
                                  LINK_STORAGE.rx_buffer[i].data, 
                                  LINK_STORAGE.rx_buffer[i].len);
      }
      
      LINK_STORAGE.rx_buffer[i].empty = 1;
    }
  }
  
  for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
    if((!LINK_STORAGE.tx_buffer[i].empty) && LINK_STORAGE.tx_buffer[i].expiration_time == LINK_STORAGE.timer_counter){
      //if sending in commit state
       printf("to tx errooo %d\n", LINK_STORAGE.tx_buffer[i].transmits_to_error);
       if((LINK_STORAGE.tx_buffer[i].transmits_to_error--) == 0){
        if (LINK_STORAGE.tx_buffer[i].address_type){
            LINKROUTER_error_handler(false, true,
                                    LINK_STORAGE.tx_buffer[i].address.ed,
                                    LINK_STORAGE.tx_buffer[i].data,
                                    LINK_STORAGE.tx_buffer[i].len);
        }else{
            LINKROUTER_error_handler(false, false,
                                    &LINK_STORAGE.tx_buffer[i].address.coord,
                                    LINK_STORAGE.tx_buffer[i].data, 
                                    LINK_STORAGE.tx_buffer[i].len);
        }
        LINK_STORAGE.tx_buffer[i].empty = 1;
      } else {
      
        if (LINK_STORAGE.tx_buffer[i].state){
          if (LINK_STORAGE.tx_buffer[i].address_type){
            send_commit(false, true, LINK_STORAGE.tx_buffer[i].address.ed, 0);
          }else{
            send_commit(false, false, &LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number);
          }
        }//else data message state
        else{
          if (LINK_STORAGE.tx_buffer[i].address_type){
            send_data(false, true,LINK_STORAGE.tx_buffer[i].address.ed, 0, LINK_STORAGE.tx_buffer[i].data,LINK_STORAGE.tx_buffer[i].len);
          }else{
            send_data(false, false,&LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number, LINK_STORAGE.tx_buffer[i].data,LINK_STORAGE.tx_buffer[i].len);
          }
        }
        //reshedule transmit
        LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 2;
      }
    }
  }
}

/*
 * Send join response directly to joining device
 */ 
void LINK_send_join_response(uint8_t *to, uint8_t* payload, uint8_t len){
    // uint8_t packet[25]; -> otestovat
    uint8_t packet[63];
    uint8_t packet_index=10;
    
    D_LINK printf("LINK_send_join_response() to %02x %02x %02x %02x\n", 
                  to[0], to[1], to[2], to[3]);
        
    gen_header(packet, false, true, to,  LINK_DATA_TYPE, LINK_DATA_JOIN_RESPONSE, 0);        
    for(uint8_t i=0;i<len && packet_index < MAX_PHY_PAYLOAD_SIZE; i++){
        packet[packet_index++] = payload[i];
    }

    PHY_send_with_cca(packet, packet_index);
}



/*
 * process data from phy layer
 */
void PHY_process_packet(uint8_t* data,uint8_t len) {    
  
    //packet too short to hold link packet
    if(len <= 7) return;
         
    // handle join packets before NID and CID check 
    if((data[0] & 0x0F) == LINK_DATA_JOIN_REQUEST && (data[0] >> 6) == LINK_DATA_TYPE) {
        uint8_t ack_packet[10];
                
        D_LINK printf("LINK_DATA_JOIN_REQUEST from: %02x %02x %02x %02x\n", \
                       data[6], data[7], data[8], data[9]);    
        gen_header(ack_packet, false, true, data+6,  LINK_ACK_TYPE, LINK_ACK_JOIN_REQUEST, 0);    
        PHY_send_with_cca(ack_packet,10);
        LINK_join_request_received(0, data+10, len-10);  // @TODO get rssi    
        return;
    }
    
    for (uint8_t i=0; i< len; i++) {
        printf("%02x ", data[i]);
    }
    printf(" NID ");
    for (uint8_t i=0; i<4; i++) {
        printf("%02x ", GLOBAL_STORAGE.nid[i]);
    }
    
   
    // packet not in my network
    if(!equal_arrays(data+1,GLOBAL_STORAGE.nid,4)) return; 

	if((data[0] & 0x0F) == LINK_DATA_WITHOUT_ACK)
		printf("===================== WITHOUT_ACK =====================\n");
	
    // packet to PAN's ED can be sent only from parent_cid, PAN does not have any parent    
    if((data[0] & LINK_ADDRESS_TYPE)) return;    
    // dont recive messages for coordinator if routing is disabled and data are recived
    // must recive other messages because there can be opened handshakes and they will 
    // failed if not    
    if(!GLOBAL_STORAGE.routing_enabled && (data[0] & LINK_ACK_MASK)==0 ) { 
        D_NET printf("PHY_process_packet(): router disabled\n");
        return;
    }    
    // packet for different coord
    if(LINK_cid_mask(data[5]) != GLOBAL_STORAGE.cid) return;    
        
    // if packet is from cid verify it - this should be in NET layer
    if(! (data[0] & LINK_ACK_ADDRESS_TYPE) ){
        uint8_t sender_cid=LINK_cid_mask(data[6]);      
        // packet must be from my child
        if(GLOBAL_STORAGE.routing_tree[GLOBAL_STORAGE.cid] != sender_cid &&
           GLOBAL_STORAGE.routing_tree[sender_cid] != GLOBAL_STORAGE.cid)
        {
            D_NET printf("PHY_process_packet(): invalid source router\n");
            return;
        } 
    }    
    router_packet_proccess(data,len);
}

void PHY_timer_interupt(void){
  LINK_STORAGE.timer_counter++;  
  check_buffers_state();
}

/*
 * implementation
 */

//! cid mask defined in routing.c too
  uint8_t LINK_cid_mask (uint8_t address){
  return address & 0x3f; //0b00111111 keep only cid address
}

void LINK_init(LINK_init_t params){
  
  LINK_STORAGE.tx_max_retries = params.tx_max_retries;
  LINK_STORAGE.rx_data_commit_timeout = params.rx_data_commit_timeout;
  
  for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
    LINK_STORAGE.rx_buffer[i].empty=1;
  }
  for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
    LINK_STORAGE.tx_buffer[i].empty=1;
  }
  
  LINK_STORAGE.ed_rx_buffer.empty=1;
  LINK_STORAGE.ed_tx_buffer.empty=1;
  
  //clear timeout counter
  LINK_STORAGE.timer_counter = 0;
  //clear tx sequence number
  LINK_STORAGE.tx_seq_number = 0;
  
}


void LINK_send_move_request(uint8_t* payload, uint8_t len){
  
  printf("send move request");
  uint8_t packet[MAX_PHY_PAYLOAD_SIZE];
  gen_header(packet, true, false, NULL, LINK_DATA_TYPE, LINK_MOVE_MASK, 0);
  
  uint8_t packet_index = 10;
  for(uint8_t i=0; i< len; i++){
    packet[packet_index++] = payload[i];
  }
  PHY_send_with_cca(packet, packet_index);
  
  printf("send move request done");
}

void LINK_send_move_response(uint8_t *to, uint8_t* payload,uint8_t len){
  
  uint8_t packet[63];
  
  //control
  //adress type ed, packet type move, move type response
  //cpu more expensive variant gen_header(packet, false, true, to, LINK_DATA_TYPE, (LINK_MOVE_MASK | LINK_COMMAND_MASK) , 0)
  packet[0]=LINK_ADDRESS_TYPE | LINK_MOVE_MASK | LINK_COMMAND_MASK;
  //nid
  packet[1]=GLOBAL_STORAGE.nid[0];
  packet[2]=GLOBAL_STORAGE.nid[1];
  packet[3]=GLOBAL_STORAGE.nid[2];
  packet[4]=GLOBAL_STORAGE.nid[3];
  //to
  packet[5]=to[0];
  packet[6]=to[1];
  packet[7]=to[2];
  packet[8]=to[3];
  //from not defined routing may not be routing_enabled
  packet[9]=0;
  
  uint8_t packet_index=10;
  
  for(uint8_t i=0;i<len && packet_index < MAX_PHY_PAYLOAD_SIZE; i++){
    packet[packet_index++] = payload[i];
  }

  PHY_send_with_cca(packet,packet_index);
}


bool LINK_send(uint8_t* payload, uint8_t payload_len){
  if(!LINK_STORAGE.ed_tx_buffer.empty) return false;
  
  for(uint8_t i=0; i< payload_len;i++){
    LINK_STORAGE.ed_tx_buffer.data[i] = payload[i];
  }
  LINK_STORAGE.ed_tx_buffer.len = payload_len;
  LINK_STORAGE.ed_tx_buffer.state = 0;
  LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
  LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately
  LINK_STORAGE.ed_tx_buffer.empty = 0;
  send_data(true, false, NULL, 0, LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);  
  //send_data(false, true, NULL, 0, LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
  
  return true;
}

  
bool LINK_send_to_ed(uint8_t* address, uint8_t* payload, uint8_t payload_len){
  if(!LINK_STORAGE.ed_tx_buffer.empty) return false;
  
  for(uint8_t i=0; i< payload_len;i++){
    LINK_STORAGE.ed_tx_buffer.data[i] = payload[i];
  }
  LINK_STORAGE.ed_tx_buffer.len = payload_len;
  LINK_STORAGE.ed_tx_buffer.state = 0;
  LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
  LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately  
  LINK_STORAGE.ed_tx_buffer.empty = 0;
  send_data(false, true, address, 0, LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
  return true;
}


bool LINK_send_without_ack(uint8_t* payload, uint8_t payload_len, uint8_t* address_next_coord)
{
        uint8_t packet[63];
        
        gen_header(packet, false, true, address_next_coord,  LINK_DATA_TYPE, LINK_DATA_WITHOUT_ACK, 0); 
        uint8_t packet_index=10; //size of header
        for(uint8_t index=0;index<payload_len;index++){
             packet[packet_index++] = payload[index];
         }           
         PHY_send_with_cca(packet,packet_index);
         return true;
}

            
bool LINKROUTER_send(bool toEd, uint8_t* address, uint8_t* payload, uint8_t payload_len, bool ack){
  //////////
  if (ack == true) {
    uint8_t packet[63];
    gen_header(packet, false, true, address,  LINK_DATA_TYPE, LINK_DATA_WITHOUT_ACK, 0);
    
    uint8_t packet_index=10; //size of header
    for(uint8_t index=0;index<payload_len;index++){
        packet[packet_index++] = payload[index];
    }
    PHY_send_with_cca(packet,packet_index);    
    return true;
  }
  //////////
  
  uint8_t freeIndex = get_free_index_tx();
  printf("===== LINKROUTER_send =====\n"); 
  if(freeIndex == LINK_TX_BUFFER_SIZE) return false;
  for(uint8_t i=0; i< payload_len;i++){
    LINK_STORAGE.tx_buffer[freeIndex].data[i] = payload[i];
  } 
  LINK_STORAGE.tx_buffer[freeIndex].len = payload_len;
  LINK_STORAGE.tx_buffer[freeIndex].seq_number = LINK_STORAGE.tx_seq_number++;  //save sequence number to packet and increment link storage variable
  if(toEd){
    for(uint8_t i=0;i < 4;i++)
    LINK_STORAGE.tx_buffer[freeIndex].address.ed[i] = address[i];
  } else {
    LINK_STORAGE.tx_buffer[freeIndex].address.coord = *address;
  }
  LINK_STORAGE.tx_buffer[freeIndex].address_type = toEd?ADDRESS_ED:ADDRESS_COORD;
  LINK_STORAGE.tx_buffer[freeIndex].state = 0;
  LINK_STORAGE.tx_buffer[freeIndex].transmits_to_error = LINK_STORAGE.tx_max_retries;
  LINK_STORAGE.tx_buffer[freeIndex].expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately
  LINK_STORAGE.tx_buffer[freeIndex].empty = 0; 
  
  if (LINK_STORAGE.tx_buffer[freeIndex].address_type){
    send_data(false, true,LINK_STORAGE.tx_buffer[freeIndex].address.ed, 0, LINK_STORAGE.tx_buffer[freeIndex].data,LINK_STORAGE.tx_buffer[freeIndex].len);
  }else{
    send_data(false, false,&LINK_STORAGE.tx_buffer[freeIndex].address.coord, LINK_STORAGE.tx_buffer[freeIndex].seq_number, LINK_STORAGE.tx_buffer[freeIndex].data,LINK_STORAGE.tx_buffer[freeIndex].len);
  }
  
  return true;
}



