#include "include.h"

#define OW_LAT LATCbits.LATC2
#define OW_TRIS TRISCbits.TRISC2
#define OW_PIN PORTCbits.RC2

#define _XTAL_FREQ 8000000  // for using __delay_us() function

void DsDeInit(void) {
    OW_TRIS = 1;
}

uint8_t OwReset(void) {    
    short device_found=0;    
    
    OW_TRIS = 0;
    OW_LAT = 0;
    __delay_us(480);
    OW_TRIS = 1;
    __delay_us(60);
    device_found = !OW_PIN;
    __delay_us(480);
    return device_found;
}

void OwWriteBit(uint8_t data) {
    if (data) {
        // Write '1' bit
        OW_TRIS = 0; 
        __delay_us(2);
        OW_TRIS = 1; 
        __delay_us(61);
    } else {
        // Write '0' bit
        OW_TRIS = 0; 
        OW_LAT=0;
        __delay_us(63);
        OW_TRIS = 1; 
    }
}

void OwWriteByte(int8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        OwWriteBit(data & 0x01);
        data >>= 1;
    }
}

uint8_t OwReadBit(void) {
    uint8_t result;
       
    OW_TRIS = 0;
    __delay_us(2);
    OW_TRIS = 1;
    __delay_us(8);
    result = OW_PIN;       
    __delay_us(53);
    return result;
}

int8_t OwReadByte(void) {
    int8_t result=0;

    for (uint8_t i= 0; i < 8; i++)
        result = (OwReadBit() << i) | result;
            
    return result;
}

void ds_prepare() {
    OwReset();
    
    ADCON1 |= 0x0F;
    OwWriteByte(0xCC);
    OwWriteByte(0x44);
}

void ds_get_temp(int32_t *res) {
	static int32_t tmp = INT32_MAX;
	
    OwReset();
    OwWriteByte(0xCC);
    OwWriteByte(0xbe);
    
    int8_t temp_low = OwReadByte();
    int8_t temp_high = OwReadByte();

    if (OwReset() == 0) {
        //printf("Nie je pripojeny senzor (OwReset() hlasi nenalezeno)\n");
        *res = INT32_MAX;
        return;
    }
    
    *res = (temp_high << 4) | ((temp_low & 0xf0) >> 4);
    *res *= 100;
    *res |= temp_low & 0x0f;
    
    if (*res == 8500)
 		*res = tmp;
 	tmp = *res;
    //printf("DS18b20Temp::%d\n",*res);
}
