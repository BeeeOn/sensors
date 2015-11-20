#include "link.h"

// link packet description
#define LINK_ACK_MASK          0b11000000
#define LINK_ACK1_MASK         0b10000000
#define LINK_ACK2_MASK         0b01000000
#define LINK_ADDRESS_TYPE_ED   0b00100000
#define LINK_ACK_ADDRESS_TYPE  0b00010000
#define LINK_BUSY_MASK         0b00001000
#define LINK_MOVE_MASK         0b00000100

// Link packet types
enum LINK_packet_type {LINK_DATA_TYPE=0, LINK_COMMIT_TYPE=1,
                       LINK_ACK_TYPE=2, LINK_COMMIT_ACK_TYPE=3};

// new defines
// link data types
#define LINK_DATA_HS            0x00
#define LINK_DATA_WITHOUT_ACK   0x01
#define LINK_DATA_BROADCAST     0x02
#define LINK_DATA_JOIN_REQUEST  0x03
#define LINK_DATA_JOIN_RESPONSE 0x04
#define LINK_BUSY               0x08

// ack data types
#define LINK_ACK_JOIN_REQUEST   0x01

// tx packet states                       
enum LINK_tx_packet_state {DATA_SENT=0, COMMIT_SENT=1};                       


#define ADDRESS_COORD 0
#define ADDRESS_ED 1


#define LINK_JOIN_CONTROL_REQUEST  0b00000000
#define LINK_JOIN_CONTROL_RESPONSE 0b10000000 
#define LINK_JOIN_CONTROL_TYPE     0b10000000
                                                   

typedef struct {
    uint8_t data[MAX_PHY_PAYLOAD_SIZE];
    uint8_t empty:1;
    uint8_t len:6;
    uint8_t expiration_time;
}LINK_rx_buffer_record_ed_t;


typedef struct {
    uint8_t data[MAX_PHY_PAYLOAD_SIZE];
    uint8_t empty:1;
    uint8_t len:6;
    uint8_t state:1;                   //1 = commit message state, 0= data message state
    uint8_t expiration_time;
    uint8_t transmits_to_error;
}LINK_tx_buffer_record_ed_t;


struct LINK_storage_t {
    uint8_t tx_max_retries;
    uint8_t rx_data_commit_timeout;  
    uint8_t timer_counter;
    LINK_rx_buffer_record_ed_t ed_rx_buffer;
    LINK_tx_buffer_record_ed_t ed_tx_buffer;
    bool link_ack_join_received;
} LINK_STORAGE;


/*
 * support functions
 */
bool equal_arrays(uint8_t* addr1,uint8_t* addr2, uint8_t size){
	DEBUG(ED, 0, "equal_arrays");
    return addr1[0] == addr2[0] && addr1[1] == addr2[1] && \
           addr1[2] == addr2[2] && addr1[3] == addr2[3];
} 

bool zero_address(uint8_t* addr){
	DEBUG(ED, 1, "zero_address");
    // yes I listen about for but this get equal or less memory and less time
    return addr[0]==0 && addr[1]==0 && addr[2]==0 && addr[3]==0;
}

bool array_cmp(uint8_t *array_a, uint8_t *array_b, uint8_t len) {  // compare two uint8_t arrays  
    for(uint8_t i=0; i<len; i++) {
        if(array_a[i] != array_b[i]) return false;    
    }
    DEBUG(ED, 2, "array_cmp");
    return true;
}

void array_copy(uint8_t *src, uint8_t *dst, uint8_t len) {
    for(uint8_t i=0; i<len; i++) {
        dst[i] = src[i];        
    }
    DEBUG(ED, 3, "array_copy");
}
uint8_t LINK_cid_mask (uint8_t address){
	DEBUG(ED, 4, "LINK_cid_mask");
    return address & 0x3f; //0b00111111 keep only cid address
}


/*
 * Generate packet header
 */                      
void gen_header(uint8_t *header_placement_pointer, uint8_t packet_type, uint8_t headerPayload){ 
    DEBUG(ED, 5, "gen_header");
    // on link layer ED can send packe only as ED to parent cid
    uint8_t header_index=0;
    header_placement_pointer[header_index++] = packet_type << 6 | /*(toEd //=false//?1:0)*/ 0 << 5 | /*(asEd//=true//?1:0)*/1 << 4 | (headerPayload & 0x0f);  
    //copy nid
    for(uint8_t i=0;i<4;i++){
        header_placement_pointer[header_index++] = GLOBAL_STORAGE.nid[i];
    }
    //as ed can send packet only to parent_cid (does not matter when sending broadcast)
    header_placement_pointer[header_index++] = GLOBAL_STORAGE.parent_cid;    
    for(uint8_t i=0;i<4;i++){
        header_placement_pointer[header_index++] = GLOBAL_STORAGE.edid[i];
    }
}

// send data
void send_data(uint8_t* payload, uint8_t payload_len){
	DEBUG(ED, 6, "send_data");
    uint8_t packet[63];
    uint8_t packet_index=10; //size of link header    
    gen_header(packet, LINK_DATA_TYPE, 0);
    for(uint8_t index=0;index<payload_len;index++){
        packet[packet_index++] = payload[index];
    }
    PHY_send_with_cca(packet,packet_index);
}

// send ack
void send_ack(){
	DEBUG(ED, 7, "send_ack");
    uint8_t ack_packet[10];           
    gen_header(ack_packet,  LINK_ACK_TYPE, 0);
    PHY_send_with_cca(ack_packet,10);
}

// send commit
void send_commit(){
	DEBUG(ED, 8, "send_commit");
    uint8_t commit_packet[10];        
    gen_header(commit_packet, LINK_COMMIT_TYPE, 0);
    PHY_send_with_cca(commit_packet,10);
}

// send commit ack
void send_commit_ack(){
	DEBUG(ED, 9, "send_commit_ack");
    uint8_t commit_ack_packet[10];    
    gen_header(commit_ack_packet, LINK_COMMIT_ACK_TYPE, 0);
    PHY_send_with_cca(commit_ack_packet,10);
}

/*
 * Local process packet
 */
void ed_packet_process(uint8_t* data, uint8_t len){
	DEBUG(ED, 10, "ed_packet_process");
    uint8_t packet_type = data[0] >> 6;
    uint8_t packet_desc = data[0] & 0x0F;                       
    D_LINK printf("Incomming packet: type 0x%02x, desc: 0x%02x\n", packet_type, packet_desc);
                   
    // process tx packet (ACK and COMMIT_ACK message)
    if(packet_type == LINK_ACK_TYPE) {             
        if(!LINK_STORAGE.ed_tx_buffer.empty){
            LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;  // opposite side answers - set timeout timer again                           
            if(packet_desc != LINK_BUSY) {
                LINK_STORAGE.ed_tx_buffer.state = COMMIT_SENT;
                LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3;  //2 +1 actual can arrive immediately, biger waiting is not problem
                send_commit();               
            }                  
        }
    }
    else if(packet_type == LINK_COMMIT_ACK_TYPE) {
        LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;  // opposite side answers, set timeout timer again         
        if(packet_desc != LINK_BUSY) {
            LINK_STORAGE.ed_tx_buffer.empty = 1;
            LINK_notify_send_done();            
        }
    }        
    // process rx packet (DATA and COMMIT message)  
    else if(packet_type == LINK_DATA_TYPE) {
    
        if (packet_desc == LINK_DATA_WITHOUT_ACK) {  // data transfer like UDP, no ACK              
            LINK_process_packet(data + 10, len - 10); 
        }                
        else if(packet_desc == LINK_DATA_HS && LINK_STORAGE.ed_rx_buffer.empty)  {  // data transfer 4-way handshake  
            // device can accept only one packet in time             
            uint8_t i;                                 
            for(i=0;i<len && i<MAX_PHY_PAYLOAD_SIZE;i++){
                LINK_STORAGE.ed_rx_buffer.data[i]=data[i];
            }
            LINK_STORAGE.ed_rx_buffer.len=i;
            LINK_STORAGE.ed_rx_buffer.empty=0;
            LINK_STORAGE.ed_rx_buffer.expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
            send_ack();
        }            
    }
    else if(packet_type == LINK_COMMIT_TYPE) {    
        if(!LINK_STORAGE.ed_rx_buffer.empty) {
            //message now can be accepted
              LINK_process_packet(LINK_STORAGE.ed_rx_buffer.data + 10 ,LINK_STORAGE.ed_rx_buffer.len - 10);
              LINK_STORAGE.ed_rx_buffer.empty=1;
              send_commit_ack();          
            
        } else {
            send_commit_ack();
        }                   
    }
}


/*
 * Periodic check of rx and tx buffers 
 */
void check_buffers_state (){
    //check ed rx buffer
    if((!LINK_STORAGE.ed_rx_buffer.empty) && LINK_STORAGE.ed_rx_buffer.expiration_time == LINK_STORAGE.timer_counter){
        LINK_error_handler(true,LINK_STORAGE.ed_rx_buffer.data,LINK_STORAGE.ed_rx_buffer.len);
        LINK_STORAGE.ed_rx_buffer.empty = 1;
    }    
    //check ed tx buffer
    if((!LINK_STORAGE.ed_tx_buffer.empty) && LINK_STORAGE.ed_tx_buffer.expiration_time == LINK_STORAGE.timer_counter){
        D_LINK printf("check_buffers_state(): retransmit %d\n", LINK_STORAGE.ed_tx_buffer.transmits_to_error);        
        if((LINK_STORAGE.ed_tx_buffer.transmits_to_error--) != 0){
            //if commitment message state          
            if(LINK_STORAGE.ed_tx_buffer.state == COMMIT_SENT){ 
                D_LINK printf("check_buffers_state(): retransmit commit\n");
                send_commit();
            }//else data message state
            else{
                D_LINK printf("check_buffers_state(): retransmit data\n");
                send_data(LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
            }                                                                                    
            //reshedule transmit
            LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 2;              
        } else {                          
            LINK_error_handler(false,LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
            LINK_STORAGE.ed_tx_buffer.empty = 1;            
        }
    }
}

/*
 * implemented function from phy layer
 */
void PHY_process_packet(uint8_t* data,uint8_t len){
    DEBUG(ED, 12, "PHY_process_packet");
    //packet too schort to hold link packet        
    if(len <= 7) return;                               
    //packet not for ed
    if(!(data[0] & LINK_ADDRESS_TYPE_ED)) return;
    //packet for ed but not for me
    if(!equal_arrays(data+5,GLOBAL_STORAGE.edid,4)) return;
                    
    // handle join packets before NID check
    if((data[0] & 0x0F) == LINK_ACK_JOIN_REQUEST &&
        (data[0] >> 6) == LINK_ACK_TYPE) 
    {
        if(GLOBAL_STORAGE.waiting_pair_response) {
            D_LINK printf("RX: LINK_ACK_JOIN_REQUEST\n");            
            LINK_STORAGE.link_ack_join_received = true;
        }
        return;
    }
    if((data[0] & 0x0F) == LINK_DATA_JOIN_RESPONSE &&
        (data[0] >> 6) == LINK_DATA_TYPE) 
    {   
        D_LINK printf("RX: LINK_DATA_JOIN_RESPONSE waiting pair: %d\n", GLOBAL_STORAGE.waiting_pair_response, GLOBAL_STORAGE.waiting_pair_response);
        if(GLOBAL_STORAGE.waiting_pair_response) {
            LINK_join_response_recived(data+10, len-10);
            GLOBAL_STORAGE.waiting_pair_response = false;
        }
        return;
    }
                        
    //packet not in my network
    if(!equal_arrays(data+1,GLOBAL_STORAGE.nid,4)) return;
    // packet not from my CID    
    if (LINK_cid_mask(data[9]) != GLOBAL_STORAGE.parent_cid) return;    
    //process packet
    ed_packet_process(data,len);     
}


void PHY_timer_interupt(void){  
  LINK_STORAGE.timer_counter++;
  check_buffers_state();
  //D_LINK printf("end phy timer interupt\n");  
}


/**
 * @def LINK_init
 * initialize link layer with values from params
 * @param params LINK_init_t values to be set for link layer
 * @return void
 */
void LINK_init(LINK_init_t params){
	DEBUG(ED, 14, "LINK_init");
  LINK_STORAGE.tx_max_retries = params.tx_max_retries;
  LINK_STORAGE.rx_data_commit_timeout = params.rx_data_commit_timeout;
    
  LINK_STORAGE.ed_rx_buffer.empty=1;
  LINK_STORAGE.ed_tx_buffer.empty=1;
  
  //clear timeout counter
  LINK_STORAGE.timer_counter = 0;
  
  LINK_STORAGE.link_ack_join_received = false;  
}       

                
/*
 * LINK_send(). Send link layer packet. Return false in case previous packet is 
 * still in process of sending. Return true if packet was sent to PHY layer. It 
 * does not mean if packet was received by opposite side. As ED it can only send 
 * packet to its parent CID. 
 */
bool LINK_send(uint8_t* payload, uint8_t payload_len, uint8_t transfer_type){
	DEBUG(ED, 15, "LINK_send");
    if (transfer_type == LINK_HS4) {  // 4 way handshake
    
        if(!LINK_STORAGE.ed_tx_buffer.empty) return false;            
        for(uint8_t i=0; i< payload_len;i++){
            LINK_STORAGE.ed_tx_buffer.data[i] = payload[i];
        }
        LINK_STORAGE.ed_tx_buffer.len = payload_len;
        LINK_STORAGE.ed_tx_buffer.state = DATA_SENT;
        LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
        LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately
        LINK_STORAGE.ed_tx_buffer.empty = 0;
        send_data(LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);  
        return true;
    }
    else if (transfer_type == LINK_HS2) {  // 2 way handshake
        return false;
    }
    else if (transfer_type == LINK_NOACK) {
        uint8_t packet[63];
        uint8_t packet_index = 10; //size of link header
        
        gen_header(packet, LINK_DATA_TYPE, LINK_DATA_WITHOUT_ACK);        
        for(uint8_t index=0;index<payload_len;index++){
            packet[packet_index++] = payload[index];
        }
        PHY_send_with_cca(packet,packet_index);
        return true;                          
    }                   
}

/*
 * LINK_send_broadcast(). Send link layer packet to broadcast layer. Suitable to 
 * use when finding out nodes in neighbourhood.
 */
bool LINK_send_broadcast(uint8_t* payload, uint8_t payload_len){ 
	DEBUG(ED, 16, "LINK_send_broadcast");
    uint8_t packet[63];
    uint8_t packet_index = 10; // size of link header
        
    gen_header(packet, LINK_DATA_TYPE, LINK_DATA_BROADCAST);        
    for(uint8_t index=0;index<payload_len;index++){
        packet[packet_index++] = payload[index];
    }
    PHY_send_with_cca(packet,packet_index);
    return true;       
}

/*
 * LINK_send_join_request(). Send link layer join packet.  
 * Return true/false depending on join response was received.
 */
bool LINK_send_join_request(uint8_t* payload, uint8_t payload_len){ 
	DEBUG(ED, 17, "LINK_send_join_request");
    uint8_t packet[63];
    uint8_t packet_index = 10; // size of link header
        
    D_LINK printf("LINK_send_join_request()\n");
    gen_header(packet, LINK_DATA_TYPE, LINK_DATA_JOIN_REQUEST);        
    for(uint8_t index=0;index<payload_len;index++){
        packet[packet_index++] = payload[index];
    }
        
    // TODO cycle to scan over available channels
    // break on first join response
    LINK_STORAGE.link_ack_join_received = false;
    while(!LINK_STORAGE.link_ack_join_received) {
        PHY_send_with_cca(packet,packet_index);
        // delay
        delay_ms(250);  // wait for ack response @TODO set as configuration variable      
        break;
    }
    return LINK_STORAGE.link_ack_join_received;       
}         
