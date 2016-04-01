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

mqtt_iface *mq;

bool direction_up = true;
int main(void) {

	//for simulator
	//save pid
	int pid = getpid();
	printf("COORD PID: %d\n", pid);
 	for (int8_t i = 3; i >= 0; i--)
 		GLOBAL_STORAGE.pid[3-i] = (uint8_t) (pid >> (8*i)) &255;

	srand(time(NULL));
	uint8_t id_len = 10;
	char mqtt_id[id_len];
	gen_random(mqtt_id, id_len);

	mq = new mqtt_iface(mqtt_id,"localhost", 1883);
	//end

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
        GLOBAL_STORAGE.nid[0]=0x02;  //N
        GLOBAL_STORAGE.nid[1]=0x00; //i
        GLOBAL_STORAGE.nid[2]=0x00; //d
        GLOBAL_STORAGE.nid[3]=0x00; //:
    //}
    /*else
    {
        GLOBAL_STORAGE.nid[0]=0x00;  //N
        GLOBAL_STORAGE.nid[1]=0x00; //i
        GLOBAL_STORAGE.nid[2]=0x00; //d
        GLOBAL_STORAGE.nid[3]=0x00; //
    }*/


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
    //GLOBAL_STORAGE.pair_mode=true;

    uint8_t buf[] = {0x92,0x00,0x01,0x00,0x64,0x01,0x00,0x0A,0x00,0x04,0x00,0x00,0x0A,0x5E};

    //printf("Link layer initialized on coordinator %d\n",GLOBAL_STORAGE.cid);

	/*
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
    }*/


	while (1) {
       usleep(1000000); //100milisekund

    }




}
