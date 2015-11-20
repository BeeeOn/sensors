#ifndef LINK_PAN_H 
#define LINK_PAN_H 

#include <stdint.h>
#include <stdbool.h>
#include "global.h"
#include "phy.h"
#include "debug.h"

//Max Message Payload Len, change only if change radio
#define LINK_HEADER_SIZE 10
#define MAX_LINK_PAYLOAD_SIZE ( MAX_PHY_PAYLOAD_SIZE - LINK_HEADER_SIZE )


/****************************************************************************************
 * 
 * ENDPOINT LINK INTERFACE
 * 
 ****************************************************************************************/

enum LINK_packet_type {LINK_DATA_TYPE=0, LINK_COMMIT_TYPE=1, LINK_ACK_TYPE=2, LINK_COMMIT_ACK_TYPE=3};

/**
 * @def LINK_init_t
 * @brief init params structure
 */
typedef struct{
  uint8_t tx_max_retries;        //!< number of retries to send packet, after all retrise are gone error handler will be called
  uint8_t rx_data_commit_timeout;//!< lenght of timeout while device is waiting for completition of packet transfer, after timeout done error handler will be called
}LINK_init_t;



/**
 * @def LINK_init
 * initialize link layer with values from params
 * @param params LINK_init_t values to be set for link layer
 * @return void
 */
void LINK_init(LINK_init_t params);



/**
 * @def LINK_cid_mask
 * normalize address value
 * @param address to normalize
 * @return uint8_t cid
 */
  uint8_t LINK_cid_mask (uint8_t address);



/**
 * @def LINK_send_join_request
 * @warning message can be blocked by network if network is not in pair mode
 * @brief send join request using radio ( its broadcast to all devices in range )
 * this message request network for access point to device may connect,
 * request ist send to PAN(directly or trought coordinators) it check for best aviable
 * access point and response message with id of coordinator to which device may be connected
 * if network have disabled joining new device cant be added to network so if you want to get
 * new access point from network(f.e. after lost connection to parent coordinator) you should use 
 * LINK_send_move_request
 * @see LINK_send_move_request
 * @return void
 */
void LINK_send_join_request(uint8_t* payload, uint8_t len);



/**
 * @def LINK_join_response_recived
 * external function called after reciving response from network
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @param payload data recived in join response
 * @param len lenght of data
 * @return void
 */
extern void LINK_join_response_recived(uint8_t* payload,uint8_t len);



/**
 * @def LINK_send_move_request
 * @brief send move request using radio ( its broadcast to all devices in range )
 * if you want join to network juse LINK_send_join_request
 * this function may be called if parent coordinator is often busy or disconnected
 * after this message is recived by network its send to PAN, it check for best aviable access points
 * for this device and send response with new parent coordinator value
 * message is allways accepted by network
 * @see LINK_send_join_request
 * @pre device joined to network (global_storage parent cid must be set)
 * @return void
 */
void LINK_send_move_request(uint8_t* payload, uint8_t len);



/**
 * @def LINK_move_response_recived
 * external function called after reciving response from network
 * this can be recived as response for move request or as command from netowrk to change
 * parent coordinator
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @param payload data recived in join response
 * @param len lenght of data
 * @return void
 */
extern void LINK_move_response_recived(uint8_t* payload,uint8_t len);



/**
 * @def LINK_send
 * send data to parent coordinator
 * @param payload data to be send
 * @param lenght  lenght of data
 * @pre layer initialized using LINK_init
 * @pre device joined to network (global_storage parent cid must be set)
 * @see GLOBAL_STORAGE
 * @return bool false if outgoing buffer is full else true
 */
bool LINK_send(uint8_t* payload, uint8_t payload_len);



/**
 * @def LINK_process_packet
 * external function called after reciving packet from network
 * this function is called after reciving packet from phy packet and successfull handshake
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @param payload data recived in join response
 * @param len lenght of data
 * @return void
 */
extern void LINK_process_packet (uint8_t* payload, uint8_t len);



/**
 * @def LINK_is_device_busy
 * external function called after reciving commit on currently recived packet 
 * deterine state of device, may return true if device is ready to revice packet
 * false if device is doing some long term network job
 * can be useful for protocol layers builded on top of link library to do its jobs
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @return bool true packet can be recived, 
 *              false device is busy send busy messige to sender of currently commited packet
 */
extern bool LINK_is_device_busy();




/** 
 * @def LINK_error_handler
 * external function called after error occured during reciving or sending data
 * this function is called after reciving packet from phy packet and unsuccessfull handshake
 * @warning function is runing inside interupt(if PHY_timer_interupt is) 
 * @param rx bool true if error during reciving, false if during sending
 * @param payload data from original message
 * @param len lenght of original message
 * @return void
 */ 
extern void LINK_error_handler(bool rx, uint8_t* payload , uint8_t len);




/****************************************************************************************
 * 
 * COORDINATOR SPECIFIC INTERFACE
 * 
 ****************************************************************************************/


/** 
 * @def LINK_join_request_recived
 * external function called after join request recived
 * @see LINK_join_response_recived
 * @param signal_strenght uint8_t signal strenght after recive
 * @param from uint8_t[4] address of join request sender
 * @param payload data for join (will be passed to LINK_join_response_recived as payload)  
 * @param len lenght of payloaded data
 * @return void
 */
extern void LINK_join_request_received(uint8_t signal_strenght, uint8_t* payload, uint8_t len);



/** 
 * @def LINK_send_join_response
 * send respond to not joined device with info about joining
 * @see LINK_join_response_recived
 * @param to uint8_t[4] endpoint address of device
 * @param payload data for join (will be passed to LINK_join_response_recived as payload)  
 * @param len lenght of payloaded data
 * @return void
 */
void LINK_send_join_response(uint8_t *to, uint8_t* data,uint8_t len);


/** 
 * @def LINK_move_request_recived
 * external function called after join request recived
 * @see LINK_join_response_recived
 * @param signal_strenght uint8_t signal strenght after recive
 * @param from uint8_t[4] address of join request sender
 * @param payload data for join (will be passed to LINK_join_response_recived as payload)  
 * @param len lenght of payloaded data
 * @return void
 */
extern void LINK_move_request_recived(uint8_t signal_strenght, uint8_t* from, uint8_t* payload, uint8_t len);

/** 
 * @def LINK_send_move_response
 * send respond to device in network but another parent id ( normal network message can be recived only from parent on ed)
 * @see LINK_move_response_recived
 * @param to uint8_t[4] endpoint address of device
 * @param payload data for join (will be passed to LINK_join_response_recived as payload)  
 * @param len lenght of payloaded data
 * @return void
 */
void LINK_send_move_response(uint8_t *to, uint8_t* payload,uint8_t len);



/**
 * @def LINKROUTER_send
 * send data to ed or coordinator using router buffers
 * @param payload data to be send
 * @param lenght  lenght of data
 * @pre layer initialized using LINK_init
 * @pre device joined to network (global_storage parent cid must be set)
 * @pre empty space in buffer
 * @see GLOBAL_STORAGE
 * @return bool false if outgoing buffer is full else true
 */
bool LINKROUTER_send(bool toEd, uint8_t* address, uint8_t* payload, uint8_t payload_len, bool ack);



/**
 * @def LINKROUTER_route
 * external function called after reciving packet from network on coordinator
 * this function is called after reciving packet from phy packet and successfull handshake
 * and tx coordinator buffer have free space
 * @warning only one space in output buffer is quaranted to be free in this function
 * that means that only one send is quaranted to be successfull
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @warning to send packet may be used LINKROUTER_send (if you want to send packet back to network using link_layer)
 * @see LINKROUTER_send
 * @pre GLOBAL_STORAGE.routing_tree must be set to proper values
 * @pre routing enabled, may route old packets (from buffers) if it is set to false but new packet
 * will not be recived in link layer for router
 * @param payload data recived in join response
 * @param len lenght of data
 * @return bool true
 */
extern bool LINKROUTER_route(uint8_t* payload , uint8_t len);

/** 
 * @def LINKCOORD_error_handler
 * external function called after error occured during reciving or sending data on coordinator
 * this function is called after reciving packet from phy packet and unsuccessfull handshake
 * @warning function is runing inside interupt(if PHY_timer_interupt is) 
 * @param rx bool true if error during reciving, false if during sending
 * @param toEd bool is message for/from coordinator 
 * @param address (toed?uint8_t[4]:&uint8_t)endpoint/coordinator address
 * @param payload data from original message
 * @param len lenght of original message
 * @return void
 */ 
extern void LINKROUTER_error_handler(bool rx, bool toEd, uint8_t* address, uint8_t*data , uint8_t len);

bool LINK_send_without_ack(uint8_t* payload, uint8_t payload_len, uint8_t* address_next_coord);
bool LINK_send_to_ed(uint8_t* address, uint8_t* payload, uint8_t payload_len);
#endif
