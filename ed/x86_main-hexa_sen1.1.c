#pragma config OSC = INTOSCPLL, WDTEN = OFF, XINST = OFF, WDTPS = 512

#include "include.h"
#include "senact.h"

extern bool accel_int;

uint8_t fitp_recived(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part) {
    D_G printf("ED received data: ");
    for(uint8_t i=0; i<len; i++) {
        D_G printf("%d ", data[i]);
    }
    D_G printf("\n");
    if(data[0] == SET_ACTORS) {
        //setActors(data+2);  // skip message type and protocol version
    }
}

bool fitp_accept_join(uint8_t nid[4], uint8_t parent_cid) {
    printf("acepting join: true\n");
    return true;
}

extern   void load_configuration(uint8_t *buf, uint8_t len);
extern   void save_configuration(uint8_t *buf, uint8_t len);

mqtt_iface *mq; //for simulator

int main(void) {
	//for simulator
	//ulozenie pid
	int pid = getpid();
	printf("ED PID: %d\n", pid);
 	for (int8_t i = 3; i >= 0; i--)
 		GLOBAL_STORAGE.pid[3-i] = (uint8_t) (pid >> (8*i)) &255;

	srand(time(NULL));
	uint8_t id_len = 10;
	char mqtt_id[id_len];
	gen_random(mqtt_id, id_len);

	mq = new mqtt_iface(mqtt_id,"localhost", 1883);
	//end


    PHY_init_t phy_params;
    LINK_init_t link_params;

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

    GLOBAL_STORAGE.sleepy_device = true;

    while (1) {
		usleep(1000000); //1s
    }
}
