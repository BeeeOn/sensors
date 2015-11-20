/*
 * Wrapper functions to adjust value read from external libraries
 * Here can be also limited range of values, multiple reads if needed etc.
 */
 
#include "include.h"
 
void getDs18b20Temp(void *p){    
    ds_get_temp((int32_t *) p);    // ds_prepare must be called before
}
void getSHT21Temp(void *p){    
    int16_t hum;
    int16_t temp;
    SHT21_get_data(&temp, &hum);
    (int32_t)(*((int32_t *) p)) = temp;
}
void getSHT21Hum(void *p){
    int16_t hum;
    int16_t temp;
    SHT21_get_data(&temp, &hum);
    (int32_t)(*((int32_t *) p)) = hum;
}
void getBatterySize(void *p) {
    uint16_t bat;     
    GetBatteryStatus(&bat);    
    *((uint16_t *) p) = bat*10;
}

void getRefreshTime(void *p) {
    //printf("getRefreshTime() %d\n", refresh_time);
    *((uint16_t *) p) = GLOBAL_STORAGE.refresh_time;
}
void setRefreshTime(void *p) {
    //refresh_time = (uint16_t *) p;
    GLOBAL_STORAGE.refresh_time = (uint16_t) ((((uint8_t *) p)[0] << 8) | ((uint8_t *) p)[1]);
    //printf("setRefreshTime() %d\n", refresh_time);
}


/*
 * Types and functions for sensor / actors manipulation
 */
static const tSenActControl senactList[] = {
    {MOD_ROOM_TEMPERATURE  , 4, RD_DISINT, getSHT21Temp, NULL},
    {MOD_OUTSIDE_TEMPERATURE, 4, RD_DISINT, getDs18b20Temp, NULL},
    {MOD_ROOM_HUMIDITY  , 4, RD_DISINT, getSHT21Hum, NULL},
    {MOD_BATTERY, 2, RD_DISINT, getBatterySize, NULL},
    {MOD_REFRESH, 2, WR_RDSEND, getRefreshTime, setRefreshTime},
};

bool sendDataPacket(uint8_t *buffer, uint8_t len) {
    return fitp_send(0, FITP_DIRECT_COORD, buffer, len); 
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Changing code bellow is normally not needed/////////////////////////////////

static const uint8_t senactListSize = sizeof(senactList)/sizeof(struct SenActControl);
uint8_t senactItems(void) {
    return senactListSize;
}

// Set actor value
// return true/false depending if item is in senact list
bool setActor(uint8_t module_id, void *val) { // type and typeOffset will be removed in next version of protocol
    uint8_t i;
    for (i = 0; i < senactListSize; i++) {
        if (senactList[i].setValue != NULL && senactList[i].module_id == module_id ) {            
            senactList[i].setValue(val);
            if (senactList[i].flags & WR_RDSEND) { // measure and send new value immediately back
                uint8_t buf[MAX_PACKET_SIZE];
                struct SensorPacket *sp = (struct SensorPacket *) buf;
                sp->cmd = FROM_SENSOR_MSG;
                sp->proto_ver = PROTO_VER;  // swap to big endian
                sp->device_id = BSWAP16(MULTISENZOR);   //device id
                sp->pairs = 1;  // only for this one value
                
                uint8_t byteIndex = sizeof (struct SensorPacket);
                *(buf + byteIndex) = senactList[i].module_id; // type 
                byteIndex++;
                for (uint8_t j=0; j<senactList[i].len; j++) {
                    *(buf + byteIndex) = *((uint8_t *) val+j);
                    byteIndex++;
                }
                
                // hack, we want to send data as normal device
                // (not ask for another data), see net.c and NET_send()
                GLOBAL_STORAGE.sleepy_device = false;                  
                sendDataPacket(buf, byteIndex);
                GLOBAL_STORAGE.sleepy_device = true;
                
            }
            return true;
        }
    }
    return false;
}

// set all actors from givem message
void setActors(uint8_t *data){
    uint8_t i, dataLen, pairs, module_id;
    
    pairs = data[0];    
    printf("PAIRS: %d\n", pairs);
    data++;
    
    for(i=0; i<pairs; i++) {
        module_id = data[0];
        data++;
        dataLen = data[0];
        data++;
        printf("DATA %d Module id: %d len: %d\n", data[0], module_id, dataLen);
        setActor(module_id, (void *) data);  
        data += dataLen;
     }    
}

/*
 * send value on given index in seactlist struct
 * Val_id - sensor id in senact list
 */
bool sendValues(void) {
    uint8_t i, j, byteIndex;
    uint8_t buf[MAX_PACKET_SIZE];
    uint8_t measured_value[8];  
    struct SensorPacket *sp;
    bool generateNewHeader = true;
    bool send_flag = false;    
        
    for(i=0; i<senactListSize; i++) {
        if (generateNewHeader) {
            sp = (struct SensorPacket *) buf;
            sp->cmd = FROM_SENSOR_MSG;
            sp->proto_ver = PROTO_VER;  
            sp->device_id = BSWAP16(MULTISENZOR); //device id
            sp->pairs = 0;
            byteIndex = sizeof(struct SensorPacket);
            generateNewHeader = false;
        }        

        if (senactList[i].getValue != NULL) {
            uint8_t ie;
            if (senactList[i].flags & RD_DISINT) {
                ie = INTCONbits.GIE;
                INTCONbits.GIE = 0;
            }
            senactList[i].getValue(measured_value);  // read data
            if (senactList[i].flags & RD_DISINT) {
                INTCONbits.GIE = ie;
            }            
            *(buf + byteIndex) = senactList[i].module_id;  // module_id
            byteIndex ++;
            for (j=0; j<senactList[i].len; j++) {                
                // swap endianess to [MSB]->[LSB]
                *(buf + byteIndex) = measured_value[senactList[i].len - j - 1];  
                byteIndex++;
            }        
            printf("\n");
            sp->pairs++;
        }

        if (i == senactListSize - 1 || byteIndex + senactList[i+1].len + 4 > MAX_PACKET_SIZE) {
            send_flag = sendDataPacket(buf, byteIndex);
            generateNewHeader = true;            
        }
    } 
    return send_flag;   
}
