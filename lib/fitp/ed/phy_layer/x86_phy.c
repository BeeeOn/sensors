#include "include.h"

/*
 * internal phy storage
 */

#define PHY_FIFO_FRONT_SIZE  63

struct PHY_storage_t {
  
    uint8_t mode;
    uint8_t channel;
    uint8_t band;
    uint8_t bitrate;
    uint8_t power;
    uint8_t send_done;
    uint8_t recived_packet[PHY_FIFO_FRONT_SIZE];
    uint8_t cca_noise_treshold;
    std::thread timer_interupt_generator;
    
} PHY_STORAGE;

/*
 * support functions
 */

void    set_rf_mode (uint8_t mode);
bool    set_power   (uint8_t power);
bool    set_bitrate (uint8_t bitrate);
bool    set_chanel_freq_rate (uint8_t channel, uint8_t offsetFreq,uint8_t bitrate);
  uint8_t get_cca_noise();
uint8_t chanelAmount(uint8_t band, uint8_t bitrate);



void set_rf_mode(uint8_t mode) {

    if ((mode == RF_TRANSMITTER) || (mode == RF_RECEIVER) ||
            (mode == RF_SYNTHESIZER) || (mode == RF_STANDBY) ||
            (mode == RF_SLEEP)) {

        PHY_STORAGE.mode = mode;
    }
}

bool set_chanel_freq_rate (uint8_t channel, uint8_t band,uint8_t bitrate){
  printf("Channel amoutn %d, chanel %d",chanelAmount(band, bitrate),channel);
  if (channel >= chanelAmount(band, bitrate)) {
    //errstate invalid_chanel_freq_rate;
    return false;
  }

  //Program registers R, P, S and Synthesize the RF
   PHY_STORAGE.channel=channel;
   PHY_STORAGE.band=band;
   PHY_STORAGE.bitrate=bitrate;
   
   printf("chanel %d\n",channel);
   printf("band %d\n",band);
   printf("bitrate %d\n",bitrate);

   return true;
}

bool set_power (uint8_t power){
    if (power > TX_POWER_N_8_DB) {
        return false;
    }
    PHY_STORAGE.power=power;
    return true;
}

bool set_bitrate (uint8_t bitrate) {
    if (bitrate > DATA_RATE_200) {
        return false;
    }

    return true;
}


/**
 * @def chanelAmount
 * @brief return number of chanels for band and bitrate
 */
uint8_t chanelAmount(uint8_t band, uint8_t bitrate){
  if ((band==BAND_863 || band==BAND_863_C950) && (bitrate==DATA_RATE_100 || bitrate==DATA_RATE_200)){
   return  25;
  } else{
    return 32;
  }
}


void timer_interupt_generator_f(){                              
  while(1){    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));    
    PHY_timer_interupt();                                       
  }
}



void PHY_init(PHY_init_t params){
    PHY_STORAGE.send_done = false;
    PHY_STORAGE.cca_noise_treshold = params.cca_noise_treshold;

    set_chanel_freq_rate (params.channel, params.band, params.bitrate);
    set_power(params.power);                    
    set_bitrate(params.bitrate);
    PHY_STORAGE.timer_interupt_generator = std::thread(timer_interupt_generator_f);
}

bool PHY_set_channel (uint8_t channel){
  if(channel == PHY_STORAGE.channel) return true;
  bool holder = set_chanel_freq_rate (channel, PHY_STORAGE.band, PHY_STORAGE.bitrate);
  return holder;
}

/**
 * @brief get a channel
 */
uint8_t PHY_get_channel(void)
{
    printf("Channel: %d\n", PHY_STORAGE.channel);
    return PHY_STORAGE.channel;
}

uint8_t PHY_get_noise (){
  return 20;
}

void PHY_send(const uint8_t* data, uint8_t len){
	string msg, msg_tmp;
	msg.clear();
	
	msg = create_head();
	
  for (uint8_t i = 0; i < len; i++) {
  	msg += to_string(data[i]) + ',';
  	
  }
  	cout << endl << "PHY_send: " << msg << endl;

//	mq->send_message(DATA_FROM_TOPIC, msg.c_str(), msg.length());
    msg_tmp = "/usr/bin/mosquitto_pub -t BeeeOn/data_from -m " + msg;

    //cout << msg_tmp << endl;
    std::system(msg_tmp.c_str());
    
}

void PHY_send_with_cca(uint8_t* data,uint8_t len){
  PHY_send(data, len);
}





















