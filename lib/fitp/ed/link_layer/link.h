#ifndef LINK_ED_H 
#define LINK_ED_H 

#include <stdint.h>
#include <stdbool.h>
#include "global.h"
#include "phy.h"
#include "debug.h"

#ifndef X86
bool send_done = false;
#else
static bool send_done = false;
#endif

//Max Message Payload Len, change only if change radio
#define LINK_HEADER_SIZE 10
// MAX_LINK_PAYLOAD_SIZE = 63 - 10 = 53
#define MAX_LINK_PAYLOAD_SIZE ( MAX_PHY_PAYLOAD_SIZE - LINK_HEADER_SIZE )

// types of link transfers passed to LINK_send() function
#define LINK_HS4    0   // send as 4 way handshake (DATA - ACK - COMMIT - COMMIT_ACK)
#define LINK_HS2    1   // send with ACK (DATA - ACK)
#define LINK_NOACK  2   // send without ACK (DATA)



// extern functions
extern void delay_ms(uint16_t t);

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
bool LINK_send_join_request(uint8_t* payload, uint8_t len);



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
bool LINK_send(uint8_t* payload, uint8_t payload_len, uint8_t transfer_type);
bool LINK_send_broadcast(uint8_t* payload, uint8_t payload_len);



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
 * @def LINK_notify_send_done
 * external function called after message is sended out from buffer
 * can be useful for protocol layers builded on top of link library
 * only successfull send will be notified, in case of fail error handler
 * is used as notification function
 * @see LINK_error_handler
 * @warning function is runing inside interupt(if PHY_process_packet is) 
 * @param payload data recived in join response
 * @param len lenght of data
 * @return void
 */
extern void LINK_notify_send_done();



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



#endif
