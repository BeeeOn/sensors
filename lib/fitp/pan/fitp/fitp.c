#include "fitp.h"
#include "net.h"


void fitp_init(){
  //NET_init();
}


bool fitp_send(uint8_t tocoord, uint8_t *toed, uint8_t* data, uint8_t len){    
  return NET_send(tocoord, toed, data, len, true);
}


bool fitp_async_send(uint8_t tocoord, uint8_t *toed, uint8_t* data, uint8_t len){
	//Opravit na NET_async_send
  //NET_send(tocoord, toed, data, len, true);

}


bool fitp_join(){
  NET_join();
}

void fitp_enable_joining(){
  GLOBAL_STORAGE.pair_mode= true;
}

void fitp_disable_joining(){
  GLOBAL_STORAGE.pair_mode= false;
}

bool fitp_joined(){
  return NET_joined();
}





