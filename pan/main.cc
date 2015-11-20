#include <iostream>
#include <thread>
#include <chrono>
#include <map>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>

#include <iostream>

#include "phy.h"
#include "link.h"
#include "net.h"
#include "fitp.h"
#include "global.h"
#include "log.h"
#include "mqtt_iface.h"

#include <Poco/Util/IniFileConfiguration.h>
#include <Poco/AutoPtr.h>
using Poco::AutoPtr;
using Poco::Util::IniFileConfiguration;

#define INI_FILE_PATH "/etc/beeeon/fitprotocold.ini"
#define DEVICES_TABLE_PATH "/var/lib/beeeon/fitprotocold.devices"

void delay_ms(uint16_t t) {
    //sleep(t/1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(t));
}

//class mqtt_iface *mq;
void test_cb(const uint8_t* addr, const uint8_t* data, uint8_t len) {
	cout << "CALLBACK" << endl;
	for(int i=0; i<len; i++) {
		printf("%d ", data[i]);
	}
	cout << endl;
	NET_send(0, (uint8_t *) addr, (uint8_t *) data, len, true);
}
int rc;
mqtt_iface *mq;


bool sent = false;
                          

uint8_t NET_received(const uint8_t from_cid, const uint8_t from_edid[4], const uint8_t* data, const uint8_t len, const uint8_t part){
  std::cout<<"================================================"<<std::endl;
  cout << dec << unsigned(len) << endl;

  mq->process_sensor_msg(from_edid, data, len-10);
          
//  std::cout<<"Message recived"<<reinterpret_cast<const char*>(data)<<std::endl;
/*  if(!sent){
    uint8_t resp[]="Response";
    NET_send(false, from_edid, resp, 9);
  }
  sent=true;
*/
  return 255;
}


bool NET_accept_join(uint8_t nid[4], uint8_t parent_cid){
  static int req_count = 0;
/*
  if(req_count != 3){
    std::cout<<"User dont want join this payload_len: "<<len<<std::endl;
    req_count++;
    return 255;
  }
*/
  return 0;
}

/**
 * init_from_file()
 * Initialize structures from .ini file
 * located at INI_FILE_PATH.
 *
 * return 0 on success
 * return negative value on error
 * */
char init_from_file(PHY_init_t *params)
{
    std::string edid[4];
    std::string device_table_path;
    int channel;
    AutoPtr<IniFileConfiguration> cfg;

    try { 
        cfg = new IniFileConfiguration(INI_FILE_PATH);

        edid[0] = cfg->getString("net_config.edid0");
        edid[1] = cfg->getString("net_config.edid1");
        edid[2] = cfg->getString("net_config.edid2");
        edid[3] = cfg->getString("net_config.edid3");
        channel = cfg->getInt("net_config.channel");
        device_table_path = cfg->getString("net_config.device_table_path");
    }

    catch (Poco::Exception& ex) {
        return -1;
    }

    params->channel = channel;
    params->cca_noise_treshold = 30;
    params->bitrate = DATA_RATE_66;
    params->band = BAND_863;
    params->power = TX_POWER_10_DB;

    GLOBAL_STORAGE.edid[0] = strtol(edid[0].c_str(),NULL,16);
    GLOBAL_STORAGE.edid[1] = strtol(edid[1].c_str(),NULL,16);
    GLOBAL_STORAGE.edid[2] = strtol(edid[2].c_str(),NULL,16);
    GLOBAL_STORAGE.edid[3] = strtol(edid[3].c_str(),NULL,16);
    
    GLOBAL_STORAGE.nid[0] = GLOBAL_STORAGE.edid[0];
    GLOBAL_STORAGE.nid[1] = GLOBAL_STORAGE.edid[1];
    GLOBAL_STORAGE.nid[2] = GLOBAL_STORAGE.edid[2];
    GLOBAL_STORAGE.nid[3] = GLOBAL_STORAGE.edid[3];
    
    GLOBAL_STORAGE.device_table_path = device_table_path;

    return 0;
}
    
int main( int argc, char** argv )
{       
    std::cout << "START fitprotocold" << std::endl;
    mq = new mqtt_iface("PAN", "localhost", 1883, &test_cb);

    PHY_init_t params;

    if (init_from_file(&params)==0)
    {
        std::cout<<"PHY configured from ini file"<<std::endl;
    }
    else
    {
        std::cout<<"PHY configuration from ini file error, setting default values"<<std::endl;
        params.cca_noise_treshold = 30;
        params.bitrate = DATA_RATE_66;
        params.band = BAND_863;
        params.channel = 28;
        params.power = TX_POWER_13_DB;

        GLOBAL_STORAGE.edid[0]=0x4e; //N
        GLOBAL_STORAGE.edid[1]=0x69; //i
        GLOBAL_STORAGE.edid[2]=0x64; //d
        GLOBAL_STORAGE.edid[3]=0x3c; //:
        
        GLOBAL_STORAGE.nid[0]=0x4e; //N
        GLOBAL_STORAGE.nid[1]=0x69; //i
        GLOBAL_STORAGE.nid[2]=0x64; //d
        GLOBAL_STORAGE.nid[3]=0x3c; //:
        
        GLOBAL_STORAGE.device_table_path = DEVICES_TABLE_PATH;
    }
    
    PHY_init(params);                 

    /*
    GLOBAL_STORAGE.routing_tree[0] = 0;
    GLOBAL_STORAGE.routing_tree[1] = 0;
    GLOBAL_STORAGE.routing_tree[2] = 0;
    GLOBAL_STORAGE.routing_tree[3] = 4;
    GLOBAL_STORAGE.routing_tree[4] = 0;
    */
    
    LINK_init_t link_params;
    link_params.tx_max_retries = 4;
    link_params.rx_data_commit_timeout = 64;
    LINK_init(link_params);
    
    NET_init();
    
    //NET_joining_disable();
    //GLOBAL_STORAGE.pair_mode = true;
    std::cout << "NID: ";
    std::cout << setfill('0') << std::setw(2) << std::hex << unsigned(GLOBAL_STORAGE.nid[0]) << " ";
    std::cout << setfill('0') << std::setw(2) << std::hex << unsigned(GLOBAL_STORAGE.nid[1]) << " ";
    std::cout << setfill('0') << std::setw(2) << std::hex << unsigned(GLOBAL_STORAGE.nid[2]) << " ";
    std::cout << setfill('0') << std::setw(2) << std::hex << unsigned(GLOBAL_STORAGE.nid[3]) << " ";
    std::cout << std::endl;
    std::cout << "Config file path: " << INI_FILE_PATH << endl;
    std::cout << "Device table path: " << GLOBAL_STORAGE.device_table_path << endl;
    std::cout << "fitp init finished"<<std::endl;
    
	uint8_t toed[4] = {0xCD, 0xED, 0x1D, 0x01};
    uint8_t buf[] = {0x92,0x00,0x01,0x00,0x64,0x01,0x00,0x0A,0x00,0x04,0x00,0x00,0x0A,0x5E}; 

	mq->loop_start();
	while(1) {
 		fflush(stdout);
		fflush(stderr);
  	//while(!fitp_send(0, toed, buf, 14));
		//std::this_thread::sleep_for(std::chrono::seconds(1));
/*
        	rc = mq->loop();
	        if(rc) {
	                mq->reconnect();
        	}
*/
	}

    PHY_stop(); //this will detach threads used for hw layer
}