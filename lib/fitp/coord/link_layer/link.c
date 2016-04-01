#include "link.h"
#include "log.h"

//#define LINK_COMMIT_ACK_MASK				0b11000000
#define LINK_COMMIT_ACK_MASK				0xc0
//#define LINK_ACK_COMMIT_ACK_MASK			0x10000000
#define LINK_ACK_COMMIT_ACK_MASK			0x80
//#define LINK_ACK_MASK						0xb0100000
#define LINK_ACK_MASK						0x40
//#define LINK_DEST_ED_TYPE					0b00100000
#define LINK_DEST_ED_TYPE					0x20
//#define LINK_DEST_COORD_TYPE				0x00010000
#define LINK_DEST_COORD_TYPE				0x10
//#define LINK_BUSY_MASK					0x00001000
#define LINK_BUSY_MASK						0x08
//#define LINK_MOVE_MASK					0x00000100
#define LINK_MOVE_MASK						0x04
//#define LINK_NO_HEADER_PAYLOAD_MASK		0x00000000
#define LINK_NO_HEADER_PAYLOAD_MASK			0x00


//LINK_COMMAND_MASK is used only in move
//1 response, 0 request
//#define LINK_COMMAND_MASK			0b00000010
#define LINK_COMMAND_MASK			0x02

// for determination of a device type
#define ADDRESS_COORD	0
#define ADDRESS_ED		1

//#define LINK_JOIN_CONTROL_RESPONSE 0b10000000
#define LINK_JOIN_CONTROL_RESPONSE	0x80

#define LINK_DATA_WITHOUT_ACK		0x01

// for JOIN messages
#define LINK_DATA_JOIN_REQUEST		0x03
#define LINK_DATA_JOIN_RESPONSE		0x04
#define LINK_ACK_JOIN_REQUEST		0x05

#define LINK_RX_BUFFER_SIZE 4
#define LINK_TX_BUFFER_SIZE 4

#define _XTAL_FREQ 8000000
#define MAX_CHANNEL 31

typedef struct
{
	uint8_t data[MAX_PHY_PAYLOAD_SIZE];
	uint8_t address_type:1;
	uint8_t empty:1;
	uint8_t len:6;
	uint8_t expiration_time;
	uint8_t seq_number;
	union
	{
		uint8_t coord;
		uint8_t ed[4];
	}address;
}LINK_rx_buffer_record_t;

typedef struct
{
	uint8_t data[MAX_PHY_PAYLOAD_SIZE];
	//uint8_t address_type:1;  allways ed accept only cid
	uint8_t empty:1;
	uint8_t len:6;
	uint8_t expiration_time;
}LINK_rx_buffer_record_ed_t;

typedef struct
{
	uint8_t data[MAX_PHY_PAYLOAD_SIZE];
	uint8_t address_type:1;
	uint8_t empty:1;
	uint8_t len:6;
	uint8_t state:1;                   //1 = commit message state, 0= data message state
	uint8_t expiration_time;
	uint8_t transmits_to_error;
	uint8_t seq_number;
	union
	{
		uint8_t coord;
		uint8_t ed[4];
	}address;
}LINK_tx_buffer_record_t;

typedef struct
{
	uint8_t data[MAX_PHY_PAYLOAD_SIZE];
	//uint8_t address_type:1;  allways ed send only cid
	uint8_t empty:1;
	uint8_t len:6;
	uint8_t state:1;                   //1 = commit message state, 0= data message state
	uint8_t expiration_time;
	uint8_t transmits_to_error;
}LINK_tx_buffer_record_ed_t;


struct LINK_storage_t
{
	uint8_t tx_max_retries;
	uint8_t rx_data_commit_timeout;

	uint8_t timer_counter;
	LINK_rx_buffer_record_t rx_buffer[LINK_RX_BUFFER_SIZE];
	LINK_tx_buffer_record_t tx_buffer[LINK_TX_BUFFER_SIZE];
	uint8_t tx_seq_number;

	LINK_rx_buffer_record_ed_t ed_rx_buffer;
	LINK_tx_buffer_record_ed_t ed_tx_buffer;
	bool link_ack_join_received;
} LINK_STORAGE;

uint8_t LINK_cid_mask(uint8_t address);

/*
 * Support functions
 */
bool equal_arrays(uint8_t* addr1,uint8_t* addr2,uint8_t size)
{
	for(uint8_t i = 0; i < size; i++){
		if(addr1[i] != addr2[i])
			return false;
	}
	return true;
}

bool zero_address(uint8_t* addr)
{
	return addr[0] ==0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0;
}

uint8_t get_free_index_tx()
{
	uint8_t freeIndex = LINK_TX_BUFFER_SIZE;

	for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
		if(LINK_STORAGE.tx_buffer[i].empty){
			freeIndex = i;
			break;
		}
	}

	return freeIndex;
}

uint8_t get_free_index_rx()
{
	uint8_t freeIndex = LINK_RX_BUFFER_SIZE;

	for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
		if(LINK_STORAGE.rx_buffer[i].empty){
		freeIndex = i;
		break;
		}
	}

	return freeIndex;
}

void gen_header(uint8_t *header_placement_pointer, bool asEd, bool toEd, uint8_t* address,  uint8_t packet_type, uint8_t headerPayload, uint8_t seq)
{
	if(asEd && toEd)
		// communication between EDs is not allowed
		return;

	uint8_t header_index=0;
	// control byte filling
	header_placement_pointer[header_index++] =  packet_type << 6 | (toEd?1:0) << 5 | (asEd?1:0) << 4 | (headerPayload & 0x0f);

	// copy nid
	for(uint8_t i = 0; i < 4; i++){
		header_placement_pointer[header_index++] = GLOBAL_STORAGE.nid[i];
	}

	// sending to ED
	if(toEd){
		for(uint8_t i = 0; i < 4; i++)
			// ED address - DST
			header_placement_pointer[header_index++] = address[i];
		// COORD address - SRC
		header_placement_pointer[header_index++] = GLOBAL_STORAGE.cid;
	}
	//sending to COORD
	else {
		if(asEd){
			// COORD address - DST
			//header_placement_pointer[header_index++] = GLOBAL_STORAGE.parent_cid;
			header_placement_pointer[header_index++] = *address;

			for(uint8_t i = 0; i < 4; i++)
				// ED aadress - SRC
				header_placement_pointer[header_index++] = GLOBAL_STORAGE.edid[i];
		}
		// as COORD
		else {
			// COORD address - DST
			header_placement_pointer[header_index++] = *address;
			// COORD address - SRC
			header_placement_pointer[header_index++] = GLOBAL_STORAGE.cid;
			// sequence number
			header_placement_pointer[header_index++] = seq;
		}
	}
}

void send_data(bool asEd, bool toEd, uint8_t* address, uint8_t seq, uint8_t* payload, uint8_t payload_len)
{
	uint8_t packet[63];
	gen_header(packet, asEd, toEd, address,  LINK_DATA_TYPE, 0, seq);
	//size of a header
	uint8_t packet_index = 10;
	for(uint8_t i =0; i < payload_len; i++)
		packet[packet_index++] = payload[i];
	PHY_send_with_cca(packet, packet_index);
}

void send_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq)
{
	uint8_t ack_packet[10];
	gen_header(ack_packet, asEd, toEd, address, LINK_ACK_TYPE, LINK_NO_HEADER_PAYLOAD_MASK, seq);
	printf("send_ack\n");
	PHY_send_with_cca(ack_packet,10);
}

void send_commit(bool asEd, bool toEd, uint8_t* address, uint8_t seq)
{
	uint8_t commit_packet[10];
	gen_header(commit_packet, asEd, toEd, address, LINK_COMMIT_TYPE, LINK_NO_HEADER_PAYLOAD_MASK, seq);
	printf("send_commit\n");
	PHY_send_with_cca(commit_packet,10);
}

void send_commit_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq)
{
	uint8_t commit_ack_packet[10];
	gen_header(commit_ack_packet, asEd, toEd, address, LINK_COMMIT_ACK_TYPE, LINK_NO_HEADER_PAYLOAD_MASK, seq);
	printf("send_commit_ack\n");
	PHY_send_with_cca(commit_ack_packet,10);
}

void send_busy_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq)
{
	uint8_t ack_packet[10];
	gen_header(ack_packet, asEd, toEd, address, LINK_ACK_TYPE, LINK_BUSY_MASK, seq);
	PHY_send_with_cca(ack_packet,10);
}

void send_busy_commit_ack(bool asEd, bool toEd, uint8_t* address, uint8_t seq)
{
	uint8_t commit_ack_packet[10];
	gen_header(commit_ack_packet, asEd, toEd, address, LINK_COMMIT_ACK_TYPE, LINK_BUSY_MASK, seq);
	printf("send_busy_commit_ack\n");
	PHY_send_with_cca(commit_ack_packet,10);
}

void ed_packet_proccess(uint8_t* data,uint8_t len)
{
	//address validity was checked in LINK_process_packet --> NO???!!!
	// packet type is ack or commit_ack
	if(data[0] & LINK_ACK_COMMIT_ACK_MASK){
		// packet type is ack
		if(!(data[0] & LINK_ACK_MASK)){
			if(!LINK_STORAGE.ed_tx_buffer.empty){
				// no space in a buffer - message can be resent 4x
				LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
				//if response is busy message, dont switch state and dont send commit
				if(!(data[0] & LINK_BUSY_MASK)){
					LINK_STORAGE.ed_tx_buffer.state = 1;
					LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3; //2 + 1 actual can arrive immediately, bigger waiting is not problem
					//send_commit(true, false, &data[9], 0);
					//send_commit(true,false,NULL,0);
				}
			}
		}
		//packet type is commit_ack
		else
		{
			if(!LINK_STORAGE.ed_tx_buffer.empty){
				// no space in a buffer - message can be resent 4x
				LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
				if(!(data[0] & LINK_BUSY_MASK)){
					LINK_STORAGE.ed_tx_buffer.empty = 1;
					LINK_notify_send_done();
				}
			}
		}

	}
	//packet type data or commit
	else {
		// packet type is data
		if(!(data[0] & LINK_ACK_MASK)){
			if(LINK_STORAGE.ed_rx_buffer.empty) {
				// save data with header
				uint8_t index = 0;
				for(uint8_t data_index = 0; data_index < len && data_index < MAX_PHY_PAYLOAD_SIZE; data_index++)
					LINK_STORAGE.ed_rx_buffer.data[index++] = data[data_index];
				LINK_STORAGE.ed_rx_buffer.len = index;
				LINK_STORAGE.ed_rx_buffer.empty = 0;
			}
			//device can accept only one packet in time so busy message is not needed, actual recived message must by same as packet in buffer
			//message is not waitin only ack was lost
			LINK_STORAGE.ed_rx_buffer.expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
			send_ack(true, false, &data[9], 0);
		}
		//packet type is commit
		else{
			// message is commited, we can now send it to process
			if(!LINK_STORAGE.ed_rx_buffer.empty){
				// no space in a buffer
				if(LINK_is_device_busy()){
					// we can accept message(handshake done), but we are busy somewhere in higher layer
					send_busy_commit_ack(true, false, &data[9], 0);
				}
				else {
					// message now can be accepted
					LINK_STORAGE.ed_rx_buffer.empty = 1;
					LINK_process_packet(LINK_STORAGE.ed_rx_buffer.data + 10 ,LINK_STORAGE.ed_rx_buffer.len - 10);
					send_commit_ack(true, false, &data[9], 0);
				}
			}
			else{
				send_commit_ack(true, false, &data[9], 0);
			}
		}
	}
}

extern void delay_ms(uint16_t maximum);
extern bool cb_push(uint8_t *data,uint8_t len);

void router_packet_proccess(uint8_t* data,uint8_t len)
{
	//packet type ack or commit_ack
	if(data[0] & LINK_ACK_COMMIT_ACK_MASK){
		printf("ack/commit_ack\n");
		// packet is ack
		if(!(data[0] & LINK_ACK_MASK)){
			printf("ack\n");
			//ack: (ED/COORD --> COORD) => commit: (COORD --> ED)
			if(data[0] & LINK_DEST_COORD_TYPE){
				printf("ack to coord\n");
				// commit for previously received ack message
				for(uint8_t i = 0; i < LINK_TX_BUFFER_SIZE; i++){
					//if not empty, with correct type (to ed) & same address (6 is start index of edid in header), its already stored
					// DATA message was sent to this ed
					if(!LINK_STORAGE.tx_buffer[i].empty){
						if(LINK_STORAGE.tx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.tx_buffer[i].address.ed, data + 6, 4)){
							LINK_STORAGE.tx_buffer[i].state = 1;
							LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 3;//2 +1 actual can arrive immediately, biger waiting is not problem
							printf("send_commit to ed\n");
							send_commit(false, true, LINK_STORAGE.tx_buffer[i].address.ed, 0);
							break;
						}
					}
				}
			}
			//ack: (ED/COORD --> COORD) => commit: (COORD --> COORD)
			else{
				printf("ack to ed\n");
				// commit for previously received ack message
				for(uint8_t i = 0; i < LINK_TX_BUFFER_SIZE; i++){
				//if not empty, with correct type (to coord) & same address(6 is index of coord in header), its already stored
				//LINK_STORAGE.tx_buffer[i].seq_number = data[7];
					// DATA message was sent to this coord
					if(!LINK_STORAGE.tx_buffer[i].empty){
						if(LINK_STORAGE.tx_buffer[i].address_type == 0 && LINK_STORAGE.tx_buffer[i].address.coord == data[6] && LINK_STORAGE.tx_buffer[i].seq_number == data[7]){
							LINK_STORAGE.tx_buffer[i].state = 1;
							LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 3; //2 +1 actual can arrive immediately, biger waiting is not problem
							printf("send_commit to coord\n");
							send_commit(false, false, &LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number);
							break;
						}
					}
				}
			}
		}
		//packet is commit_ack
		else{
			if(data[0] & LINK_DEST_COORD_TYPE){
				printf("commit_ack to ed\n");
				for(uint8_t i = 0; i < LINK_TX_BUFFER_SIZE; i++){
					//if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
					if(!LINK_STORAGE.tx_buffer[i].empty){
						if(LINK_STORAGE.tx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.tx_buffer[i].address.ed, data+6, 4)){
							LINK_STORAGE.tx_buffer[i].empty = 1;
							break;
						}
					}
				}
			}
			//else addressed by cid
			else{
				printf("commit_ack to coord\n");
				for(uint8_t i = 0; i < LINK_TX_BUFFER_SIZE; i++){
					//if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
					if(!LINK_STORAGE.tx_buffer[i].empty){
						if(LINK_STORAGE.tx_buffer[i].address_type == 0 && LINK_STORAGE.tx_buffer[i].address.coord == data[6] && LINK_STORAGE.tx_buffer[i].seq_number == data[7]){
							LINK_STORAGE.tx_buffer[i].empty = 1;
							break;
						}
					}
				}
			}
		}
	}
	// packet type data or commit
	else{
		printf("data or commit\n");
		// if packet is of type DATA
		if(!(data[0] & LINK_ACK_MASK)){
			printf("data\n");
			uint8_t sender_address_type = (data[0] & LINK_DEST_COORD_TYPE)?1:0;

			// if buffer is not full, save infos about data message
			uint8_t empty_index = get_free_index_rx();
			//printf("EMPTY_INDEX: %d\n", empty_index);
			if(empty_index < LINK_RX_BUFFER_SIZE){
				uint8_t data_index = 0;
				//printf("EMPTY_INDEX: %d\n", empty_index);
				for(; data_index < len && data_index < MAX_PHY_PAYLOAD_SIZE; data_index++){
					LINK_STORAGE.rx_buffer[empty_index].data[data_index]=data[data_index];
				}
				LINK_STORAGE.rx_buffer[empty_index].len = data_index;
				LINK_STORAGE.rx_buffer[empty_index].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
				LINK_STORAGE.rx_buffer[empty_index].empty = 0;
				LINK_STORAGE.rx_buffer[empty_index].address_type = sender_address_type;

				if(sender_address_type){
					for(uint8_t i = 0; i < 4; i++)
						LINK_STORAGE.rx_buffer[empty_index].address.ed[i] = data[6+i];
				} else{
					LINK_STORAGE.rx_buffer[empty_index].address.coord = LINK_cid_mask(data[6]);
					LINK_STORAGE.rx_buffer[empty_index].seq_number = data[7];
				}
			}
			else{
				send_busy_ack(false, sender_address_type, data+6, data[7]);
				//TODO: check if empty_index is not out od range
				LINK_STORAGE.rx_buffer[empty_index].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
			}

			//data: (ED/COORD --> COORD) => ack: (COORD --> ED)
			if(sender_address_type){
				// ack for previously received data message
				for(uint8_t i = 0; i < LINK_RX_BUFFER_SIZE; i++){
				//if not empty, with correct type & same address(6 is start index of edid in header), its already stored
					if(!LINK_STORAGE.rx_buffer[i].empty){
						if(LINK_STORAGE.rx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.rx_buffer[i].address.ed, data+6, 4)){
							send_ack(false, sender_address_type,data+6,0);
							LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
							return;
						}
					}
				}
			}
			//data: (ED/COORD --> COORD) => ack: (COORD --> COORD)
			else {
				// ack for previously received data message
				for(uint8_t i = 0; i < LINK_RX_BUFFER_SIZE; i++){
					//if not empty, with correct type & same address(6 is index of coord in header), its allredy stored
					if(!LINK_STORAGE.rx_buffer[i].empty){
						if(LINK_STORAGE.rx_buffer[i].address_type == 0 && LINK_STORAGE.rx_buffer[i].address.coord == data[6] && LINK_STORAGE.rx_buffer[i].seq_number == data[7]){
							send_ack(false, sender_address_type, data+6, data[7]);
							LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
							return;
						}
					}
				}
			}
		}//packet is commit message
		else{
			printf("commit\n");
			bool commit_ack = false;
			//commit: (ED/COORD --> COORD) => commit_ack: (COORD --> ED)
			if(data[0] & LINK_DEST_COORD_TYPE){
				// commit_ack for previously received commit message
				uint8_t i = 0;
				for(; i < LINK_RX_BUFFER_SIZE; i++){
					//if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
					if(!LINK_STORAGE.rx_buffer[i].empty){
						if(LINK_STORAGE.rx_buffer[i].address_type == 1 &&  equal_arrays(LINK_STORAGE.rx_buffer[i].address.ed, data+6, 4)){
							printf("commit routing\n");
							send_commit_ack(false, data[0] & LINK_DEST_COORD_TYPE, data+6, 0);
							commit_ack = true;
							bool pom = LINKROUTER_route(LINK_STORAGE.rx_buffer[i].data+10,LINK_STORAGE.rx_buffer[i].len-10);
							printf("BOOL: %d\n", pom);
							LINK_STORAGE.rx_buffer[i].empty=1;
							break;
						}
					}
				}
				if(!commit_ack){
					// send busy and renew expiration time
					printf("  -- commit send busy back\n");
					send_busy_commit_ack(false, data[0] & LINK_DEST_COORD_TYPE, data+6, data[7]);
					LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
				}

			}
			//commit: (ED/COORD --> COORD) => commit_ack: (COORD --> COORD)
			else{
				commit_ack = false;
				// commit_ack for previously received commit message
				uint8_t i = 0;
				for(;i < LINK_RX_BUFFER_SIZE; i++){
					//if not empty, with correct type & same address(6 is start index of edid in header), its allredy stored
					if(!LINK_STORAGE.rx_buffer[i].empty){
						if(LINK_STORAGE.rx_buffer[i].address_type == 0 && LINK_STORAGE.rx_buffer[i].address.coord == LINK_cid_mask(data[6]) && LINK_STORAGE.rx_buffer[i].seq_number == data[7]){
							printf("commit routing\n");
							send_commit_ack(false, data[0] & LINK_DEST_COORD_TYPE,data+6, data[7]);
							commit_ack = true;
							bool pom = LINKROUTER_route(LINK_STORAGE.rx_buffer[i].data+10,LINK_STORAGE.rx_buffer[i].len-10);
							printf("BOOL: %d\n", pom);
							LINK_STORAGE.rx_buffer[i].empty=1;
							break;
						}
					}
				}
				if(!commit_ack){
					//send busy and renew expiration time
					printf("  -- commit send busy back\n");
					send_busy_commit_ack(false, data[0] & LINK_DEST_COORD_TYPE,data+6,data[7]);
					LINK_STORAGE.rx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + LINK_STORAGE.rx_data_commit_timeout;
				}
			}
		}
	}
}

void check_buffers_state ()
{
	//check ed buffers
	//printf("LINK_STORAGE.ed_tx_buffer.empty: %d\n", LINK_STORAGE.ed_tx_buffer.empty);
	//is record after expiration
	if((!LINK_STORAGE.ed_rx_buffer.empty) && LINK_STORAGE.ed_rx_buffer.expiration_time == LINK_STORAGE.timer_counter){
		LINK_STORAGE.ed_rx_buffer.empty = 1;
		LINK_error_handler(true, LINK_STORAGE.ed_rx_buffer.data, LINK_STORAGE.ed_rx_buffer.len);
	}

	if((!LINK_STORAGE.ed_tx_buffer.empty) && LINK_STORAGE.ed_tx_buffer.expiration_time == LINK_STORAGE.timer_counter){
		if((LINK_STORAGE.ed_tx_buffer.transmits_to_error--) == 0){
			LINK_error_handler(false,LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
			LINK_STORAGE.ed_tx_buffer.empty = 1;
		}
		else{
			// try to resend the packet
			if(LINK_STORAGE.ed_tx_buffer.state)
				send_commit(true, false, NULL,0);
			else
				send_data(true, false, NULL, 0, LINK_STORAGE.ed_tx_buffer.data, LINK_STORAGE.ed_tx_buffer.len);
			//reshedule transmit
			LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 2;
		}
	}

	//check coord buffers
	for(uint8_t i = 0; i < LINK_RX_BUFFER_SIZE; i++){
		//is record after expiration
		if((!LINK_STORAGE.rx_buffer[i].empty) && LINK_STORAGE.rx_buffer[i].expiration_time == LINK_STORAGE.timer_counter){
		//report rx error
			if (LINK_STORAGE.rx_buffer[i].address_type){
				LINKROUTER_error_handler(true, true,
				LINK_STORAGE.rx_buffer[i].address.ed,
				LINK_STORAGE.rx_buffer[i].data,
				LINK_STORAGE.rx_buffer[i].len);
			}
			else{
				LINKROUTER_error_handler(true, false,
				&LINK_STORAGE.rx_buffer[i].address.coord,
				LINK_STORAGE.rx_buffer[i].data,
				LINK_STORAGE.rx_buffer[i].len);
			}
			LINK_STORAGE.rx_buffer[i].empty = 1;
		}
	}

	for(uint8_t i = 0; i < LINK_TX_BUFFER_SIZE; i++){
		//is record after expiration
		if((!LINK_STORAGE.tx_buffer[i].empty) && LINK_STORAGE.tx_buffer[i].expiration_time == LINK_STORAGE.timer_counter){
			if((LINK_STORAGE.tx_buffer[i].transmits_to_error--) == 0){
				if (LINK_STORAGE.tx_buffer[i].address_type){
					LINKROUTER_error_handler(false, true,
					LINK_STORAGE.tx_buffer[i].address.ed,
					LINK_STORAGE.tx_buffer[i].data,
					LINK_STORAGE.tx_buffer[i].len);
				}else{
					LINKROUTER_error_handler(false, false,
					&LINK_STORAGE.tx_buffer[i].address.coord,
					LINK_STORAGE.tx_buffer[i].data,
					LINK_STORAGE.tx_buffer[i].len);
				}
				LINK_STORAGE.tx_buffer[i].empty = 1;
			}
			else{
				// try to resend the packet
				if (LINK_STORAGE.tx_buffer[i].state){
					if(LINK_STORAGE.tx_buffer[i].address_type){
						send_commit(false, true, LINK_STORAGE.tx_buffer[i].address.ed, 0);
					}else{
						send_commit(false, false, &LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number);
					}
				}
				else{
					if(LINK_STORAGE.tx_buffer[i].address_type){
						send_data(false, true,LINK_STORAGE.tx_buffer[i].address.ed, 0, LINK_STORAGE.tx_buffer[i].data,LINK_STORAGE.tx_buffer[i].len);
				}
					else{
						send_data(false, false,&LINK_STORAGE.tx_buffer[i].address.coord, LINK_STORAGE.tx_buffer[i].seq_number, LINK_STORAGE.tx_buffer[i].data,LINK_STORAGE.tx_buffer[i].len);
					}
				}
				//reshedule transmit
				LINK_STORAGE.tx_buffer[i].expiration_time = LINK_STORAGE.timer_counter + 2;
			}
		}
	}
}

/*
 * Implemented function from phy layer
 */
void PHY_process_packet(uint8_t* data,uint8_t len)
{
	/*printf("RAW: ");
	for(uint8_t i = 0; i < 10; i++)
	{
		printf("0x%02X,",data[i]);
	}
	printf("\n");*/
	printf("PHY_process_packet\n");

	// packet is too short
	if(len <= 7)
		return;

	// handle join packets before NID check
	if((data[0] & 0x0F) == LINK_ACK_JOIN_REQUEST &&
		(data[0] >> 6) == LINK_ACK_TYPE){
		if(GLOBAL_STORAGE.waiting_join_response){
			// wait for the JOIN RESPONSE
			printf("RX: LINK_ACK_JOIN_REQUEST\n");
			LINK_STORAGE.link_ack_join_received = true;
		}
		return;
	}
	if((data[0] & 0x0F) == LINK_DATA_JOIN_RESPONSE &&
		(data[0] >> 6) == LINK_DATA_TYPE){
		printf("RX: GLOBAL_STORAGE.waiting_join_response: %d\n", GLOBAL_STORAGE.waiting_join_response);
		if(GLOBAL_STORAGE.waiting_join_response && LINK_STORAGE.link_ack_join_received) {
			LINK_join_response_recived(data+10, len-10);
			GLOBAL_STORAGE.waiting_join_response = false;
		}
		return;
	}

	// packet not in my network
	if(!equal_arrays(data+1,GLOBAL_STORAGE.nid,4))
		return;

	// TODO: verify, if a packet is from my parent
	// check if a source device is my child or my parent
	// packet can be from ED!
	// packet to ED (LINK_DEST_ED_TYPE = 0x20)
	if(data[0] & LINK_DEST_ED_TYPE){
		printf("PHY_process_packet packet for ED\n");
		if((data[0] & 0x0F) == LINK_DATA_WITHOUT_ACK){
			LINK_notify_send_done();

			//save and send routing table
			if(equal_arrays(data+5, GLOBAL_STORAGE.edid, 4))
				routing(data+20, len-20);

			return;
		}

		//packet for another ED
		if(!equal_arrays(data+5,GLOBAL_STORAGE.edid,4)){
			// routing packet
			//LINKROUTER_route(data+10, len-10);
			return;
		}

		//different cid can be accepted only if moved message
		if(data[0] & LINK_MOVE_MASK){
			if(data[0] & LINK_COMMAND_MASK) {
				LINK_move_response_recived(data+10, len-10);
				return;
			}
			else{
				//request must be received in router
			}
		}



		// process packet
		ed_packet_proccess(data,len);
	}
	// packet to COORD
	else{
		printf("PHY_process_packet packet for COORD\n");
		if(data[0] & LINK_MOVE_MASK){
			printf("PHY_process_packet router move request/response\n");
			if (!(data[0] & LINK_COMMAND_MASK)){
				printf("PHY_process_packet router move request\n");
				//TODO verify data + 6
				LINK_move_request_recived(PHY_get_noise(), data+6, data+10, len -10);
				return;
			}
		}

		// dont recive messages for COORD if routing is disabled and data are recived (message is not COMMIT ACK)
		// must recive other messages because there can be opened handshakes and they will failed if not
		if(!GLOBAL_STORAGE.routing_enabled && (data[0] & LINK_COMMIT_ACK_MASK) == 0){
			printf("PHY_process_packet router disabled\n");
			return;
		}

		printf("PHY_process_packet router packet\n");

		// packet for another COORD
		if(LINK_cid_mask(data[5]) != GLOBAL_STORAGE.cid){
			printf("Packet for another COORD!\n");
			return;
		}

		printf("Packet for this COORD!\n");

		// if packet is from COORD, verify it
		if(!(data[0] & LINK_DEST_COORD_TYPE)){
			printf("Packet is from COORD!/n");
			uint8_t sender_cid = LINK_cid_mask(data[6]);
			//packet must be from my parent or child
			// TODO: this check will be after this if-else command
			if( GLOBAL_STORAGE.routing_tree[GLOBAL_STORAGE.cid] != sender_cid &&
			GLOBAL_STORAGE.routing_tree[sender_cid] != GLOBAL_STORAGE.cid)
				return;
		}
		//else packet is from ED, cant be verifed, always received

		// packet routing
		router_packet_proccess(data,len);
	}
}

void PHY_timer_interupt(void)
{
	LINK_STORAGE.timer_counter++;
	check_buffers_state();
}

uint8_t LINK_cid_mask(uint8_t address)
{// CID - 6b
	return address & 0x3f;
}

void LINK_init(LINK_init_t params){

	LINK_STORAGE.tx_max_retries = params.tx_max_retries;
	LINK_STORAGE.rx_data_commit_timeout = params.rx_data_commit_timeout;

	for(uint8_t i=0;i<LINK_RX_BUFFER_SIZE;i++){
		LINK_STORAGE.rx_buffer[i].empty=1;
	}
	for(uint8_t i=0;i<LINK_TX_BUFFER_SIZE;i++){
		LINK_STORAGE.tx_buffer[i].empty=1;
	}

	LINK_STORAGE.ed_rx_buffer.empty=1;
	LINK_STORAGE.ed_tx_buffer.empty=1;

	//clear timeout counter
	LINK_STORAGE.timer_counter = 0;
	//clear tx sequence number
	LINK_STORAGE.tx_seq_number = 0;

}

bool LINK_send_join_request(uint8_t* payload, uint8_t payload_len)
{
	printf("LINK_send_join_request\n");
	//TODO: a packet declaration could be uint8_t packet[20];??? -> test!!!
	uint8_t packet[63];
	// size of a link header
	uint8_t packet_index = 10;
	bool result;
	uint8_t my_channel;

	// waiting for ACK JOIN REQUEST
	LINK_STORAGE.link_ack_join_received = false;
	// waiting for JOIN RESPONSE
	GLOBAL_STORAGE.waiting_join_response = true;

	// find out a current channel
	my_channel = PHY_get_channel();
	for(uint8_t i = 0; i <= MAX_CHANNEL; i++){
		// channel setting
		result = PHY_set_channel(i);
		if(result != false){
			// check if a channel is set successfully
			gen_header(packet, true, false, &GLOBAL_STORAGE.parent_cid, LINK_DATA_TYPE, LINK_DATA_JOIN_REQUEST, 0);
			for(uint8_t index = 0; index < payload_len; index++)
				// copy data from a net layer
				packet[packet_index++] = payload[index];

			PHY_send_with_cca(packet,packet_index);
			// waiting for ACK JOIN REQUEST
			// @TODO set as configuration variable
			//PAN -- COORD: minimum = delay_ms(17);
			//COORD -- COORD: minimum = delay_ms(21);
			//delay_ms(25);
			// suitable delay for a simulator
			delay_ms(1000);
			if(LINK_STORAGE.link_ack_join_received)
				// ACK JOIN REQUEST recived
				return LINK_STORAGE.link_ack_join_received;
		}
		else{
			printf("Wrong number of channels for band and bitrate.\n");
			return false;
		}
	}
	// no ACK JOIN REQUEST recived, set channel to a previous value
	result = PHY_set_channel(my_channel);
	printf("Default channel\n");
	return false;
  //}
  // ===========================================================================
  // RESI SE VE FUNKCI, KTERA POSILA JOIN REQUEST???
  /*if(!GLOBAL_STORAGE.waiting_join_response && LINK_STORAGE.link_ack_join_received)
  {// doslo k prijeti ACK JOIN REQUEST, ale nedoslo mi JOIN RESPONSE
      printf("ACK JOIN REQUEST, but no JOIN RESPONSE!\n");
      return false;
  }*/
  // ===========================================================================
}

void LINK_send_join_response(uint8_t *to, uint8_t* payload,uint8_t len)
{
	uint8_t packet[63];
	uint8_t packet_index=10;

	//control
	packet[0]=0;
	//nid
	packet[1]=0;
	packet[2]=0;
	packet[3]=0;
	packet[4]=0;
	//to
	packet[5]=to[0];
	packet[6]=to[1];
	packet[7]=to[2];
	packet[8]=to[3];

	packet[9]=LINK_JOIN_CONTROL_RESPONSE;

	for(uint8_t i=0; i < len && packet_index < MAX_PHY_PAYLOAD_SIZE; i++)
		packet[packet_index++] = payload[i];
	PHY_send_with_cca(packet,packet_index);
}

// to be implemented
void LINK_send_move_request(uint8_t* payload, uint8_t len)
{
	printf("Send move request\n");
	uint8_t packet[MAX_PHY_PAYLOAD_SIZE];
	gen_header(packet, true, false, NULL, LINK_DATA_TYPE, LINK_MOVE_MASK, 0);

	uint8_t packet_index = 10;
	for(uint8_t i=0; i < len; i++)
		packet[packet_index++] = payload[i];
	PHY_send_with_cca(packet, packet_index);

	printf("Send move request done\n");
}

void LINK_send_move_response(uint8_t *to, uint8_t* payload,uint8_t len)
{
	uint8_t packet[63];

	//control
	//adress type ed, packet type move, move type response
	//cpu more expensive variant gen_header(packet, false, true, to, LINK_DATA_TYPE, (LINK_MOVE_MASK | LINK_COMMAND_MASK), 0)
	packet[0] = LINK_DEST_ED_TYPE | LINK_MOVE_MASK | LINK_COMMAND_MASK;
	//nid
	packet[1] = GLOBAL_STORAGE.nid[0];
	packet[2] = GLOBAL_STORAGE.nid[1];
	packet[3] = GLOBAL_STORAGE.nid[2];
	packet[4] = GLOBAL_STORAGE.nid[3];
	//to
	packet[5] = to[0];
	packet[6] = to[1];
	packet[7] = to[2];
	packet[8] = to[3];
	//from not defined routing may not be routing_enabled
	packet[9] = 0;

	uint8_t packet_index = 10;

	for(uint8_t i=0; i < len && packet_index < MAX_PHY_PAYLOAD_SIZE; i++)
		packet[packet_index++] = payload[i];
	PHY_send_with_cca(packet,packet_index);
}

bool LINK_send(uint8_t* payload, uint8_t payload_len, uint8_t* address_next_coord, uint8_t transfer_type)
{
	printf("LINK_send\n");
	printf("Next coord address: %02x\n", *address_next_coord);
	if(!LINK_STORAGE.ed_tx_buffer.empty)
		return false;

	for(uint8_t i = 0; i < payload_len; i++)
		LINK_STORAGE.ed_tx_buffer.data[i] = payload[i];
	LINK_STORAGE.ed_tx_buffer.len = payload_len;
	LINK_STORAGE.ed_tx_buffer.state = 0;
	LINK_STORAGE.ed_tx_buffer.transmits_to_error = LINK_STORAGE.tx_max_retries;
	LINK_STORAGE.ed_tx_buffer.expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately
	LINK_STORAGE.ed_tx_buffer.empty = 0;
	//send_data(true, false, address_next_coord, 0, LINK_STORAGE.ed_tx_buffer.data,LINK_STORAGE.ed_tx_buffer.len);
	//return true;

	if(transfer_type == LINK_DATA_WITHOUT_ACK){
		uint8_t packet[63];
		//size of link header
		uint8_t packet_index = 10;

		gen_header(packet, true, false, address_next_coord, LINK_DATA_TYPE, LINK_DATA_WITHOUT_ACK, 0);
		for(uint8_t index=0;index<payload_len;index++)
			packet[packet_index++] = payload[index];
		PHY_send_with_cca(packet,packet_index);
		return true;
	}
}

bool LINKROUTER_send(bool toEd, const uint8_t* address, uint8_t* payload, uint8_t payload_len)
{

	uint8_t freeIndex = get_free_index_tx();

	if(freeIndex >= LINK_TX_BUFFER_SIZE)
		return false;

	for(uint8_t i = 0; i < payload_len;i++)
		LINK_STORAGE.tx_buffer[freeIndex].data[i] = payload[i];
	LINK_STORAGE.tx_buffer[freeIndex].len = payload_len;
	//save sequence number to packet and increment this number
	LINK_STORAGE.tx_buffer[freeIndex].seq_number = LINK_STORAGE.tx_seq_number++;
	if(toEd){
		for(uint8_t i = 0; i < 4; i++)
			LINK_STORAGE.tx_buffer[freeIndex].address.ed[i] = address[i];
	}
	else{
		LINK_STORAGE.tx_buffer[freeIndex].address.coord = *address;
	}
	LINK_STORAGE.tx_buffer[freeIndex].address_type = toEd?ADDRESS_ED:ADDRESS_COORD;
	LINK_STORAGE.tx_buffer[freeIndex].state = 0;
	LINK_STORAGE.tx_buffer[freeIndex].transmits_to_error = LINK_STORAGE.tx_max_retries;
	LINK_STORAGE.tx_buffer[freeIndex].expiration_time = LINK_STORAGE.timer_counter + 3;// 2+1, interupt can arive immediately
	LINK_STORAGE.tx_buffer[freeIndex].empty = 0;

	if (LINK_STORAGE.tx_buffer[freeIndex].address_type)
		send_data(false, true, LINK_STORAGE.tx_buffer[freeIndex].address.ed, 0, LINK_STORAGE.tx_buffer[freeIndex].data, LINK_STORAGE.tx_buffer[freeIndex].len);
	else
		send_data(false, false, &LINK_STORAGE.tx_buffer[freeIndex].address.coord, LINK_STORAGE.tx_buffer[freeIndex].seq_number, LINK_STORAGE.tx_buffer[freeIndex].data, LINK_STORAGE.tx_buffer[freeIndex].len);

	return true;
}
