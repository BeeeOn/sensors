#include <stdint.h>
#include <stdbool.h>

#ifndef FITP_LAYER_H 
#define FITP_LAYER_H 

// for simplify sending directly to coordinator 
#define FITP_DIRECT_COORD (uint8_t*)"\x00\x00\x00\x00"

#define FITP_RECIVING_START 0
#define FITP_RECIVING_DONE 255

/**
 * @brief initialize network layer before use
 */
void fitp_init();


/**
 * @brief send data to another device
 * 
 * @param tocoord [6b] address of coordinator to which EndDevice is connected.
 *                0 is reserved for PAN coordinator, if you dont know coord address
 *                of EndDevice send it to pan, PAN will rerout message to corect address
 *                prefered option is to send message directly(it can save resources)
 * 
 * @param toed    [4B] address of destination EndDevice
 *                special option is send to enddevice [0,0,0,0] this is interpreted as
 *                message directly for coordinator(message is processed on EndDevice part
 *                of coordinator)
 * 
 * @param data    [*] data which will be send to another device
 * 
 * @param len     lenght of data
 *                @see fitp_maxDataLenght();
 */
bool fitp_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len);


/**
 * @brief ask networks for connection
 */
bool fitp_join();

/**
 * @brief determine state of joining device in to network 
 * @return true if is connected else false
 */
bool fitp_joined();


/**
 * @brief extern method called with recived packet, for stack optimalisation was data and sender euid connected to one pointer
 * @warning function is called inside interupt
 * @warning only one send is allowed inside this function because only one free space is guaranted in outgoing buffer. 
 * All interupts are blocked because function is called inside high priority interupt. For this reason buffers can't be freed parallely.
 * Aviable solution:
 * for solving this problem ther is parameter part where is stored return value from previous NET_recived call so you can split 
 * your code into chunks and use this variable as index, and return next chunk index from function
 * if you return 255 a.k.a FITP_RECIVING_DONE user handling is done and function will never be called again with this packet
 * on first function call value of part variable is 0 a.k.a FITP_RECIVING_START 
 * @param from_cid uint8_t address of sender parent coordinator
 * @param from_edid uint8_t* 4byte address of sender followed with payload array
 * @param data uint8_t* packet payload
 * @param len uint8_t lenght of recived payload
 * @param part uint8_t in which part user code may continue after send, on begining FITP_RECIVING_START {0x00} is set
 * @return uint8_t next part where function may continue look warning description, value for all done is FITP_RECIVING_DONE {0xff}
 */
extern uint8_t fitp_recived(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part);


/**
 * @brief extern method called after reciving pair response from network
 * function purpose is to let device chose if connect to specific network
 * @example another_network device is in place where multiple networks listening for 
 * pair, so after send pair request it will recive multiple responses, but we want to
 * join only one specific network, so inside this function we can wait for user interaction
 * and if they dont select this network we can skip pair(save this network id somewhere and check another responses)
 * 
 * @param nid id of network where we will be connected
 * @param parent_cid id of coordinator where we will be connected(only informative value, if you refuse network by parent_cid value network probably dont offer you new value)
 * 
 * @return true if want join network, false if refuse it
 */
extern bool fitp_accept_join(uint8_t nid[4], uint8_t parent_cid);

/**
 * @brief return maximal lenght of packet in fitp 
 * this value is computed in compiletiome as max lenght of radio packet 
 * minus all required headers for fitp
 * @return max lenght of packet
 */
uint8_t fitp_maxDataLenght();

#endif
