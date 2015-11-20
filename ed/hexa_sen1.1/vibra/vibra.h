/* 
 * File:   vibra.h
 * Author: Panda
 *
 * Created on Štvrtok, 2015, júl 9, 14:58
 */

#ifndef VIBRA_H
#define	VIBRA_H



#define SEC_IN_MIN      60
#define SHAKING_INTERVAL      2
#define INTERRUPTS_FOR_ONE_SHAKE  10


#define    HOURS_18 64800

#define    SA02_VIBRA_INTPIN_TRIS     TRISBbits.TRISB0
#define    SA02_VIBRA_INTPIN_EDGE     INTCON2bits.INTEDG0 
#define    SA02_VIBRA_INTPIN_IF       INTCONbits.INT0IF
#define    SA02_VIBRA_INTPIN_IE       INTCONbits.INT0IE

#define    SA02_VIBRA_MAX_SHAKE     15


void Vibra_Init(void);
void add_Count_shakes(void);
bool is_shakes(void);

//Funkcie navyse
void Vibra_ADCInit(void);
int Vibra_ADCRead(unsigned char ch);
void Vibra_show_location();




uint8_t BCDToDecimal(uint8_t bcdByte);
bool isInInterval (uint16_t start_sec, uint16_t current_sec);
void StartTimer(uint16_t seconds);

#endif	/* VIBRA_H */

