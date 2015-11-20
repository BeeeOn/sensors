#ifndef MIMAC_CONSTANTS_H
#define MIMAC_CONSTANTS_H

#define RF_SLEEP        0x00
#define RF_STANDBY      0x20
#define RF_SYNTHESIZER  0x40
#define RF_RECEIVER     0x60
#define RF_TRANSMITTER  0x80

#define CHIPMODE_STBYMODE RF_STANDBY

//register description
#define GCONREG     0x00        // General Configuration Register
#define DMODREG     0x02        // Data and Modulation Configuration Register
#define FDEVREG     0x04        // Frequency Deviation Control Register
#define BRREG       0x06          // Bit Rate Set Register
#define FLTHREG     0x08        // Floor Threshold Control Register
#define FIFOCREG    0x0A        // FIFO Configuration Register
#define R1CNTREG    0x0C        // R1 Counter Set Register
#define P1CNTREG    0x0E        // P1 Counter Set Register
#define S1CNTREG    0x10        // S1 Counter Set Register
#define R2CNTREG    0x12        // R2 Counter Set Register        
#define P2CNTREG    0x14        // P2 Counter Set Register
#define S2CNTREG    0x16        // S2 Counter Set Register
#define PACONREG    0x18        // Power Amplifier Control Register
#define FTXRXIREG   0x1A
#define FTPRIREG    0x1C
#define RSTHIREG    0x1E
#define FILCONREG   0x20
#define PFILCREG    0x22
#define SYNCREG     0x24
#define RESVREG     0x26
#define RSTSREG     0x28
#define OOKCREG     0x2A
#define SYNCV31REG  0x2C
#define SYNCV23REG  0x2E
#define SYNCV15REG  0x30
#define SYNCV07REG  0x32
#define TXPARAMREG  0x34         // TXCONREG
#define CLKOUTREG   0x36
#define PLOADREG    0x38
#define NADDREG     0x3A
#define PCONREG     0x3C
#define FCRCREG     0x3E



//TODO make band shits
#define LNA_GAIN 0x00
#define FREQ_BAND 0x10
#define VCO_TRIM_11 0x06
#define FILCON_SET 0x70
#define BANDWIDTH 0x02
#define TX_POWER  TX_POWER_1_DB


//default register settings
#define GCONREG_SET     (CHIPMODE_STBYMODE | FREQ_BAND | VCO_TRIM_11)
#define DMODREG_SET     (0x84 | LNA_GAIN)  // 0b10000100 DMODE1:DMODE0=10 => Packet mode
#define FLTHREG_SET     (0x0C)
#define FIFOCREG_SET    (0xC1)                                                          //FIFO size = 64 bytes and threshold limit for IRQ is 1
#define PACONREG_SET    (0x38)
#define FTXRXIREG_SET   (0xC8)  // 0b11001000, IRQ0 source = SYNC or Address match, IRQ1 source = CROK
#define FTPRIREG_SET    (0x0D)  // 0b00001101
#define RSTHIREG_SET    (0x00)  
#define FILCONREG_SET   (FILCON_SET | BANDWIDTH)
#define PFILCREG_SET    (0x38)
#define SYNCREG_SET     (0x38)
#define RESVREG_SET     (0x07)
#define SYNCV31REG_SET  (0x69)
#define SYNCV23REG_SET  (0x81)
#define SYNCV15REG_SET  (0x7E)
#define SYNCV07REG_SET  (0x96)
#define TXPARAMREG_SET  (0xF0 | (TX_POWER<<1))
#define CLKOUTREG_SET   (0x88)
#define PLOADREG_SET    (0x40)
#define NADDREG_SET     (0x00)
#define PCONREG_SET     (0xE8)
#define FCRCREG_SET     (0x00)

//power values
#define TX_POWER_13_DB      0x00    //[3:1], 13dBm
#define TX_POWER_10_DB      0x01        //10dBm
#define TX_POWER_7_DB       0x02        //7dBm
#define TX_POWER_4_DB       0x03        //4dBm
#define TX_POWER_1_DB       0x04        //1dBm
#define TX_POWER_N_2_DB     0x05        //-2dBm
#define TX_POWER_N_5_DB     0x06        //-5dBm
#define TX_POWER_N_8_DB     0x07        //-8dBm

//bitrate values
#define DATA_RATE_5         0x00
#define DATA_RATE_10        0x01
#define DATA_RATE_20        0x02
#define DATA_RATE_40        0x03
#define DATA_RATE_50        0x04
#define DATA_RATE_66        0x05
#define DATA_RATE_100       0x06
#define DATA_RATE_200       0x07

#define BITRATE_200     0x00
#define BITRATE_100     0x01
#define BITRATE_66      0x02
#define BITRATE_50      0x03
#define BITRATE_40      0x04
#define BITRATE_25      0x07
#define BITRATE_20      0x09
#define BITRATE_10      0x13
#define BITRATE_5       0x27
#define BITRATE_2       0x63


#define BW_25  0x00
#define BW_50  0x01
#define BW_75  0x02
#define BW_100 0x03
#define BW_125 0x04
#define BW_150 0x05
#define BW_175 0x06
#define BW_200 0x07
#define BW_225 0x08
#define BW_250 0x09
#define BW_275 0x0A
#define BW_300 0x0B
#define BW_325 0x0C
#define BW_350 0x0D
#define BW_375 0x0E
#define BW_400 0x0F

#define FREQ_DEV_33  0x0B
#define FREQ_DEV_40  0x09
#define FREQ_DEV_50  0x07
#define FREQ_DEV_67  0x05
#define FREQ_DEV_80  0x04
#define FREQ_DEV_100 0x03
#define FREQ_DEV_133 0x02
#define FREQ_DEV_200 0x01

#define FILCON_SET_65   0x00		//65 KHz
#define FILCON_SET_82   0x10		//82 KHz
#define FILCON_SET_109  0x20
#define FILCON_SET_137  0x30
#define FILCON_SET_157  0x40
#define FILCON_SET_184  0x50
#define FILCON_SET_211  0x60
#define FILCON_SET_234  0x70
#define FILCON_SET_262  0x80
#define FILCON_SET_321  0x90
#define FILCON_SET_378  0xA0
#define FILCON_SET_414  0xB0
#define FILCON_SET_458  0xC0
#define FILCON_SET_514  0xD0
#define FILCON_SET_676  0xE0
#define FILCON_SET_987  0xF0

//TODO verify me
#define CHANNEL_ASSESSMENT_CARRIER_SENSE  0x00
#define CHANNEL_ASSESSMENT_ENERGY_DETECT 0x01


const uint8_t InitConfigRegs[] = {
    /* 0 */ GCONREG_SET,
    /* 1 */ DMODREG_SET,
    /* 2 */ //FREQ_DEV,
    0x09,
    /* 3 */ //DATARATE,
    0x09,
    /* 4 */ FLTHREG_SET,
    /* 5 */ FIFOCREG_SET,
    /* 6 */ //R1CNT,
    125,
    /* 7 */ //P1CNT,
    100,
    /* 8 */ //S1CNT,
    20,
    /* 9 */ 0x00,
    /* 10 */ 0x00,
    /* 11 */ 0x00,
    /* 12 */ PACONREG_SET,
    /* 13 */ FTXRXIREG_SET,
    /* 14 */ FTPRIREG_SET,
    /* 15 */ RSTHIREG_SET,
    /* 16 */ FILCONREG_SET,
    /* 17 */ PFILCREG_SET,
    /* 18 */ SYNCREG_SET,
    /* 19 */ RESVREG_SET,
    /* 20 */ 0x00,
    /* 21 */ 0x00,
    /* 22 */ SYNCV31REG_SET, // 1st byte of Sync word,
    /* 23 */ SYNCV23REG_SET, // 2nd byte of Sync word,
    /* 24 */ SYNCV15REG_SET, // 3rd byte of Sync word,
    /* 25 */ SYNCV07REG_SET, // 4th byte of Sync word,
    /* 26 */ TXPARAMREG_SET,
    /* 27 */ CLKOUTREG_SET,
    /* 28 */ PLOADREG_SET,
    /* 29 */ NADDREG_SET,
    /* 30 */ PCONREG_SET,
    /* 31 */ FCRCREG_SET
};


#endif 
