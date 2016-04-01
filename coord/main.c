#include "include.h"

#pragma config OSC = INTOSCPLL, WDTEN = OFF, XINST = OFF

#define _XTAL_FREQ 8000000
#define SENDER 0

extern void delay_ms(uint16_t maximum);


void delay(){
for(uint8_t i=0;i<20;i++)
 for(uint8_t k=0;k<254;k++)
  for(uint8_t j=0;j<254;j++);
}


uint8_t fitp_recived(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part){
  printf("recived message: %s",data);
  return FITP_RECIVING_DONE;
}

bool fitp_accept_join(uint8_t nid[4], uint8_t parent_cid){
  printf("Accepting join: true");
  return true;
}

bool join = false;

bool direction_up = true;
void main(void) {
    
//     WDTCONbits.SWDTEN = 0; // sleep watchdog timer off
//     OSCTUNEbits.PLLEN = 1; // enable PLL
//     while (OSCCONbits.OSTS == 0); // Wait for clock to stable

    
    LOG_init();
    
    printf("Main started\n");
    {
      PHY_init_t params;
      //params.bitrate = DATA_RATE_20;
      params.bitrate = DATA_RATE_20;
      params.band = BAND_863;
      /*if(SENDER)
      {
          params.channel = 23;
      }
      else
      {*/
          params.channel = 17;
      //}
      params.power = TX_POWER_1_DB;
      params.cca_noise_treshold = 30;
      
      PHY_init(params);
      //printf("PHY inicialized\n");
    }
    
    //if(!SENDER)
    //{
        GLOBAL_STORAGE.nid[0]=0x4e;  //N
        GLOBAL_STORAGE.nid[1]=0x69; //i
        GLOBAL_STORAGE.nid[2]=0x64; //d
        GLOBAL_STORAGE.nid[3]=0x3c; //:
    //}
    /*else
    {
        GLOBAL_STORAGE.nid[0]=0x00;  //N
        GLOBAL_STORAGE.nid[1]=0x00; //i
        GLOBAL_STORAGE.nid[2]=0x00; //d
        GLOBAL_STORAGE.nid[3]=0x00; // 
    }*/
    
    
    GLOBAL_STORAGE.routing_tree[0] = 0;
    //GLOBAL_STORAGE.routing_tree[1] = 0;
    GLOBAL_STORAGE.routing_tree[1] = 2;
    //GLOBAL_STORAGE.routing_tree[2] = 1;
    GLOBAL_STORAGE.routing_tree[2] = 3;
    GLOBAL_STORAGE.routing_tree[3] = 4;
    GLOBAL_STORAGE.routing_tree[4] = 0;
    GLOBAL_STORAGE.routing_tree[5] = 1;
    GLOBAL_STORAGE.routing_tree[6] = 5;
    GLOBAL_STORAGE.routing_enabled = true;
    
    if(SENDER)
    {
        //GLOBAL_STORAGE.cid=0x01;
        //GLOBAL_STORAGE.parent_cid=0x02; //PAN
        GLOBAL_STORAGE.cid=0x01;
        GLOBAL_STORAGE.parent_cid=0x02; //PAN
    }
    else
    {
        //GLOBAL_STORAGE.cid=0x02;
        //GLOBAL_STORAGE.parent_cid=0x00;
        GLOBAL_STORAGE.cid=0x04;
        GLOBAL_STORAGE.parent_cid=0x00; //PAN
    }
    
    
    GLOBAL_STORAGE.edid[0]=0xCD;
    GLOBAL_STORAGE.edid[1]=0xED;
    GLOBAL_STORAGE.edid[2]=0x1D;
    if(SENDER)
    {
        GLOBAL_STORAGE.edid[3]=0x01;
    }
    else
    {                                                                                                                                                       
        GLOBAL_STORAGE.edid[3]=0x04;
    }
    
    /*GLOBAL_STORAGE.edid[0]=0x00;
    GLOBAL_STORAGE.edid[1]=0x00;
    GLOBAL_STORAGE.edid[2]=0x00;
    GLOBAL_STORAGE.edid[3]=0x00;*/
    
    uint8_t toed[4] = {0xCD, 0xED, 0x1D, 0x01};
    uint8_t topan[4] = {0x4E, 0x69, 0x64, 0x3C};

    LINK_init_t params;
    params.tx_max_retries = 4;
    params.rx_data_commit_timeout = 64;
    
    LINK_init(params);
    NET_init();
    // GLOBAL_STORAGE.pair_mode=true;
    
    uint8_t buf[] = {0x92,0x00,0x01,0x00,0x64,0x01,0x00,0x0A,0x00,0x04,0x00,0x00,0x0A,0x5E};
 
    printf("Link layer initialized on coordinator %d\n",GLOBAL_STORAGE.cid);
    //delay();
    //delay();
    
    //delay_ms(500);
    //printf("Link layer will be sending");
    //delay_ms(500);
    
    while(1)
    {
        if(SENDER)
        {
            // INTCONbits.GIE = 0;
            while(!fitp_send(4, NET_DIRECT_COORD, buf, 14));
            //while(!fitp_send(1, toed, buf, 14));
            // INTCONbits.GIE = 1;
            delay_ms(2000); 
        }
        //printf("main\n");
    }
    
    //LINK_send_join_request(NULL, 0);
    while(1)
    {
      if(SENDER)
      {
        if(PORTBbits.RB1 == 0)
        {
            printf("Zmacknuto tlacitko\n");
            // osetreni zakmitu
            delay_ms(50);
            // dokud nepovolime tlacitko...
            while(PORTBbits.RB1 == 0);
            join = true;
        }
          //printf("while\n");
          if(join)
          {
              printf("JOINING\n");
              set_rf_mode(RF_STANDBY); 
              set_rf_mode(RF_RECEIVER);
              fitp_join();
              //delay();
              //delay();
              //delay();
              //set_rf_mode(RF_SLEEP);  
              join = false;
          }
          if(fitp_joined())
          {
              printf("JOINED\n");
              for(uint8_t i = 0; i < 4; i++)
              {
                  printf("0x%02x ", GLOBAL_STORAGE.nid[i]);
              }
              printf("\n");
          }
        }
    }
    
    
    /*printf("LINK join response send\n");
    LINK_send_join_response((uint8_t*)"\xED\x00\x00\x01", (uint8_t*)"252525",6);
    printf("LINK join request send done\ndelay\nLINK move send\n");
    delay();
    
    
    //for next functions must be joined so... 
    GLOBAL_STORAGE.nid[0]=0x4e;  //N
    GLOBAL_STORAGE.nid[1]=0x69; //i
    GLOBAL_STORAGE.nid[2]=0x64; //d
    GLOBAL_STORAGE.nid[3]=0x3a;  //:
    
    
    LINK_send_move_response((uint8_t*)"\xED\x00\x00\x01", (uint8_t*)"336633",6);
    printf("LINK move response send done\ndelay\nLINK send\n");
    delay();
    
    
    LINK_send((uint8_t*)"53262",5);
    printf("LINK send done\ndelay\LINKROUTER move send\n");
    delay();
    
    
    LINKROUTER_send(true,(uint8_t*)"\xED\x00\x00\x01", (uint8_t*)"31415926535",11);
    

    loop:
      delay();
      goto loop;*/

}
