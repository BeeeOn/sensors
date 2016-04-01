#include "glib.h"
#include <stdlib.h> 
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

#include "constants.h"
#include "phy.h"

#include "mqtt_iface.h"
#include "simulator.h"
#include "global.h"

using namespace std; 

//process synchronization during send
bool waiting_send = false;
std::mutex m, mm, mmm;
std::condition_variable cv;


struct PHY_storage_t {
  
    uint8_t mode;
    uint8_t channel;
    uint8_t band;
    uint8_t bitrate;
    uint8_t power;
    uint8_t send_done;  //if send wait forever on test of this variable ignoring value change add volatile to disable optimalization on this variable
    uint8_t recived_packet[MAX_PHY_PAYLOAD_SIZE];
    uint8_t cca_noise_treshold;
    
    //PAN coordinator specific
    std::thread irq_interupt_deamon;
    std::thread timer_interupt_generator;
    bool irq1_enabled = false;
    bool irq0_enabled = false;

    bool terminate_timer = false;
    
} PHY_STORAGE;


/*
 * support functions
 */

void    set_register(uint8_t address, uint8_t value);
uint8_t get_register(uint8_t address);
void    write_fifo  (uint8_t data);
void    set_rf_mode (uint8_t mode);
bool    set_power   (uint8_t power);
bool    set_bitrate (uint8_t bitrate);
bool    set_chanel_freq_rate (uint8_t channel, uint8_t offsetFreq,uint8_t bitrate);
void    send_reload_radio ();
inline uint8_t get_cca_noise();

/*
 * 
 */

void HW_irq0occurred(void);
void HW_irq1occurred(void);




/*
CONSTANT TABLE
                             BAND_863             BAND863_C950                BAND_902                 BAND_915 */
const uint16_t _start_freq[] = {860,                   950,                      902,                   915};     
uint16_t _chanel_spacing[] = {  384,                   400,                      400,                   400};  

uint16_t _channelCompare(uint8_t band, uint8_t channel, uint8_t bitrate){
  uint32_t freq = (uint32_t)_start_freq[band]*1000;
  if ((band==BAND_863 || band==BAND_863_C950) && !(bitrate==DATA_RATE_100 || bitrate==DATA_RATE_200)){
    freq += channel * 300;
  } else{
    freq += channel * _chanel_spacing[band];
  }    
  uint32_t chanelCompareTmp =(freq * 808);
  return (uint16_t) (chanelCompareTmp/((uint32_t) 9*FXTAL)); 
}

uint8_t chanelAmount(uint8_t band, uint8_t bitrate){
  if ((band==BAND_863 || band==BAND_863_C950) && (bitrate==DATA_RATE_100 || bitrate==DATA_RATE_200)){
   return  25;
  } else{
    return 32;
  }
}


uint8_t rValue(){
  return 100;
}

uint8_t pValue(uint8_t band, uint8_t chanel, uint8_t bitrate){  

}


void set_register(const uint8_t address, const uint8_t data){
  
}

uint8_t get_register(uint8_t address) {
  
}

uint8_t read_fifo(void) {
    
}

void write_fifo(const uint8_t data){
  
}

void set_rf_mode(uint8_t mode) {

}

bool set_chanel_freq_rate (uint8_t channel, uint8_t band,uint8_t bitrate){

}

bool set_bitrate (uint8_t bitrate) {
    uint8_t datarate;
    uint8_t bandwidth;
    uint8_t freq_dev;
    uint8_t filcon_set;
    if (bitrate > DATA_RATE_200) {
        return false;
    }

    switch (bitrate)
    {
        case DATA_RATE_5:
            datarate = BITRATE_5;
            bandwidth = BW_50;
            freq_dev = FREQ_DEV_33;
            filcon_set = FILCON_SET_157;
            break;
        case DATA_RATE_10:
            datarate = BITRATE_10;
            bandwidth = BW_50;
            freq_dev = FREQ_DEV_33;
            filcon_set = FILCON_SET_157;
            break;
        case DATA_RATE_20:
            datarate = BITRATE_20;
            bandwidth = BW_75;
            freq_dev = FREQ_DEV_40;
            filcon_set = FILCON_SET_234;
            break;
        case DATA_RATE_40:
            datarate = BITRATE_40;
            bandwidth = BW_150;
            freq_dev = FREQ_DEV_80;
            filcon_set = FILCON_SET_414;
            break;
        case DATA_RATE_50:
            datarate = BITRATE_50;
            bandwidth = BW_175;
            freq_dev = FREQ_DEV_100;
            filcon_set = FILCON_SET_514;
            break;
        case DATA_RATE_66:
            datarate = BITRATE_66;
            bandwidth = BW_250;
            freq_dev = FREQ_DEV_133;
            filcon_set = FILCON_SET_676;
            break;
        case DATA_RATE_100:
            datarate = BITRATE_100;
            bandwidth = BW_400;
            freq_dev = FREQ_DEV_200;
            filcon_set = FILCON_SET_987;
            break;
        case DATA_RATE_200:
            datarate = BITRATE_200;
            bandwidth = BW_400;
            freq_dev = FREQ_DEV_200;
            filcon_set = FILCON_SET_987;
            break;
    }
    set_register(BRREG, datarate);
    set_register(FILCONREG, filcon_set | bandwidth);
    set_register(FDEVREG, freq_dev);
    return true;
}


bool set_power (uint8_t power){
  
}

void send_reload_radio (){
  
  set_rf_mode(RF_STANDBY);
  set_rf_mode(RF_SYNTHESIZER);
  set_register(FTPRIREG, (FTPRIREG_SET & 0xFD) | 0x02);
  set_rf_mode(RF_STANDBY);
  set_rf_mode(RF_RECEIVER);
}

uint8_t get_cca_noise() {
  return get_register(RSTSREG) >> 1;
}


/*
 * HW LAYER
 */
void HW_init(void){
  
}

void HW_sniffTX(uint8_t len,const uint8_t* data){
  char* m_type;
  char str[]="data\0commit\0ack\0commit_ack";
  
  switch (data[0] >> 6){
    case 0: m_type= str;break;
    case 1: m_type= str+5;break;
    case 2: m_type= str+12;break;
    case 3: m_type= str+16;break;
  }
  
  printf("RAW DATA:");
  for(int i=0;i<len;i++){
    printf("0x%02X ",data[i]);
  }
  printf("\n");
  
  printf("{\"direction\":\"tx\",\"len\": %u,\"link\":{\"message_type\":\"%s\",",len, m_type);
  
  printf("\"nid\":");
  printf("[\"0x%02X\"",data[1]);
  printf(",\"0x%02X\"",data[2]);
  printf(",\"0x%02X\"",data[3]);
  printf(",\"0x%02X\"],",data[4]);
  
  printf("\"to\":{");
  if ( data[0] & 0x20){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[5]);
    printf(",\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"]",data[8]);
  }else {
    printf("\"type\":\"cid\"");
    printf(",\"address\":\"0x%02X\"",data[5]);
  }
  printf("},");
  
  printf("\"from\":{");
  if ( data[0] & 0x10){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"",data[8]);
    printf(",\"0x%02X\"]",data[9]);
  }else{
    printf("\"type\":\"cid\"");
    if ( data[0] & 0x20){
      printf(",\"address\":\"0x%02X\"",data[9]);
    }else{
      printf(",\"address\":\"0x%02X\"",data[6]);
      printf(",\"seq num\":\"0x%02X\"",data[7]);
    }
  }
  printf("}}}\n");
}

void HW_sniffRX(uint8_t len,const uint8_t* data){
  char* m_type;
  char str[]="data\0commit\0ack\0commit_ack";
  
  switch (data[0] >> 6){
    case 0: m_type= str;break;
    case 1: m_type= str+5;break;
    case 2: m_type= str+12;break;
    case 3: m_type= str+16;break;
  }
  
  printf("RAW DATA:");
  for(int i=0;i<len;i++){
    printf("0x%02X ",data[i]);
  }
  printf("\n");
  
  printf("{\"direction\":\"rx\",\"len\": %u,\"link\":{\"message_type\":\"%s\",",len, m_type);
  
  printf("\"nid\":");
  printf("[\"0x%02X\"",data[1]);
  printf(",\"0x%02X\"",data[2]);
  printf(",\"0x%02X\"",data[3]);
  printf(",\"0x%02X\"],",data[4]);
  
  printf("\"to\":{");
  if ( data[0] & 0x20){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[5]);
    printf(",\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"]",data[8]);
  }else {
    printf("\"type\":\"cid\"");
    printf(",\"address\":\"0x%02X\"",data[5]);
  }
  printf("},");
  
  printf("\"from\":{");
  if ( data[0] & 0x10){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"",data[8]);
    printf(",\"0x%02X\"]",data[9]);
  }else{
    printf("\"type\":\"cid\"");
    if ( data[0] & 0x20){
      printf(",\"address\":\"0x%02X\"",data[9]);
    }else{
      printf(",\"address\":\"0x%02X\"",data[6]);
      printf(",\"seq num\":\"0x%02X\"",data[7]);
    }
  }
  printf("}}}\n");
}

extern void PHY_process_packet(uint8_t* data,uint8_t len);
extern void PHY_timer_interupt(void);

void HW_irq0occurred(void){}


void HW_irq1occurred(void){
//  printf("PCONREG %02x\n", get_register(PCONREG));
  if (PHY_STORAGE.mode == RF_RECEIVER) {
    printf("HW_irq1occurred() RF_RECIVER\n");

    uint8_t recived_len = 0;

//    std::lock_guard<std::mutex> lock(mm);
    {
           std::lock_guard<std::mutex> lock(mm);

	    PHY_STORAGE.irq1_enabled = false;
	    PHY_STORAGE.irq0_enabled = false;
	   
	    uint8_t fifo_stat = get_register(FTXRXIREG);

	    while (fifo_stat & 0x02) {
	      //HW_enableData();
	      uint8_t readed_byte = read_fifo();
	      //HW_disableData();
	      
	      PHY_STORAGE.recived_packet[recived_len++] = readed_byte;
	      
	      fifo_stat = get_register(FTXRXIREG);
	    }
	    
	    PHY_STORAGE.irq1_enabled = true;
	    PHY_STORAGE.irq0_enabled = true;
    }
    
    //HW_sniffRX(recived_len, PHY_STORAGE.recived_packet+1);
    printf("HW_irq1occurred() recived_len %d\n", recived_len);
    if (recived_len == 0) return;
    
    //send without first byte, test if recived_len == recived_packet[0]
    
    //add zero termination needed for string operations
    if(recived_len<63){
      PHY_STORAGE.recived_packet[recived_len]=0;
    }
      
    PHY_process_packet(PHY_STORAGE.recived_packet + 1 , recived_len - 1);
        
  } else {
    printf("HW_irq1occurred() ELSE\n");
    //if(waiting_send){
    //  cv.notify_one();
    //}
    //PHY_STORAGE.send_done = 1;
  }
}


void irq_interupt_deamon_f(){
  
}

void timer_interupt_generator_f(){

  while(!PHY_STORAGE.terminate_timer){
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
//    printf("%d %d\n", gpioGetValue(PIN_IRQ0), gpioGetValue(PIN_IRQ1));
//    printf("- GCONREG %02x FTPRIREG %02x FTXRXIREG %02x DMODREG %02x RSTHIREG %02x TXCONREG %02x PCONREG %02x\n", get_register(GCONREG), get_register(FTPRIREG), get_register(FTXRXIREG), get_register(DMODREG), get_register(RSTHIREG), get_register(TXPARAMREG), get_register(PCONREG)  );
//    for (int i=0;i<10;i++) {
//       printf("=====================================================================");
//    }
    PHY_timer_interupt();
  }
  
}

void PHY_init(PHY_init_t params){
    
    HW_init();
    //HW_disableConfig();
    //HW_disableData();
    PHY_STORAGE.irq_interupt_deamon = std::thread(irq_interupt_deamon_f);
    PHY_STORAGE.timer_interupt_generator = std::thread(timer_interupt_generator_f);
    
    PHY_STORAGE.send_done = false;
    PHY_STORAGE.cca_noise_treshold = params.cca_noise_treshold;

    
    printf("INIT channel %d",params.channel);
    printf("INIT band %d",params.band);
    printf("INIT bitrate %d",params.bitrate);
    printf("INIT power %d",params.power);

    
    //----  configuring the RF link --------------------------------    
    for (uint8_t i = 0; i <= 31; i++) {
        //setup freq
        if((i << 1)== R1CNTREG ){
          set_chanel_freq_rate (params.channel, params.band, params.bitrate);
          //jump over R1CNTREG, P1CNTREG, S1CNTREG
          i+=3;
        }

        if((i << 1)== TXPARAMREG ){
          set_power(params.power);
          //jump over TXPARAMREG
          i+=1;
        }

        if((i << 1)== FDEVREG){
          set_bitrate(params.bitrate);
          //jump over FDEVREG, BRREG
          i+=2;
        }
        if((i << 1)== FILCONREG){
          // already done in previous call of set_bitrate(params.bitrate);
          i+=1;
        }

        set_register(i<<1, InitConfigRegs[i]);
    }
    
    for (uint8_t i = 0; i <= 31; i++) {
      uint8_t reg_val = get_register(i<<1);
      printf("Register[%d] value: 0x%02X\n",i,reg_val);
    }
    
    send_reload_radio();

    //HW_clearIRQ0();
    //HW_enableIRQ0();
    PHY_STORAGE.irq0_enabled=true;

    //HW_clearIRQ1();
    //HW_enableIRQ1();
    PHY_STORAGE.irq1_enabled=true;
}

void PHY_stop(){

  PHY_STORAGE.terminate_timer = true;
  
  // wait for threads to end
  PHY_STORAGE.irq_interupt_deamon.join();
  PHY_STORAGE.timer_interupt_generator.join();
}

void PHY_send(const uint8_t* data, uint8_t len){
	string msg, msg_tmp;
	msg.clear();
	
	msg = create_head();
	
  for (uint8_t i = 0; i < len; i++) {
  	msg += to_string(data[i]) + ',';
  	
  }
  	cout << endl << "PHY_send: " << msg << endl;

    msg_tmp = "/usr/bin/mosquitto_pub -t BeeeOn/data_from -m " + msg; 

    //cout << msg_tmp << endl;
    std::system(msg_tmp.c_str());

	//mq->publish(NULL, "BeeeOn/data_from", msg.length(), msg.c_str());

 
  
}

void PHY_send_with_cca(uint8_t* data,uint8_t len){
  uint8_t noise;
  std::lock_guard<std::mutex> lock(mm);
  while((noise=PHY_get_noise()) > PHY_STORAGE.cca_noise_treshold){
  }
  PHY_send(data,len);
}

bool PHY_set_freq   (uint8_t band){
  if(band == PHY_STORAGE.band) return true;
  bool holder = set_chanel_freq_rate (PHY_STORAGE.channel, band, PHY_STORAGE.bitrate);
  send_reload_radio();
  return holder;
}

bool PHY_set_channel (uint8_t channel){
  if(channel == PHY_STORAGE.channel) return true;
  bool holder = set_chanel_freq_rate (channel, PHY_STORAGE.band, PHY_STORAGE.bitrate);
  send_reload_radio();
  return holder;
}

bool PHY_set_bitrate(uint8_t bitrate){
  if(bitrate == PHY_STORAGE.bitrate) return true;
  if (set_bitrate(bitrate) == false) return false;
  bool holder = set_chanel_freq_rate (PHY_STORAGE.channel, PHY_STORAGE.band, bitrate);
  send_reload_radio();
  return holder;
}

bool PHY_set_power(uint8_t power){
    if (power == PHY_STORAGE.power) return true;
    return set_power(power);
}

uint8_t PHY_get_noise (){
  return get_cca_noise();
}








