#include "net.h"
#include "net_common.h"

#include "log.h"

#define NETROUTER_EP_BYPASS_PIZE 4
#define LINK_DATA_WITHOUT_ACK 0x01
#define MAX_ROUTING_DATA 40

typedef struct
{
  uint8_t signal_strenght;
  uint8_t from[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE]; //message in join and move can be bigger on link layer but they are send trought network protocol too so we can have shorter buffer
  uint8_t len:7;
  uint8_t empty:1;
} NET_movejoin_request_t;

typedef struct
{
  uint8_t address[4];
  uint8_t data[MAX_NET_PAYLOAD_SIZE];
  uint8_t len:6;
  uint8_t is_set:1;
} NET_direct_join_struct_t;

//TODO Bypass flag?
typedef struct
{
  uint8_t type;
  uint8_t scid;
  uint8_t sedid[4];
  uint8_t payload[MAX_NET_PAYLOAD_SIZE];
  uint8_t len;

} NET_current_processing_packet_t;

struct NET_storage_t
{
  /* this values are meneged by circular buffer functions dont touch them*/
  uint8_t cb_data[NETROUTER_EP_BYPASS_PIZE][MAX_NET_PAYLOAD_SIZE+1];
  uint8_t cb_start;
  uint8_t cb_end;
  bool cb_full;
  /* end of circular values*/
  NET_direct_join_struct_t direct_join;
  uint8_t state;
  uint8_t state_stash;
  NET_current_processing_packet_t processing_packet;

  NET_movejoin_request_t move_request;
  NET_movejoin_request_t join_request;
}NET_STORAGE;

extern void delay_ms(uint16_t maximum);
extern bool zero_address(uint8_t* addr);
uint8_t ROUTING_get_next(uint8_t destination_cid);

/**
 * @brief send using net frame
 */
bool send(uint8_t message_type, uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len, uint8_t transfer_type)
{
	uint8_t tmp[MAX_NET_PAYLOAD_SIZE];
	uint8_t index = 0;
	uint8_t address_coord;

	tmp[index++] = (message_type << 4) | ((tocoord >> 2) & 0x0F);
	printf("message_type|tocoord: %d\n", tmp[0]);
	tmp[index++] = ((tocoord << 6) & 0xC0) | (GLOBAL_STORAGE.cid & 0x3f);
	printf("tocoord|srccid: %d\n", tmp[1]);
	tmp[index++] = toed[0];
	tmp[index++] = toed[1];
	tmp[index++] = toed[2];
	tmp[index++] = toed[3];
	tmp[index++] = GLOBAL_STORAGE.edid[0];
	tmp[index++] = GLOBAL_STORAGE.edid[1];
	tmp[index++] = GLOBAL_STORAGE.edid[2];
	tmp[index++] = GLOBAL_STORAGE.edid[3];

	printf("send\\n");
	for(uint8_t i = 0; i < len && index < MAX_NET_PAYLOAD_SIZE; i++)
		tmp[index++]=data[i];

	/*for(uint8_t i = 0; i < index; i++)
	{
	  printf("0x%02x ", tmp[i]);
	}
	printf("\n");*/

	address_coord = ROUTING_get_next(tocoord);
	printf("address_coord: %02x\n", address_coord);
	return LINK_send(tmp,index, &address_coord, transfer_type);
}

void LINK_error_handler(bool rx, uint8_t* data, uint8_t len)
{
	if(rx)
		printf("LINK error during reciving\n");
	else
		printf("LINK error during transmitting\n");
}

// TODO: to be deleted?
void LINK_notify_send_done()
{
  ;
}

// TODO: to be deleted?
bool LINK_is_device_busy()
{
  ;
}

// TODO: to be deleted?
void LINK_process_packet(uint8_t* payload, uint8_t len)
{
	if(len < 10)
	{
		printf("Invalid net packet, too short\n");
		return;
	}
	printf("Link process\n");

	NET_STORAGE.processing_packet.type = payload[0] >> 4;
	NET_STORAGE.processing_packet.scid = payload[1] & 0x3f;

	for(uint8_t i=0;i<4;i++)
		NET_STORAGE.processing_packet.sedid[i] = payload[i + 6];

	uint8_t i = 0;
	for(;i < (len - 10) && i < MAX_NET_PAYLOAD_SIZE; i++)
		NET_STORAGE.processing_packet.payload[i] = payload[i + 10];
	NET_STORAGE.processing_packet.len = i;
}

// TODO: to be implemented
void LINK_join_response_recived(uint8_t* payload, uint8_t len)
{
	// check valid length
	if(len < 15)
		return;

	// if (NET_accept_join(payload + 10, payload[14])) { -> testing
	if (NET_accept_join(payload,payload[4])){
		for(uint8_t i = 0; i < 4; i++)
			GLOBAL_STORAGE.nid[i] = payload[i];

		GLOBAL_STORAGE.parent_cid = LINK_cid_mask(payload[4]);

		//TODO coord config is longer
		uint8_t cfg[5];
		cfg[0] = GLOBAL_STORAGE.nid[0];
		cfg[1] = GLOBAL_STORAGE.nid[1];
		cfg[2] = GLOBAL_STORAGE.nid[2];
		cfg[3] = GLOBAL_STORAGE.nid[3];
		cfg[4] = GLOBAL_STORAGE.parent_cid;
		// save_configuration(cfg, 5);

		printf("Join response recived.\n");
	}
}

// TODO: to be implemented
void LINK_join_request_recived(uint8_t ss, uint8_t* from, uint8_t* payload, uint8_t len)
{
	if(NET_STORAGE.join_request.empty)
	{
		//if join response for this device is alredy known send it
		if(NET_STORAGE.direct_join.is_set){
			bool can_direct = true;
			for(uint8_t i = 0; i < 4; i++){
				if(NET_STORAGE.direct_join.address[i] != from[i]){
					can_direct = false;
					break;
				}
			}
			if(can_direct){
				LINK_send_join_response(from,NET_STORAGE.direct_join.data,NET_STORAGE.direct_join.len);
				return;
			}
		}

		//store request until link layer can send it
		NET_STORAGE.join_request.signal_strenght = ss;

		for(uint8_t i = 0; i < 4; i++)
			NET_STORAGE.join_request.from[i] = from[i];

		uint8_t i = 0;
		for(; i < len && i < MAX_NET_PAYLOAD_SIZE - 6; i++) // in packet to pan must be extended header, from and signal strenght
			NET_STORAGE.join_request.payload[i] = payload[i];
		NET_STORAGE.join_request.len = i;

		NET_STORAGE.join_request.empty = 0;
	}
	else{
		printf("Cant process join request");//some previous join is alredy waiting
	}
}
// TODO: to be implemented?
void LINK_move_response_recived(uint8_t* payload, uint8_t len)
{
	// CP* ; C Byte of cid address, P Byte of real payload
	if(len < 5)
		return;

	GLOBAL_STORAGE.parent_cid = LINK_cid_mask(payload[1]);
}

// TODO: to be implemented?
void LINK_move_request_recived(uint8_t ss, uint8_t* from, uint8_t* payload, uint8_t len)
{
	if(NET_STORAGE.move_request.empty){
		NET_STORAGE.move_request.signal_strenght = ss;

		for(uint8_t i = 0; i < 4; i++)
			NET_STORAGE.move_request.from[i] = from[i];

		uint8_t i = 0;
		for(; i < len && i < MAX_NET_PAYLOAD_SIZE - 6 ; i++) // in packet to pan must be extended header, from and signal strenght
			NET_STORAGE.move_request.payload[i] = payload[i];
		NET_STORAGE.move_request.len = i;
		NET_STORAGE.move_request.empty = 0;
	}
	else{
		printf("Cant process move request");//one unprocessed join alredy stored
	}
}

/**
 * @ section routing
 * @brief routing inside NET layer
 */
void LINKROUTER_error_handler(bool rx, bool toEd, uint8_t* address, uint8_t*data , uint8_t len)
{
	if(rx)
		printf("LINKROUTER error during reciving\n");
	else
		printf("LINKROUTER error during transmiTting\n");
}

uint8_t ROUTING_get_next(uint8_t destination_cid)
{
	printf("dest_coord: %d\n", destination_cid);
	uint8_t address = GLOBAL_STORAGE.cid;
	uint8_t previous_address;

	while(1){
		// towards PAN
		address = GLOBAL_STORAGE.routing_tree[address];
		if(destination_cid == address){
			address = GLOBAL_STORAGE.parent_cid;
			return address;
		}
		if(address == 0 && destination_cid != 0){
			// the target device is next to me or under me
			address = 0xFF;
			break;
		}
	}
	if(address == 0xFF){
		address = destination_cid;
		while(1){
			printf("whiiile\n");
			if(address == GLOBAL_STORAGE.cid)
			{
				address = previous_address;
				return address;
			}
			previous_address = address;
			address = GLOBAL_STORAGE.routing_tree[address];
			if(address == 0)
				// the same level
				return GLOBAL_STORAGE.parent_cid;
		}
	}
	// to avoid warning
	return address;
}

bool LINKROUTER_route(uint8_t* payload , uint8_t len)
{
	printf("LINKROUTER_route\n");
	/*for(uint8_t i = 0; i < len; i++)
	{
	printf("%02x ", payload[i]);
	}
	printf("\n");*/
	if(len > 10){
		//get destination coordinator id ccccdddd|ddssssss c = control, d = dcid, s = scid
		uint8_t dcid = ((payload[0] << 2) & 0b00111100) | ((payload[1] >> 6) & 0b00000011);
		printf("dcid: %02x\n", dcid);
		//if for my or childs
		if(dcid == GLOBAL_STORAGE.cid){
			if(zero_address(payload+2)) {}
				//return cb_push(payload,len);
			else
				LINKROUTER_send(true, payload+2, payload, len);
		}
		else{
			// pokud zero_address(payload+2) neni TRUE, kuk do tabulky coord_ed_table
			// a podle EDID zjistit cilove ID COORD
			uint8_t next_hop_coord_address = LINK_cid_mask(ROUTING_get_next(dcid));
			LINKROUTER_send(false, &next_hop_coord_address, payload, len);
		}
	}
	return true;
}

/**
 * @ section net layer rest
 * @brief net layer functions
 */
void NET_init()
{
	NET_STORAGE.direct_join.is_set = false;

	GLOBAL_STORAGE.routing_enabled = false;

	//TODO coordinator need longer config as ed
	uint8_t cfg[5];
	// load_configuration(cfg, 5);

	if(cfg[0] != 0xFF && cfg[1] != 0xFF && cfg[3] != 0xFF && cfg[4] != 0xFF){
		GLOBAL_STORAGE.nid[0] = cfg[0];
		GLOBAL_STORAGE.nid[1] = cfg[1];
		GLOBAL_STORAGE.nid[2] = cfg[2];
		GLOBAL_STORAGE.nid[3] = cfg[3];
		GLOBAL_STORAGE.parent_cid = cfg[4];
	}
	else{
		GLOBAL_STORAGE.nid[0] = 0;
		GLOBAL_STORAGE.nid[1] = 0;
		GLOBAL_STORAGE.nid[2] = 0;
		GLOBAL_STORAGE.nid[3] = 0;
		GLOBAL_STORAGE.parent_cid = 0;
	}

	NET_STORAGE.join_request.empty = 1;
	NET_STORAGE.move_request.empty = 1;
}

bool NET_joined()
{
	if(GLOBAL_STORAGE.waiting_join_response)
		return false;

	/*for(uint8_t i = 0; i < 4; i++)
	{
	printf("0x%02x ", GLOBAL_STORAGE.nid[i]);
	}*/
	for(uint8_t i = 0; i < 4; i++){
		if(GLOBAL_STORAGE.nid[i] != 0)
			return true;
	}
	return false;
}


bool NET_send(uint8_t tocoord, uint8_t* toed, uint8_t* data, uint8_t len)
{
	return send(NET_PACKET_TYPE_DATA ,tocoord, toed, data, len, LINK_DATA_WITHOUT_ACK);
}


bool NET_join()
{
	// staci uint8_t packet[10];??? -> testing
	uint8_t tmp[53];
	uint8_t index = 0;

	if(GLOBAL_STORAGE.waiting_join_response)
		return false;

	tmp[index++] = (PT_DATA_JOIN_REQUEST << 4) & 0xF0;
	// device is a coordinator
	tmp[index++] = 0xCC;
	// dest ED == {0, 0, 0, 0}
	tmp[index++] = 0;
	tmp[index++] = 0;
	tmp[index++] = 0;
	tmp[index++] = 0;
	tmp[index++] = GLOBAL_STORAGE.edid[0];
	tmp[index++] = GLOBAL_STORAGE.edid[1];
	tmp[index++] = GLOBAL_STORAGE.edid[2];
	tmp[index++] = GLOBAL_STORAGE.edid[3];

	GLOBAL_STORAGE.waiting_join_response = true;
	if(LINK_send_join_request(tmp, index)){
		printf("NET_join(): link ack request received\n");
		for(uint8_t i = 0; i < 100; i++){
			// wait for i * delay_ms()
			delay_ms(10);
			if(!GLOBAL_STORAGE.waiting_join_response)
				break;
		}
	}

	if (GLOBAL_STORAGE.waiting_join_response){
		printf("NET_join(): timeout\n");
		GLOBAL_STORAGE.waiting_join_response = false;
		return false;
	}

	printf("NET_join(): success\n");
	return true;
}

/**
 * ROUTING
 */
	 //TODO added to link.c - 762 and comment out line 736
	 /*
	 //save routing table
	 if(equal_arrays(data+5, GLOBAL_STORAGE.edid, 4)){
		routing(data+20, len-20);
		return;
	 }
	 */

/**
 * select routing record
 */
bool select_routing_record(uint8_t from, uint8_t to){
	int i = 0;
	while(i < 64){
		 if (from == to)
			 return true;
		from = GLOBAL_STORAGE.routing_tree[from];
		i++;
	}
	return false;
}

/**
 * Send routing table
 */
void send_routing_table(uint8_t tocoord, uint8_t* euid, uint8_t* data, uint8_t len)
{
	uint8_t config_packet = 0;
	uint8_t packet_count = 0;

	//total packet
 	if(len%MAX_ROUTING_DATA != 0)
 		packet_count++;

	packet_count += len/MAX_ROUTING_DATA;
	config_packet = packet_count << 4;
	printf("TOTAL PACKET: %d\n", packet_count);
	printf("DATA LEN: %d\n", len);

  	//send
	int data_index = 0;
	uint8_t send_data[MAX_ROUTING_DATA+1];
	for(uint8_t j = 0; MAX_ROUTING_DATA*j < len; j++){
		config_packet++;

		// create part of packet, config byte + MAX_ROUTING_DATA
		send_data[0] = config_packet;
		uint8_t i;
		for (i = 1; i <= MAX_ROUTING_DATA && data_index < len; i += 2){
			// remove pan record
			if(send_data[i-2] == 0 && send_data[i-1] == 0)
				i -= 2; break;

			//send to coord only child coord
			if(data_index%2 == 0 && select_routing_record(data[data_index], tocoord)){
				//cout << "TMP::" << unsigned(data[data_index]) << endl;
				send_data[i] = data[data_index++];
				send_data[i+1] = data[data_index++];
				continue;
			}
			data_index += 2;
			i -= 2;
		}

		printf("CONFIG PACKET: %d\n", config_packet);

		send(PT_NETWORK_ROUTING_DATA, tocoord, euid, send_data, i, LINK_DATA_WITHOUT_ACK);
	}
}

void routing(uint8_t *data, uint8_t len)
{
	uint8_t r_table[128];
	int k = 0;

	//count of packet
	int count = data[0]>>4;
	int act_packet = data[0]&0x0f;

	printf("COUNT: %d\n", count);
	printf("ACT_PACKET: %d\n", act_packet);

	//save routing data to routing tree
	for(int i = 1; i < len; i += 2)
		GLOBAL_STORAGE.routing_tree[data[i]] = data[i+1];

	//routing table
	printf("ROUTING TABLE:\n");
	for(uint8_t i = 0; i < 64; i++){
		if (GLOBAL_STORAGE.routing_tree[i] == 0 && i != GLOBAL_STORAGE.cid) continue;

		printf("%d : %d\n", i, GLOBAL_STORAGE.routing_tree[i]);

		r_table[k] = i;
		r_table[k+1] = GLOBAL_STORAGE.routing_tree[i];
		k += 2;
	}

	//resend routing tree
	for(int i = 0; i < 16; i++){
		if(GLOBAL_STORAGE.routing_tree[i] != GLOBAL_STORAGE.cid)
			continue;

		printf("RESEND ROUTING TREE TO: %d\n", i);
		send_routing_table(i, GLOBAL_STORAGE.edid, r_table, k);
		/*if (select_routing_record(i, GLOBAL_STORAGE.cid)) {
			cout << "RESEND ROUTING TREE TO: " << i << endl;
			send_routing_table(i, GLOBAL_STORAGE.edid, r_table, k);
		}*/
	}
}
