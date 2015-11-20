#include "include.h"

//#define MIWI_DEMO_KIT
#define HEXA_BOARD_V1

//velkost pouziteho miesta pre data
#define USED_SPACE  9  

#if defined(HEXA_BOARD_V1)
    #define MCHP_EEPROM         MCHP_4KBIT  // eeprom size

    #define EE_nCS              PORTAbits.RA2
    #define EE_nCS_TRIS         TRISAbits.TRISA2

    #define EESPIPut( x )       WriteSPI1( x )
    #define EESPIGet()          ReadSPI1()

#elif defined(MIWI_DEMO_KIT)
/* Pin remap required!
 * iPPSInput(IN_FN_PPS_SDI2, IN_PIN_PPS_RP23);
 * iPPSOutput(OUT_PIN_PPS_RP21, OUT_FN_PPS_SCK2);
 * iPPSOutput(OUT_PIN_PPS_RP19, OUT_FN_PPS_SDO2);
 */
    #define MCHP_EEPROM         MCHP_2KBIT  // eeprom size

    #define EE_nCS              LATDbits.LATD5
    #define EE_nCS_TRIS         TRISDbits.TRISD5

    #define EESPIPut( x )       WriteSPI2( x )
    #define EESPIGet()          ReadSPI2()


    #define SPI_SDI2            PORTDbits.RD6
    #define SDI2_TRIS           TRISDbits.TRISD6
    #define SPI_SDO2            LATDbits.LATD2
    #define SDO2_TRIS           TRISDbits.TRISD2
    #define SPI_SCK2            LATDbits.LATD4
    #define SCK2_TRIS           TRISDbits.TRISD4

#else
    #error No board defined
#endif



/*------ EEPROM size and pages definition -------*/
#define MCHP_1KBIT              1       
#define MCHP_2KBIT              2
#define MCHP_4KBIT              3
#define MCHP_8KBIT              4
#define MCHP_16KBIT             5
#define MCHP_32KBIT             6
#define MCHP_64KBIT             7
#define MCHP_128KBIT            8
#define MCHP_256KBIT            9
#define MCHP_512KBIT            10
#define MCHP_1MBIT              11

#if MCHP_EEPROM == 0
#error MCHP_EEPROM is not defined
#elif MCHP_EEPROM < MCHP_32KBIT
#define NVM_PAGE_SIZE   16
#elif MCHP_EEPROM < MCHP_128KBIT
#define NVM_PAGE_SIZE   32
#elif MCHP_EEPROM < MCHP_512KBIT
#define NVM_PAGE_SIZE   64
#elif MCHP_EEPROM < MCHP_1MBIT
#define NVM_PAGE_SIZE   128
#elif MCHP_EEPROM == MCHP_1MBIT
#error Microchip 1MBit EEPROM is not supported at this time. User need to modify the EEPROM access function to make it work. The address must be 3 bytes.
#else
#error Invalid MCHP EEPROM part
#endif

#if MCHP_EEPROM == MCHP_1KBIT
#define TOTAL_NVM_BYTES 128
#elif MCHP_EEPROM == MCHP_2KBIT
#define TOTAL_NVM_BYTES 256
#elif MCHP_EEPROM == MCHP_4KBIT
#define TOTAL_NVM_BYTES 512
#elif MCHP_EEPROM == MCHP_8KBIT
#define TOTAL_NVM_BYTES 1024
#elif MCHP_EEPROM == MCHP_16KBIT
#define TOTAL_NVM_BYTES 2048
#elif MCHP_EEPROM == MCHP_32KBIT
#define TOTAL_NVM_BYTES 4096
#elif MCHP_EEPROM == MCHP_64KBIT
#define TOTAL_NVM_BYTES 8192
#elif MCHP_EEPROM == MCHP_128KBIT
#define TOTAL_NVM_BYTES 16384
#elif MCHP_EEPROM == MCHP_256KBIT
#define TOTAL_NVM_BYTES 32768
#elif MCHP_EEPROM == MCHP_512KBIT
#define TOTAL_NVM_BYTES 65535
#elif MCHP_EEPROM == MCHP_1MBIT
#error Microchip 1MBit EEPROM is not supported at this time. User need to modify the EEPROM access function to make it work. The address must be 3 bytes.
#else
#error MCHP_EEPROM is not defined
#endif
/* END ------ EEPROM size and pages definition -------*/

/*
 * I/O and SPI bus configuration
 */
void spieepromInit(void) {
    EE_nCS = 1;
    EE_nCS_TRIS = 0;
        
#if defined(HEXA_BOARD_V1)
    
#elif defined(MIWI_DEMO_KIT)    
    SDI2_TRIS = 1;
    SDO2_TRIS = 0;
    SCK2_TRIS = 0;
    
    OpenSPI2(SPI_FOSC_4, MODE_00, SMPMID);
    
    SSP2STAT = 0x00;
    SSP2CON1 = 0x31;
    
    PIR3bits.SSP2IF = 0;    
#endif
    
}
void spieepromRead(uint8_t *dest, uint16_t addr, uint16_t count) {
    uint8_t oldGIEH = INTCONbits.GIE;
    INTCONbits.GIE = 0;

    EE_nCS = 0;

#if MCHP_EEPROM < MCHP_4KBIT
    EESPIPut(SPI_READ);
    EESPIPut(addr);
#elif MCHP_EEPROM == MCHP_4KBIT
    if (addr > 0xFF) {
        EESPIPut(SPI_READ | 0x08);
    } else {
        EESPIPut(SPI_READ);
    }
    EESPIPut(addr);
#elif MCHP_EEPROM < MCHP_1MBIT
    EESPIPut(SPI_READ);
    EESPIPut(addr >> 8);
    EESPIPut(addr);
#endif

    while (count > 0) {
        *dest++ = EESPIGet();
        count--;
    }

    EE_nCS = 1;

    INTCONbits.GIE = oldGIEH;

}

void spieepromWrite(uint8_t *source, uint16_t addr, uint16_t count) {
    uint8_t statusReg = 0;
    uint8_t oldGIE = INTCONbits.GIE;
    INTCONbits.GIE = 0;

EEPROM_NEXT_PAGE:
    do {
        EE_nCS = 0;
        EESPIPut(SPI_RDSR);
        statusReg = EESPIGet();
        EE_nCS = 1;
        Nop();
    } while (statusReg & 0x01);

    EE_nCS = 0;
    EESPIPut(SPI_WREN);
    EE_nCS = 1;
    Nop();
    EE_nCS = 0;
#if MCHP_EEPROM < MCHP_4KBIT
    EESPIPut(SPI_WRITE);
    EESPIPut(addr);
#elif MCHP_EEPROM == MCHP_4KBIT
    if (addr > 0xFF) {
        EESPIPut(SPI_WRITE | 0x08);
    } else {
        EESPIPut(SPI_WRITE);
    }
    EESPIPut(addr);
#elif MCHP_EEPROM < MCHP_1MBIT
    EESPIPut(SPI_WRITE);
    EESPIPut(addr >> 8);
    EESPIPut(addr);
#endif
    statusReg = 0;
    while (count > 0) {
        EESPIPut(*source++);
        count--;
        statusReg++;
        if (((addr + statusReg) & (NVM_PAGE_SIZE - 1)) == 0) {
            EE_nCS = 1;
            addr += statusReg;
            goto EEPROM_NEXT_PAGE;
        }
    }
    EE_nCS = 1;
    Nop();    
    // waiting for write finish
    do {        
        EE_nCS = 0;
        EESPIPut(SPI_RDSR);
        statusReg = EESPIGet();
        EE_nCS = 1;
        Nop();
    
    } while (statusReg & 0x01);

    INTCONbits.GIE = oldGIE;
}

void spieepromClean() {
    uint8_t edid[USED_SPACE];
    for (uint8_t i = 0; i < USED_SPACE; i++)
        edid[i] = 0;
    spieepromWrite(edid, 0, USED_SPACE);
}


