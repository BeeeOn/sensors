#include "phy.h"
#include "constants.h"
#include "link.h"
#include "net.h"
#include "fitp.h"
#include "include.h"

void fitp_init(){
    NET_init();
}


bool fitp_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len){        
    return NET_send(tocoord, toed, data, len);
}


bool fitp_join(){
  return NET_join();
}


bool fitp_joined(){
  return NET_joined();
}


uint8_t NET_recived(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part){
  return fitp_recived(from_cid, from_edid, data, len, part);
}

bool NET_accept_join(uint8_t nid[4], uint8_t parent_cid){
  return fitp_accept_join(nid, parent_cid);
}

uint8_t fitp_maxDataLenght(){
  return MAX_NET_PAYLOAD_SIZE;
}
