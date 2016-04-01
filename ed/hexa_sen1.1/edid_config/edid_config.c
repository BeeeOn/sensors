#include "edid_config.h"
#include "debug.h"

// nacita EDID z eeprom a ulozi do GLOBAL_STORAGE
void euid_load_eeprom() {
    char eepromData[EUID_ADRESS_LEN];
    spieepromRead(eepromData, EUID_ADRESS, EUID_ADRESS_LEN);

    for (uint8_t k = 0; k < EUID_ADRESS_LEN; k++)
         GLOBAL_STORAGE.edid[k] = eepromData[k];
}

//load refresh time
void refresh_load_eeprom() {
    uint8_t time[2];
    spieepromRead(time, 9, 2);

   	//convert to 16bit number from 2x8bit
   	GLOBAL_STORAGE.refresh_time = (time[0] << 8) | time[1];
}

// save refresh time
void save_refresh_eeprom(uint16_t val) {
	uint8_t time[2];
	time[0] = val >> 8;
	time[1] = val & 255;
	spieepromWrite(time, 9, 2); //write refresh time
}

// vypise udaje z eeprom
void get_data_eeprom() {
    char eepromData[11];
    spieepromRead(eepromData, 0, 11);

    for (uint8_t k = 0; k < 11; k ++) {
        printf("%d, ", eepromData[k]);
    }

}

// start stringom da programu do pc vediet ze caka na nacitanie EDID
// ak sa v programe oznaci, ze sa ma EDID zapisat, tak sa z pc posle char 'w'
// ktory indikuje, ze sa ma zapisat EDID do EEPROM
void euid_load() {
    //zakazanie vsetkych preruseni
    INTCONbits.GIE = 0;

	//Old data
    printf("\nOD:");
    get_data_eeprom();

    printf("startedid");//start string

    uint16_t i = 0;
    while(i < 200) {
        char rx;
        if ( DataRdy2USART() && (rx = Read2USART()) == 'w') {
        		//clean EEPROM
        		spieepromClean();

                uint8_t edid[4];

                for (uint8_t i = 0; i < 4; i++) {
                    while(!DataRdy2USART());
                    edid[i] = Read2USART();
                }
                //Write data
                printf("WD:");
                spieepromWrite(edid, 5, 4);
                save_refresh_eeprom(300); //write refresh time, default 300s - 5min
                break;
        }
        i++;
        delay_ms(10);
    }
    printf("stopedid"); //end string

    //New data
    printf("\nND:");
    get_data_eeprom();
    INTCONbits.GIE = 1;

    euid_load_eeprom();//Nacitanie EUID do z EEPROM do GLOBAL_STORAGE
}
