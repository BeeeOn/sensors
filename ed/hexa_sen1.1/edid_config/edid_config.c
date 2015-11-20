#include "edid_config.h"
#include "debug.h"

// nacita EDID z eeprom a ulozi do GLOBAL_STORAGE
void euid_load_eeprom() {
    char eepromData[EUID_ADRESS_LEN];
    spieepromRead(eepromData, EUID_ADRESS, EUID_ADRESS_LEN);
   
    for (uint8_t k = 0; k < EUID_ADRESS_LEN; k++)
         GLOBAL_STORAGE.edid[k] = eepromData[k];
}

// vypise udaje z eeprom
void get_data_eeprom() {
    char eepromData[9];
    spieepromRead(eepromData, 0, NID_ADRESS_LEN+EUID_ADRESS_LEN);

    for (uint8_t k = 0; k < NID_ADRESS_LEN+EUID_ADRESS_LEN; k ++) {
        D_G printf("%d, ", eepromData[k]);
    }
}

// start stringom da programu do pc vediet ze caka na nacitanie EDID
// ak sa v programe oznaci, ze sa ma EDID zapisat, tak sa z pc posle char 'w'
// ktory indikuje, ze sa ma zapisat EDID do EEPROM
void euid_load() {
    //zakazanie vsetkych preruseni
    INTCONbits.GIE = 0;     

    D_G printf("\nOld EEPROM Data::");
    get_data_eeprom();
    
    printf("startedid");//start string 
    
    uint16_t i = 0;
    while(i < 200) {
        char rx;
        if ( DataRdy2USART() && (rx = Read2USART()) == 'w') {
                uint8_t edid[4];
                for (uint8_t i = 0; i < 4; i++) {
                    while(!DataRdy2USART());
                    edid[i] = Read2USART();
                }
                D_G printf("=====Write eeprom data.======");
                spieepromWrite(edid, 5, 4);
                break;      
        }
        i++;
        delay_ms(10);
    }
    printf("stopedid"); //end string
    
    D_G printf("\nNew EEPROM Data::");
    get_data_eeprom();
    INTCONbits.GIE = 1;  
    
    euid_load_eeprom();//Nacitanie EUID do z EEPROM do GLOBAL_STORAGE
}