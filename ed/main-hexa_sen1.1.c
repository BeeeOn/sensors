#pragma config OSC = INTOSCPLL, WDTEN = OFF, XINST = OFF, WDTPS = 512

#include "include.h"

extern bool accel_int;

uint8_t fitp_recived(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part) {
    printf("ED received data: ");
    for(uint8_t i=0; i<len; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");
    if(data[0] == SET_ACTORS) {
        setActors(data+2);  // skip message type and protocol version
    }    
}

bool fitp_accept_join(uint8_t nid[4], uint8_t parent_cid) {
    printf("acepting join: true");
    return true;
}


extern   void load_configuration(uint8_t *buf, uint8_t len);
extern   void save_configuration(uint8_t *buf, uint8_t len);
void main(void) {
    PHY_init_t phy_params;
    LINK_init_t link_params;       

    WDTCONbits.SWDTEN = 0; 
    
    LOG_init();                
    D_G printf("Main started\n");    
    
    phy_params.bitrate = DATA_RATE_66;
    phy_params.band = BAND_863;
    phy_params.channel = 28;
    phy_params.power = TX_POWER_13_DB;
    phy_params.cca_noise_treshold = 30;
    PHY_init(phy_params);
    D_G printf("PHY inicialized\n");
    
    link_params.tx_max_retries = 0;
    link_params.rx_data_commit_timeout = 64; 
    LINK_init(link_params);

    ds_prepare();
    for (uint8_t i = 0; i < 12; i++) {
        LED0 = ~LED0;
        delay_ms(50);
    }
    
    fitp_init(); 
    
    /*
    GLOBAL_STORAGE.edid[0] = 0xED; //E
    GLOBAL_STORAGE.edid[1] = 0x00; //d
    GLOBAL_STORAGE.edid[2] = 0x00; //i
    GLOBAL_STORAGE.edid[3] = 0x01; //d
    
    GLOBAL_STORAGE.nid[0]=0x4e;  //N
    GLOBAL_STORAGE.nid[1]=0x69; //i
    GLOBAL_STORAGE.nid[2]=0x64; //d
    GLOBAL_STORAGE.nid[3]=0x3c;  //:
    */
             
    GLOBAL_STORAGE.sleepy_device = true;
    GLOBAL_STORAGE.refresh_time = 30;
    
    //clean EEPROM before first write
    //spieepromClean();
    euid_load(); // load euid from eeprom
    
    accel_int = false;
    while (1) {       
        
        if (accel_int) {
            HW_ReInit();      
            if (sendValues()) { 
                D_G printf("Send values success\n");               
                LED1 = 1;
                delay_ms(1000);
                LED1 = 0;            
            }
            else if (fitp_join()){
                D_G printf("Join success\n"); 
                LED1 = 1;
                delay_ms(1000);
                LED1 = 0;
            }
            else {  // cannot send data and even join fails
                D_G printf("Send value and join failed\n");
                LED0 = 1;
                delay_ms(1000);
                LED0 = 0;
            }
            accel_int = false;                                    
        }           
       
        if (fitp_joined()){            
            ds_prepare();
            HW_DeInit();
            StartTimer(2);            
            goSleep();
            if (accel_int) {
                continue;
            }
            //LED1 = 1;
            HW_ReInit();
            sendValues();             
        }   
        
        HW_DeInit();
        if(GLOBAL_STORAGE.refresh_time < 3) GLOBAL_STORAGE.refresh_time = 3;  // if too short time         
        StartTimer(GLOBAL_STORAGE.refresh_time - 2);  // -2 because ds_prepare takes up to 2seconds 
        //LED1 = 0;
        goSleep();
    }
}
