#ifndef NET_LAYER_H 
#define NET_LAYER_H 

#include <stdint.h>
#include <stdbool.h>
#include "global.h"
#include "link.h"
#include "debug.h"


#define NET_PACKET_TYPE_DATA 0
#define NET_PACKET_TYPE_JOIN 1

//TODO what was this
#define NET_JOIN_REQUEST 0 



#define NET_RECIVED_DONE 255
#define NET_RECIVED_START 0

//Max Message Payload Len, change only if change radio
#define NET_HEADER_SIZE 10
#define MAX_NET_PAYLOAD_SIZE ( MAX_LINK_PAYLOAD_SIZE - NET_HEADER_SIZE )
#define MAX_SLEEPY_MESSAGES 10

//TODO move this away, not public functions of net layer
extern   void save_configuration(uint8_t *buf, uint8_t len);
extern   void load_configuration(uint8_t *buf, uint8_t len);

/**
 * @def NET_init()
 * @brief initialize network layer before use
 */
void NET_init();

/**
 * @def NET_send();
 * @brief send data to another device
 * blocking version of send on network layer function return after packet is sended to next device in network
 * in case thet sending failed for some reason message is sended to error handler
 * @param tocoord uint8_t coordinator id of destionation device
 * @param toed    uint8_t[4] destionation endpoint address 
 * @param data    uint8_t* data to be send max lenght is MAX_NET_PAYLOAD_SIZE
 * @param len     uint8_t  lenght of data
 * @return true if message successfuly inserted to transmition buffer else false
 */
//bool NET_send(uint8_t tocoord, const uint8_t *toed, const uint8_t* data, uint8_t len, bool with_ack);
bool NET_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len, bool with_ack);

/**
 * @def NET_unpair
 * @brief send unpair packet to a device and delete its record from device table. 
 */
bool NET_unjoin(uint8_t* ed);

/**
 * @def NET_joining_enable
 * @brief send join to all devices
 */
void NET_joining_enable();

/**
 * @def NET_joining_disable
 * @brief send join to all devices
 */
void NET_joining_disable();

/**
 * @def NET_join
 * @brief broadcast join request
 */
void NET_join();

/**
 * @def NET_joined
 * @brief determine state of joining device in to network 
 * @return true if is connected else false
 */
bool NET_joined();


/**
 * @def NET_received
 * @brief extern method called with recived packet, for stack optimalisation was data and sender euid connected to one pointer
 * @warning function is called inside interupt
 * @warning only one send is allowed inside this function because only one free space is guaranted in outgoing buffer and 
 * all interapts is blocked because function can be called inside high priority interupt so buffers can be freed pararerly
 * so if you call send multiple times it may return false and if you try send in loop until true is returned this would never happend !
 * Aviable solution:
 * for solving this problem ther is parameter part where is stored return value from previous NET_recived call so you can split 
 * your code into chunks and use this variable as index, and return next chunk index from function
 * if you return 255 a.k.a NET_RECIVED_DONE user handling is done and function will never be called again with this packet
 * on first function call value of part variable is 0 a.k.a NET_RECIVED_START 
 * @param from_cid uint8_t address of sender parent coordinator
 * @param from_edid uint8_t* 4byte address of sender followed with payload array
 * @param data uint8_t* packet payload
 * @param len uint8_t lenght of recived payload
 * @param part uint8_t in which part user code may continue after send, on begining NET_RECIVED_START {0x00} is set
 * @return uint8_t next part where function may continue look warning description, value for all done is NET_RECIVED_OK {0xff}
 */
extern uint8_t NET_received(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part);


/**
 * @def NET_accept_join
 * @brief extern method called after reciving pair response from network
 * function purpose is to let device chose if connect to specific network
 * @example another_network device is in place where multiple networks listening for 
 * pair, so after send pair request it will recive multiple responses, but we want to
 * join only one specific network, so inside this function we can wait for user interaction
 * and if they dont select this network we can skip pair(save this network id somewhere and check another responses)
 * @see NET_hook_onJoin
 * 
 * @param nid id of network where we will be connected
 * @param parent_cid id of coordinator where we will be connected(only informative value, if you refuse network by parent_cid value network dont offer you new value)
 * 
 * @return uint8_t valid cid address if want to connect else 255
 */
//extern uint8_t NET_device_join(uint8_t nid[4], uint8_t parent_cid, const uint8_t* data, const uint8_t len);
extern bool NET_accept_join(uint8_t nid[4], uint8_t parent_cid);


#endif
