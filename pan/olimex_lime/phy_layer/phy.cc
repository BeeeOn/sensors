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
#include "debug.h"

//#include "constants.h"
#include "constants.h"
#include "phy.h"

using namespace std; 

#define SYSGPIO "/sys/class/gpio"
#define PIN_IRQ0 "gpio274"
#define PIN_IRQ1 "gpio275"
#define PIN_RESET "gpio260"

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
    GMainLoop* gloop;
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
  uint8_t get_cca_noise();

/*
 * 
 */

void HW_irq0occurred(void);
void HW_irq1occurred(void);

// SPI config
// more at http://linux-sunxi.org/SPIdev
static const char *devspi_config = "/dev/spidev32766.0";  // SPI bus 0, chipselect 0
static const char *devspi_data = "/dev/spidev32766.1";  // SPI bus 0, chipselect 1
static uint8_t mode = SPI_MODE_0;
const uint8_t spi_bits_per_word = 8;
const uint32_t spi_speed_hz = 1000000;  // at 1 MHz
struct spi_ioc_transfer xfer[2];



static gboolean onIRQ0Event(GIOChannel *channel, GIOCondition condition, gpointer user_data){
    GError *error = 0;
    gsize bytes_read = 0; 
    const int buf_sz = 1024;
    gchar buf[buf_sz] = {};

    g_io_channel_seek_position( channel, 0, G_SEEK_SET, 0 );
    g_io_channel_read_chars( channel, buf, buf_sz - 1, &bytes_read, &error );    
    if(PHY_STORAGE.irq0_enabled){
        HW_irq0occurred();
    }
    return 1;
}

static gboolean onIRQ1Event(GIOChannel *channel, GIOCondition condition, gpointer user_data){  
    GError *error = 0;
    gsize bytes_read = 0;
    const int buf_sz = 1024;
    gchar buf[buf_sz] = {};

    g_io_channel_seek_position( channel, 0, G_SEEK_SET, 0 );
    g_io_channel_read_chars(channel, buf, buf_sz - 1, &bytes_read, &error );
    if(PHY_STORAGE.irq0_enabled) {    
        HW_irq1occurred();
    }
    return 1;
}

int spiOpenConfig(void){
    int fd = open(devspi_config, O_RDWR); 
    if (fd < 0) {
        cerr << "Can't open SPI device!" << endl;
    }   
    return fd;
}

int spiOpenData(void){
    int fd = open(devspi_data, O_RDWR);
    
    if (fd < 0) {
      cerr << "Can't open SPI device!" << endl;
    }            
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
      cerr << "Can't set spi mode!" << endl;
      return -1;
    }            
    return fd;
}

int spiBusConfig(int fd) {  

    //xfer[0].len = 3; /* Length of  command to write*/
    xfer[0].cs_change = 0; /* Keep CS activated */
    xfer[0].delay_usecs = 0; //delay in us
    xfer[0].speed_hz = spi_speed_hz; //speed
    xfer[0].bits_per_word = spi_bits_per_word; // bites per word 8

    //xfer[0].len = 3; /* Length of  command to write*/
    xfer[1].cs_change = 0; /* Keep CS activated */
    xfer[1].delay_usecs = 0; //delay in us
    xfer[1].speed_hz = spi_speed_hz; //speed
    xfer[1].bits_per_word = spi_bits_per_word; // bites per word 8  
    return 0;
}


void gpioExport(int pin) {
    ofstream fd;
    fd.open(SYSGPIO"/export");
    fd << int(pin);  // IRQ0
    fd.close(); 
}

void gpioSetDirection(string gpio, string dir) {
    ofstream fd;
    string dirFile = string(SYSGPIO"/") + gpio + string("/direction");
    fd.open(dirFile.c_str());
    fd << dir.c_str();
    fd.close();
}

void gpioSetEdge(string gpio, string edge) {
    ofstream fd;
    string edgeFile = string(SYSGPIO"/") + gpio + string("/edge");
    fd.open(edgeFile.c_str());
    fd << edge.c_str();
    fd.close();
}

void gpioSetValue(string gpio, string value) {
    ofstream fd;
    string valueFile = string(SYSGPIO"/") + gpio + string("/value");
    fd.open(valueFile.c_str());
    fd << value.c_str();
    fd.close();
}

bool gpioGetValue(string gpio) {
    ifstream fd;
    char val[1];
    string valueFile = string(SYSGPIO"/") + gpio + string("/value");
    fd.open(valueFile.c_str());
    fd.read(val, 1);
//    fd << value.c_str();
    fd.close();
    if (val[0] == '0') return false;
    return true;
}


void resetMRF(void) {
    gpioSetValue(PIN_RESET, "1");
    usleep(100);    
    gpioSetValue(PIN_RESET, "0");
    usleep(10000);  // wait 10ms for MRF ready
}

void init_io(void) {
    ofstream fd;
    GIOChannel* channel;
    GIOCondition cond;
    int IRQ0fd, IRQ1fd;

    // export gpio (default input direction)
    gpioExport(274);  // IRQ0
    gpioExport(275);  // IRQ1
    gpioExport(260);  // RESET
    gpioSetDirection(PIN_RESET, "out");
    gpioSetValue(PIN_RESET, "0");

    // set gpio interrupt edge
    gpioSetEdge(PIN_IRQ0, "rising");
    gpioSetEdge(PIN_IRQ1, "rising");

    // init callbacks
    IRQ0fd = open(SYSGPIO"/gpio274/value", O_RDONLY | O_NONBLOCK );
    channel = g_io_channel_unix_new(IRQ0fd);
    cond = GIOCondition(G_IO_PRI);
    /*guint idIRQ0 = */g_io_add_watch( channel, cond, onIRQ0Event, 0 );

    IRQ1fd = open(SYSGPIO"/gpio275/value", O_RDONLY | O_NONBLOCK );
    channel = g_io_channel_unix_new(IRQ1fd);
    cond = GIOCondition(G_IO_PRI);
    /*guint idIRQ1 = */g_io_add_watch( channel, cond, onIRQ1Event, 0);

}
 
/**
 * CONSTANT TABLE for BAND_863, BAND863_C950, BAND_902, BAND_915 
 */
const uint16_t _start_freq[] = {860,                   950,                      902,                   915};     
uint16_t _chanel_spacing[] = {  384,                   400,                      400,                   400};  

uint16_t _channelCompare(uint8_t band, uint8_t channel, uint8_t bitrate){
    uint32_t freq = (uint32_t)_start_freq[band]*1000;
    if ((band==BAND_863 || band==BAND_863_C950) && !(bitrate==DATA_RATE_100 || bitrate==DATA_RATE_200)){
        freq += channel * 300;
    } 
    else{
        freq += channel * _chanel_spacing[band];
    }    
    uint32_t chanelCompareTmp =(freq * 808);
    return (uint16_t) (chanelCompareTmp/((uint32_t) 9*FXTAL)); 
}

uint8_t chanelAmount(uint8_t band, uint8_t bitrate){
    if ((band==BAND_863 || band==BAND_863_C950) && (bitrate==DATA_RATE_100 || bitrate==DATA_RATE_200)){
        return  25;
    } 
    else {
        return 32;
    }
}


/**
 * Get constant for MRF89 configuration
 */
uint8_t rValue(){
    return 100;
}   
uint8_t pValue(uint8_t band, uint8_t chanel, uint8_t bitrate){  
    uint8_t pvalue = (uint8_t)(((uint16_t)(_channelCompare(band,chanel,bitrate) - 75)/76)+1);
    return pvalue;
}       
uint8_t sValue(uint8_t band, uint8_t chanel, uint8_t bitrate){
    uint16_t channel_cmp=_channelCompare(band,chanel,bitrate);
    uint8_t pvalue = (uint8_t)(((uint16_t)(channel_cmp - 75)/76)+1);
    uint8_t svalue = (uint8_t)(((uint16_t)channel_cmp - ((uint16_t)(75*(pvalue+1)))));
    return svalue;
}


void set_register(const uint8_t address, const uint8_t data){  
    int fd;
    char wr_buf[2];
  
    fd = spiOpenConfig();
    spiBusConfig(fd);
  
    wr_buf[0] = address;  // address
    wr_buf[1] = data;  // value
  
    xfer[0].tx_buf = (unsigned long) wr_buf;
    xfer[0].len = 2;
    xfer[0].rx_buf = 0;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), xfer) < 0) {
        D_PHY printf("set_register(): Can't write register!\n");
        return;
    }           
    close(fd);
}

uint8_t get_register(uint8_t address) {
    int fd;
    char wr_buf[1];
    char rd_buf[1];

    std::lock_guard<std::mutex> lock(mmm);

    fd = spiOpenConfig();
    spiBusConfig(fd);

    wr_buf[0] = (address | 0x40) &0x7e;
    xfer[0].tx_buf = (unsigned long) wr_buf;
    xfer[0].len = 1;
    xfer[0].rx_buf = 0;
    xfer[1].rx_buf = (unsigned long) rd_buf;
    xfer[1].len = 1;

    if (ioctl(fd, SPI_IOC_MESSAGE(2), xfer) < 0) {
        cerr << "Can't read register!" << endl;
        return 0;
    }

    close(fd);
    return (uint8_t)rd_buf[0];
}

uint8_t read_fifo(void) {
    int fd = 0;

    char rd_buf[1];
    fd = spiOpenData();
    spiBusConfig(fd);  
    if (read(fd, rd_buf, 1) != 1) {
        cerr << "Read error" << endl;
        return -1;
    }              
    close(fd);
    return rd_buf[0];
}

void write_fifo(const uint8_t data){
    int fd;
    uint8_t data_nostack = data;
    
    fd = spiOpenData();
    spiBusConfig(fd);
    
    xfer[0].tx_buf = (unsigned long) &data_nostack;
    xfer[0].len = 1;
    xfer[0].rx_buf = 0;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), xfer) < 0) {
        D_PHY printf("Can't write register!\n");
        return;
    }  
    close(fd);
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
    //printf("Channel amoutn %d, chanel %d",chanelAmount(band, bitrate),channel);
    if (channel >= chanelAmount(band, bitrate)) {
      D_PHY printf("Invalid channel set\n");
      return false;
    }
  
    //Program registers R, P, S and Synthesize the RF
    PHY_STORAGE.channel=channel;
    PHY_STORAGE.band=band;
    PHY_STORAGE.bitrate=bitrate;
    
    /*
    D_PHY printf("chanel %d\n",channel);
    D_PHY printf("band %d\n",band);
    D_PHY printf("bitrate %d\n",bitrate);
    D_PHY printf("RVAL %d\n",rValue());
    D_PHY printf("PVAL %d\n",pValue(band, channel, bitrate));
    D_PHY printf("SVAL %d\n",sValue(band, channel, bitrate));
    */                                                    
    set_register(R1CNTREG, rValue());
    set_register(P1CNTREG, pValue(band, channel, bitrate));
    set_register(S1CNTREG, sValue(band, channel, bitrate));
    
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


bool set_power (uint8_t power){
  if (power > TX_POWER_N_8_DB) {
        return false;
    }
    //set value 1111xxx(r)
    set_register(TXPARAMREG, 0xF0 | (power << 1));
    PHY_STORAGE.power=power;
    return true;
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
 * HW LAYER initialization
 */
void HW_init(void){
    init_io();
    resetMRF();
}

extern void PHY_process_packet(uint8_t* data,uint8_t len);
extern void PHY_timer_interupt(void);

void HW_irq0occurred(void){}


/*
 * IRQ when packet is received or sent
 */
void HW_irq1occurred(void){
    
    if (PHY_STORAGE.mode == RF_RECEIVER) 
    {   
        uint8_t received_len = 0;           
        printf("HW_irq1occurred(): RF_RECIVER\n");
        {
            std::lock_guard<std::mutex> lock(mm);    
    	    PHY_STORAGE.irq1_enabled = false;
    	    PHY_STORAGE.irq0_enabled = false;
    	   
    	    uint8_t fifo_stat = get_register(FTXRXIREG);
    
    	    while (fifo_stat & 0x02) {    	           	      
    	       PHY_STORAGE.recived_packet[received_len++] = read_fifo();    	      
    	       fifo_stat = get_register(FTXRXIREG);
    	    }                                                              	    
    	    PHY_STORAGE.irq1_enabled = true;
    	    PHY_STORAGE.irq0_enabled = true;
        }
    
        //HW_sniffRX(recived_len, PHY_STORAGE.recived_packet+1);
        // @TODO - read data from MRF accroding to first readed byte, everything
        // else can be another packet in one turn
        D_PHY printf("HW_irq1occurred(): recived_len %d\n", received_len);
        if (received_len == 0 || received_len - 1 != PHY_STORAGE.recived_packet[0]) {
            D_PHY printf("HW_irq1occurred(): ");
            return;
        }
        //send without first byte, test if recived_len == recived_packet[0]
        PHY_process_packet(PHY_STORAGE.recived_packet + 1 , received_len - 1);           
    } 
    else {
        printf("HW_irq1occurred(): NOT in RF_RECEIVER mode\n");    
    }   
}

/*
 * Interrupt waiting thread
 */
void irq_interupt_deamon_f(){
  PHY_STORAGE.gloop = g_main_loop_new( 0, 0 );
  g_main_loop_run( PHY_STORAGE.gloop );
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
    PHY_STORAGE.irq_interupt_deamon = std::thread(irq_interupt_deamon_f);
    PHY_STORAGE.timer_interupt_generator = std::thread(timer_interupt_generator_f);
    
    PHY_STORAGE.send_done = false;
    PHY_STORAGE.cca_noise_treshold = params.cca_noise_treshold;
        
    printf("Channel %d band %d bitrate %d power %d\n", params.channel, params.band, 
            params.bitrate, params.power);
    
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
      //D_PHY printf("Register[%d] value: 0x%02X\n",i,reg_val);
    }
    
    send_reload_radio();        
    PHY_STORAGE.irq0_enabled=true;        
    PHY_STORAGE.irq1_enabled=true;
}

void PHY_stop(){
    g_main_loop_quit(PHY_STORAGE.gloop);
    PHY_STORAGE.terminate_timer = true;  
    // wait for threads to end
    PHY_STORAGE.irq_interupt_deamon.join();
    PHY_STORAGE.timer_interupt_generator.join();
}

/**
 * Send data to RF
 */
void PHY_send(const uint8_t* data, uint8_t len){
    PHY_STORAGE.irq1_enabled = false;
    PHY_STORAGE.irq0_enabled = false;
    
    D_PHY printf("PHY_send()\n");
   
    set_rf_mode(RF_STANDBY);
    set_register(FTXRXIREG, FTXRXIREG_SET | 0x01);    
    write_fifo(len);
    for (int i = 0; i < len; i++) {
        write_fifo(data[i]);
    }  
    set_rf_mode(RF_TRANSMITTER);
        
    PHY_STORAGE.irq1_enabled = true;
    PHY_STORAGE.irq0_enabled = true;
        
    uint32_t tout = 0;  
    while((get_register(FTPRIREG) & 0x20) == 0) {        
        usleep(500);
        tout++;
        if (tout > 4000) break;  // timeout 2seconds
    }    
    set_rf_mode(RF_STANDBY);
    set_rf_mode(RF_RECEIVER);
}

/**
 * Wait for silent on medium
 */
void PHY_send_with_cca(uint8_t* data,uint8_t len){
    uint8_t noise;
    std::lock_guard<std::mutex> lock(mm);
    while((noise=PHY_get_noise()) > PHY_STORAGE.cca_noise_treshold){
    }
    PHY_send(data,len);
}

bool PHY_set_freq(uint8_t band){
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

uint8_t PHY_get_channel(void) {
    D_PHY printf("PHY_get_channel(): my channel is %d\n", PHY_STORAGE.channel);
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








