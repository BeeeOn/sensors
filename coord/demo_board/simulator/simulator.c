#include "include.h"

void set_config(uint8_t *msg, uint8_t len) {
	uint8_t i = 0;
	uint8_t id_hodnoty = 0;
	uint8_t dlzka_hodnoty = 0;

	while (i < len) {
		id_hodnoty = msg[i++]; //preskocenie na dlzku hodnoty
		dlzka_hodnoty = msg[i++]; // preskocenie na data

		//Cyklus, ktory prechadza data
		for (uint8_t j = 0; j < dlzka_hodnoty; j++) {

			//Vyber a ulozenie dat pre spravnu hodnotu
			switch (id_hodnoty) {
				case 0:
					//printf("CMD: %d\n", msg[i+j]);
					run_cmd(msg[i+j]);
					break;
				case 1:
					GLOBAL_STORAGE.edid[j] = msg[i+j];
					printf("EDID: %d\n", GLOBAL_STORAGE.edid[j]);
					break;
				case 2:
					GLOBAL_STORAGE.cid = msg[i+j];
					printf("CID: %d\n", GLOBAL_STORAGE.cid);
					break;
				case 3:
					GLOBAL_STORAGE.nid[j] = msg[i+j];
					printf("NID: %d\n", GLOBAL_STORAGE.nid[j]);
					break;
				case 4:
					GLOBAL_STORAGE.channel = msg[i+j];
					printf("CHANNEL: %d\n", GLOBAL_STORAGE.channel);
					break;
				case 5:
					GLOBAL_STORAGE.bitrate = msg[i+j];
					printf("BITRATE: %d\n", GLOBAL_STORAGE.bitrate);
					break;
				case 6:
					GLOBAL_STORAGE.band = msg[i+j];
					printf("BAND: %d\n", GLOBAL_STORAGE.band);
					break;
				case 7:
					GLOBAL_STORAGE.rssi = msg[i+j];
					printf("RSSI: %d\n", GLOBAL_STORAGE.rssi);
					break;
				case 8:
					GLOBAL_STORAGE.tocoord = msg[i+j];
					printf("TOCOORD: %d\n", GLOBAL_STORAGE.tocoord);
					break;
				case 9:
					GLOBAL_STORAGE.toed[j] = msg[i+j];
					printf("TOED: %d\n", GLOBAL_STORAGE.toed[j]);
					break;
				case 10:
					GLOBAL_STORAGE.data_len = msg[i+j];
					printf("DATA_LEN: %d\n", GLOBAL_STORAGE.data_len);
					break;
				case 11:
					GLOBAL_STORAGE.data[j] = msg[i+j];
					printf("DATA: %d\n", GLOBAL_STORAGE.data[j]);
					break;
				case 13:
					GLOBAL_STORAGE.parent_cid = msg[i+j];
					printf("PCID: %d\n", GLOBAL_STORAGE.parent_cid);
					break;
			}
		}



		i += dlzka_hodnoty; //preskocenie dat a pokracovanie na dalsiu hodnotu
	}
}

void run_cmd(uint8_t cmd) {
	switch(cmd) {
		case 0:
			fitp_send(GLOBAL_STORAGE.tocoord,
				   GLOBAL_STORAGE.toed,
				   GLOBAL_STORAGE.data,
				   GLOBAL_STORAGE.data_len);
			break;
		case 1:
			{
				std::thread th_set_config (fitp_join);
				th_set_config.detach();
				cout << "fitp_join()" << endl;
			}
			break;

		case 6:
			{
			string msg = create_head();

			//EDID
			msg += "1,4,";
			for (int i =0; i < 4; i++)
				msg += to_string(GLOBAL_STORAGE.edid[i]) + ",";

			//CID
			msg += "2,1," + to_string(GLOBAL_STORAGE.cid) + ",";

			//NID
			msg += "3,4,";
			for (int i =0; i < 4; i++)
				msg += to_string(GLOBAL_STORAGE.nid[i]) + ",";

			//CHANNEL
			msg += "4,1," + to_string(GLOBAL_STORAGE.channel) + ",";

			//BITRATE
			msg += "5,1," + to_string(GLOBAL_STORAGE.bitrate) + ",";

			//BAND
			msg += "6,1," + to_string(GLOBAL_STORAGE.band) + ",";

			//RSSI
			msg += "7,1," + to_string(GLOBAL_STORAGE.rssi) + ",";

			//PCID
			msg += "13,1," + to_string(GLOBAL_STORAGE.parent_cid) + ",";

			mq->publish(NULL, "BeeeOn/config_from", msg.length(), msg.c_str());
			}
			break;

		default:
			printf("nefunkcne");
	}
}
