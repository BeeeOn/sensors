#include "include.h"
#include "global.h"

#define TIMEOUT 60
#define RX_BUFFER_SIZE 128


mqtt_iface::mqtt_iface(const char *id, const char *host, int port) : mosquittopp(id) {
	mosqpp::lib_init();
	//cout << "Mosquito initialization " << host << ":" << port << endl;
	int rc = connect(host, port, TIMEOUT);
	//cout << "RC connect: " << rc << endl;
	loop_start();
} 

mqtt_iface::~mqtt_iface() {
}

void mqtt_iface::on_message(const struct mosquitto_message *message) {
	std::string msg_topic(message->topic);
	std::string msg_text((const char*)message->payload);

	if(msg_topic.compare(CONFIG_TOPIC) == 0 || msg_topic.compare(DATA_TO_TOPIC) == 0){
		vector<int> vec;
		string s(msg_text);
		size_t pos = 0;
		string token;
		string delimiter = ",";

		while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);
			vec.push_back(stoi(token));
			s.erase(0, pos + delimiter.length());
		}
		
		uint8_t recived_packet[100];
		int recived_len = 0;

		for(std::vector<int>::iterator it = vec.begin(); it != vec.end(); it++) {
			recived_packet[recived_len++] = (uint8_t)*it;
		}
	

		
		//overenie ci sa pid rovnaju, ci je sprava pre toto zariadenie
		uint32_t id = 0;
		bool pid = true;
		if (recived_packet[0] == 0){
			uint8_t len = recived_packet[1];
			
			for (uint8_t i = 0; i< 4; i++)
				if (recived_packet[i] != GLOBAL_STORAGE.pid[i]) {
					pid = false;
					break;
				}
		} 
		else 
			pid = false;			
		

		//preskocenie ak udaje nepatrili spravnemu zariadeniu
		if (pid) {
			cout << "-------------------------------------------------------------" << endl;
			cout << "ED: Message received on " << msg_topic << ": " << msg_text << endl;
		
			if(msg_topic.compare(CONFIG_TOPIC) == 0) {
				//Config 
				cout << "ED CONFIG DATA::" << endl;
				
				//preskocenie hlavicky - 8bajtov
				set_config(recived_packet+8, recived_len-8);

			} else {
				//Data
			       if(recived_len<63)
					recived_packet[recived_len]=0;
			    
			       PHY_process_packet(recived_packet + 8 , recived_len - 8);
			}
		}
		
		
	}
	
	
}

void mqtt_iface::on_connect(int rc) {
	//printf("Mosquito connecting with code %d.\n", rc);
	if (rc == 0) {
		subscribe(NULL, "BeeeOn/#");
	}
}

void mqtt_iface::on_subscribe(int mid, int qos_count, const int *granted_qos) {
	
}
bool mqtt_iface::send_message(const char *topic, const char * msg, uint8_t len) {
 	int ret = publish(NULL, topic,len,msg,1,false);
}





