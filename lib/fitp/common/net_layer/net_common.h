#ifndef COMMON_NET_LAYER_H 
#define COMMON_NET_LAYER_H 

/**
 * @warning values of enums are defined by hand an may never be changed. 
 * this values can be used on old devices what using this protocol and change
 * DESTROY BACK COMPATIBILITY.
 */
 
#define NET_DIRECT_COORD (uint8_t*)"\0\0\0\0"


/**
 * @brief structure describing net layer message type
 * @warning
 * From back compatibility reason this value cant be changed
 */
enum packet_type_e { 
  PT_DATA = 0x00,
  PT_DATA_DR = 0x01, // data request
  PT_DATA_NACK = 0x02,
  PT_DATA_JOIN_REQUEST = 0x03, // join request
  PT_DATA_ACK = 0x04,
  PT_DATA_ACK_DR_WAIT = 0x05,
  PT_DATA_ACK_DR_SLEEP = 0x06,
  PT_DATA_JOIN_RESPONSE = 0x07, // join response
  PT_DATA_UNJOIN = 0x08,
  PT_SLEEP_DATA_ENC = 0x09,
  PT_SLEEP_WAKED_UP = 0x0A,
  PT_SLEEP_EXTENDED = 0x0B,
  PT_NETWORK_NACK = 0x0C,
  PT_NETWORK_NACK_ENC = 0x0D,
  PT_NETWORK_ACK = 0x0E,
  PT_NETWORK_EXTENDED = 0x0F
};


/**
 * @brief structure describing additional type in PT_NETWORK_EXTENDED packet type
 * @see packet_type_e
 * @warning only appending this enum is alloved
 */
enum packet_network_subtype_e { 
  PTNETEXT_JOIN_REQUEST, 
  PTNETEXT_JOIN_RESPONSE,
  PTNETEXT_MOVE_REQUEST,
  PTNETEXT_MOVE_RESPONSE,
  PTNETEXT_SET_CHANNEL
};




/**
 * @brief structure describing additional type in PT_SLEEP_EXTENDED packet type
 * @see packet_type_e
 * @warning only appending this enum is alloved
 */
enum packet_sleep_subtype_e { 
  PTSLEEP_QUERY,
  PTSLEEP_ACK,
  PTSLEEP_FAILED
};



/**
 * @brief names of net layer states, values can be overlaped and all values
 * can be changed without effect on compatibility
 * SED=State End device
 * SCD=State CoorDinator
 */
enum net_layer_states_e {
  SED_READY_STATE, 
  SED_PREPROCES,   //state to be set when need to switch processing by packet type
  SED_USER_DATA_ACK,
  SED_USER_DATA,
  SCD_SENDJOIN_TO_PAN,
  SCD_SENDMOVE_TO_PAN
};



#endif