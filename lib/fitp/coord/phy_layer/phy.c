#include "phy.h"

/*
 * internal phy storage
 */

#include "log.h"
#define _XTAL_FREQ 8000000

struct PHY_storage_t {
  
    uint8_t mode;
    uint8_t channel;
    uint8_t band;
    uint8_t bitrate;
    uint8_t power;
    uint8_t send_done;
    uint8_t recived_packet[MAX_PHY_PAYLOAD_SIZE];
    uint8_t cca_noise_treshold;
    
} PHY_STORAGE;

/*
 * extern functions required
 */
extern   void HW_spiPut(const uint8_t data);
extern   uint8_t HW_spiGet(void);
extern   void HW_disableConfig(void);
extern   void HW_enableConfig(void);
extern   void HW_disableData(void);
extern   void HW_enableData(void);
extern   bool HW_isIRQ0Enabled(void);
extern   bool HW_isIRQ0Active(void);
extern   bool HW_isIRQ0Clear(void);
extern   void HW_clearIRQ0(void);
extern   void HW_disableIRQ0(void);
extern   void HW_enableIRQ0(void);
extern   bool HW_isIRQ1Enabled(void);
extern   bool HW_isIRQ1Active(void);
extern   bool HW_isIRQ1Clear(void);
extern   void HW_clearIRQ1(void);
extern   void HW_disableIRQ1(void);
extern   void HW_enableIRQ1(void);
extern   void HW_init(void);
extern   void HW_sniffRX(uint8_t len,const char* data);
extern   void HW_sniffTX(uint8_t len,const char* data);


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
  uint8_t get_cca_noise();

/*
 * support functions for frequency
 */

uint8_t chanelAmount(uint8_t band, uint8_t bitrate);
uint16_t channelCompare(uint8_t band, uint8_t channel, uint8_t bitrate);
uint8_t rValue();
uint8_t pValue(uint8_t band, uint8_t chanel, uint8_t bitrate);
uint8_t sValue(uint8_t band, uint8_t chanel, uint8_t bitrate);

/*
 * implemented function from hw layer
 */

  void HW_irq0occurred(void);
  void HW_irq1occurred(void);
  void HW_timeoccurred(void);



/*
 * implementation
 */
void set_register(uint8_t address, uint8_t value) {

    uint8_t IRQ1select = HW_isIRQ1Enabled();
    uint8_t IRQ0select = HW_isIRQ0Enabled();

    HW_disableIRQ0();
    HW_disableIRQ1();

    HW_enableConfig();
    //0x3E = 0b00111110 START_BIT|_W_/R|D|D|D|D|D|STOP_BIT
    HW_spiPut(address & 0x3E);
    HW_spiPut(value);
    HW_disableConfig();


    if (IRQ1select) HW_enableIRQ1();
    if (IRQ0select) HW_enableIRQ0();
}

uint8_t get_register(uint8_t address) {
    uint8_t value;
    uint8_t IRQ1select = HW_isIRQ1Enabled();
    uint8_t IRQ0select = HW_isIRQ0Enabled();

    HW_disableIRQ0();
    HW_disableIRQ1();

    HW_enableConfig();
    //0x7E = 0b01111110 START_BIT|W/_R_|D|D|D|D|D|STOP_BIT
    address = (address | 0x40) &0x7e;
    HW_spiPut(address);
    value = HW_spiGet();
    HW_disableConfig();
    if (IRQ1select) HW_enableIRQ1();
    if (IRQ0select) HW_enableIRQ0();

    return value;
}

void write_fifo(uint8_t Data) {
    uint8_t IRQ1select = HW_isIRQ1Enabled();
    uint8_t IRQ0select = HW_isIRQ0Enabled();

    HW_disableIRQ0();
    HW_disableIRQ1();

    HW_enableData();
    HW_spiPut(Data);
    HW_disableData();

    if (IRQ1select) HW_enableIRQ1();
    if (IRQ0select) HW_enableIRQ0();
}

void set_rf_mode(uint8_t mode) {

    if ((mode == RF_TRANSMITTER) || (mode == RF_RECEIVER) ||
            (mode == RF_SYNTHESIZER) || (mode == RF_STANDBY) ||
            (mode == RF_SLEEP)) {

        set_register(GCONREG, (GCONREG_SET & 0x1F) | mode);
        PHY_STORAGE.mode = mode;
    }
}

bool set_chanel_freq_rate (uint8_t channel, uint8_t band,uint8_t bitrate){

  if (channel >= chanelAmount(band, bitrate)) {
    //errstate invalid_chanel_freq_rate;
    return false;
  }

  //Program registers R, P, S and Synthesize the RF
    PHY_STORAGE.channel=channel;
    PHY_STORAGE.band=band;
    PHY_STORAGE.bitrate=bitrate;

  set_register(R1CNTREG, rValue());
  //logiln(I3, "MiMacSetChanel rValue", rValue());
  set_register(P1CNTREG, pValue(band, channel, bitrate));
  //logiln(I3, "MiMacSetChanel pValue", pValue(band, channel, bitrate));
  set_register(S1CNTREG, sValue(band, channel, bitrate));
  //logiln(I3, "MiMacSetChanel sValue", sValue(band, channel, bitrate));
  
  return true;
}

bool set_power (uint8_t power){
    if (power > TX_POWER_N_8_DB) {
        return false;
    }
    //set value 1111xxx(r)
    set_register(TXPARAMREG, 0xF0 | (power << 1));
    PHY_STORAGE.power=power;
    return true;
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

// 
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
CONSTANT TABLE
                                                BAND_863      BAND863_C950        BAND_902        BAND_915*/
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

/**
 * @def rvalue
 * @brief compute rvalue for mrf89xa
 */
uint8_t rValue(){
  return 100;
}

/**
 * @def pvalue 
 * @brief compute pvalue for mrf89xa based on band chanel & bitrate
 * @param band enum value with band
 * @param channel uint8_t value in range from 1 to chanelAmount(band bitrate)
 * @param bitrate enum value with selected bitrate
 */
uint8_t pValue(uint8_t band, uint8_t chanel, uint8_t bitrate){  
  uint8_t pvalue = (uint8_t)(((uint16_t)(_channelCompare(band,chanel,bitrate) - 75)/76)+1);
  return pvalue;
}

/**
 * @def svalue 
 * @brief compute svalue for mrf89xa based on band chanel & bitrate
 * @param band enum value with band
 * @param channel uint8_t value in range from 1 to chanelAmount(band bitrate)
 * @param bitrate enum value with selected bitrate
 */
uint8_t sValue(uint8_t band, uint8_t chanel, uint8_t bitrate){
  uint16_t channel_cmp=_channelCompare(band,chanel,bitrate);
  uint8_t pvalue = (uint8_t)(((uint16_t)(channel_cmp - 75)/76)+1);
  uint8_t svalue = (uint8_t)(((uint16_t)channel_cmp - ((uint16_t)(75*(pvalue+1)))));
  return svalue;
}

void PHY_init(PHY_init_t params){
    printf("PHY_init\n");
    HW_init();
    
    HW_disableConfig();
    HW_disableData();
    
    PHY_STORAGE.send_done = false;
    PHY_STORAGE.cca_noise_treshold = params.cca_noise_treshold;

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
    send_reload_radio();

    HW_clearIRQ0();
    HW_enableIRQ0();

    HW_clearIRQ1();
    HW_enableIRQ1();
}

bool PHY_set_freq   (uint8_t band){
  if(band == PHY_STORAGE.band) return true;
  bool holder = set_chanel_freq_rate (PHY_STORAGE.channel, band, PHY_STORAGE.bitrate);
  send_reload_radio();
  return holder;
}

bool PHY_set_channel (uint8_t channel){
    printf("Kanal: %d\n", PHY_STORAGE.channel);   
    bool holder = set_chanel_freq_rate (channel, PHY_STORAGE.band, PHY_STORAGE.bitrate);
    send_reload_radio();
    return holder;
}

uint8_t PHY_get_channel(void)
{
    printf("my: %d\n", PHY_STORAGE.channel);
    return PHY_STORAGE.channel;
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

void PHY_send(const char* data, uint8_t len){
  printf("PHY_send\n");
  HW_disableIRQ0();
  HW_disableIRQ1();

  set_rf_mode(RF_STANDBY);
  set_register(FTXRXIREG, FTXRXIREG_SET | 0x01);

  write_fifo(len);
  
  for (int i = 0; i < len; i++) {
      write_fifo(data[i]);
  }
  set_rf_mode(RF_TRANSMITTER);

  HW_enableIRQ0();
  HW_enableIRQ1();
  
  while((get_register(FTPRIREG) & 0x20) == 0) {
      //printf("PHY_send 2 %02x\n", get_register(FTPRIREG));      
  }
  //PHY_STORAGE.send_done = 0; 
  set_rf_mode(RF_STANDBY);
  set_rf_mode(RF_RECEIVER);
  printf("PHY_send done\n");
}

void PHY_send_with_cca(uint8_t* data,uint8_t len){
  uint8_t noise;
  while((noise=PHY_get_noise()) > PHY_STORAGE.cca_noise_treshold){
  }
  PHY_send(data,len);
}

  void HW_irq0occurred(void){}

  void HW_irq1occurred(void){
    if (PHY_STORAGE.mode == RF_RECEIVER) 
    {
        PORTAbits.RA6 = ~ PORTAbits.RA6;
        uint8_t recived_len = 0;
        uint8_t fifo_stat = get_register(FTXRXIREG);

        while (fifo_stat & 0x02) 
        {
          HW_enableData();
          uint8_t readed_byte = HW_spiGet();
          HW_disableData();

          PHY_STORAGE.recived_packet[recived_len++] = readed_byte;

          fifo_stat = get_register(FTXRXIREG);
        }
    
    //add zero termination needed for string operations
    if(recived_len<63){
      PHY_STORAGE.recived_packet[recived_len]=0;
    }
      
    PHY_process_packet(PHY_STORAGE.recived_packet + 1 , recived_len - 1);
 
  } else {
    // PHY_STORAGE.send_done = 1;
  }
}


  void HW_timeoccurred(void){
  PHY_timer_interupt();
}
















