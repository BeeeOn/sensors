#ifndef SENACT_H
#define SENACT_H

// protoco dependent defines
#define MAX_PACKET_SIZE 32
#define PROTO_VER       0x01
#define FROM_SENSOR_MSG 0x92
#define SET_ACTORS      0xB0

// device_ID - devices.xml
#define MULTISENZOR     0x00
#define REGULATORVPT    0x01

// module_ID - devices.xml
#define MOD_ROOM_TEMPERATURE        0x00
#define MOD_OUTSIDE_TEMPERATURE     0x01
#define MOD_ROOM_HUMIDITY           0x02
#define MOD_BATTERY                 0x03
#define MOD_RSSI                    0x04
#define MOD_REFRESH                 0x05

#pragma pack(1)

// swap little and big endian
#define BSWAP16(n) ((n) << 8 | ((n) >> 8 & 0x00FF))
//#define BSWAP32(n) ((n) >> 24) | (((n) << 8) & 0x00FF0000L) | (((n) >> 8) & 0x0000FF00L) | ((n) << 24)
#define BSWAP32(n) (((n) >> 24) & 0x000000FFL) | (((n) << 8) & 0x00FF0000L) | (((n) >> 8) & 0x0000FF00L) | (((n) << 24) & 0xFF000000L)

// packet header
typedef struct SensorPacket {
    uint8_t cmd;               // command
    uint8_t proto_ver;         // application protocol version
    uint16_t device_id;             // device_id, devices.xml, VPT, Honeywell
    uint8_t pairs;             // number of type:valu pairs
} tSensorPacket;

// flags when reading or writing data
#define SEND_EERYTIME 0b00000001  // send data everytime readed
#define SEND_ONCHANGE 0b00000010  // send data when changed only
#define SEND_DISABLE  0b00000011  // do not send data
#define RD_DISINT     0b00000100  // disable interrupt when reading sensor
#define WR_DISINT     0b00001000  // disable interrupt when writing sensor
#define WR_RDSEND     0b00010000  // write value, read it immediately and send back

typedef struct SenActControl {
    uint8_t module_id; // module_id on given device, devices.xml
    uint8_t len; // value length (bytes)
    uint8_t flags;  // (not implemented yet)
    // send everytime, or on change 
    // disable interrupts during set/get     
    void (*getValue)(void *); // vycteni ze senzoru dat do pameti
    void (*setValue)(void *); // nastaveni dat
} tSenActControl;


  uint8_t senactItems(void);
void setActors(uint8_t *data);
bool sendValues(void);
void sendAllValues(void);
void checkStoredMsg(void);

#endif  /* SENACT_H */

