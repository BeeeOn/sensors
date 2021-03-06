#include "include.h"

#define SPI_SDI             PORTBbits.RB5
#define SDI_TRIS            TRISBbits.TRISB5
#define SPI_SDO             LATCbits.LATC7
#define SDO_TRIS            TRISCbits.TRISC7
#define SPI_SCK             LATBbits.LATB4
#define SCK_TRIS            TRISBbits.TRISB4
 
#define Config_nCS          LATDbits.LATD1
#define Config_nCS_TRIS     TRISDbits.TRISD1
#define Data_nCS            LATAbits.LATA5
#define Data_nCS_TRIS       TRISAbits.TRISA5

#define IRQ0_INT_PIN        PORTBbits.RB1
#define IRQ0_INT_TRIS       TRISBbits.TRISB1
#define PHY_IRQ0            INTCONbits.INT0IF
#define PHY_IRQ0_En         INTCONbits.INT0IE

#define IRQ1_INT_PIN        PORTCbits.RC6
#define IRQ1_INT_TRIS       TRISCbits.TRISC6
#define PHY_IRQ1            INTCON3bits.INT1IF
#define PHY_IRQ1_En         INTCON3bits.INT1IE

#define PHY_RESETn          LATDbits.LATD0
#define PHY_RESETn_TRIS     TRISDbits.TRISD0

#define LED0                PORTCbits.RC2
#define LED0_TRIS           TRISCbits.TRISC2
#define LED1                PORTAbits.RA6
#define LED1_TRIS           TRISAbits.TRISA6

#define _XTAL_FREQ 8000000

void delay_ms(uint16_t maximum) {
       for(int i = 0; i < maximum; i++) {
           __delay_ms(1);
       }
    }

uint8_t spi_xchange(const uint8_t data){
  uint8_t i;

  PIR1bits.SSP1IF = 0;
  //clear SSP1BUF
    i = SSP1BUF;

  do
  {
      SSP1CON1bits.WCOL = 0;
      SSP1BUF = data;
  } while( SSP1CON1bits.WCOL );

  while( PIR1bits.SSP1IF == 0 );

  PIR1bits.SSP1IF = 0;

  return SSP1BUF;
}


  void HW_spiPut(const uint8_t data){
  spi_xchange(data);
}


  uint8_t HW_spiGet(void){
  uint8_t get;
  get = spi_xchange(0x00);
  return get;  
}

/*
 * pins for chip select onmrf interface 
 */

  void HW_disableConfig(void){
  Config_nCS = 1; 
}

  void HW_enableConfig(void){
  Config_nCS = 0; 
}

  void HW_disableData(void){
  Data_nCS = 1;
}

  void HW_enableData(void){
  Data_nCS = 0;
}


/* interupts mrf interface */

  bool HW_isIRQ0Enabled(void){
  return PHY_IRQ0_En;
}

  bool HW_isIRQ0Active(void){
  return PHY_IRQ0_En && PHY_IRQ0;
}

  bool HW_isIRQ0Clear(void){
  return PHY_IRQ0 == 0;
}

  void HW_clearIRQ0(void){
  PHY_IRQ0 = 0;
}

  void HW_disableIRQ0(void){
  PHY_IRQ0_En = 0;
}
  void HW_enableIRQ0(void){
  PHY_IRQ0_En = 1;
}




  bool HW_isIRQ1Enabled(void){
  return PHY_IRQ1_En;
}

  bool HW_isIRQ1Active(void){
  return PHY_IRQ1_En && PHY_IRQ1;
}

  bool HW_isIRQ1Clear(void){
  return PHY_IRQ1 == 0;
}

  void HW_clearIRQ1(void){
  PHY_IRQ1 = 0;
}

  void HW_disableIRQ1(void){
  PHY_IRQ1_En = 0;
}
  void HW_enableIRQ1(void){
  PHY_IRQ1_En = 1;
}

void remapAllPins() {
    PPSUnLock();

    /* pins used by mimac to comunicate radio */
    iPPSInput(IN_FN_PPS_INT1, IN_PIN_PPS_RP17);
    // mapovani tlacitka SW1
    iPPSInput(IN_FN_PPS_INT2, IN_PIN_PPS_RP4);
    /* pins used by usart loging */
    iPPSInput(IN_FN_PPS_RX2DT2, IN_PIN_PPS_RP23); // Mapping USART2 RX
    iPPSOutput(OUT_PIN_PPS_RP21, OUT_FN_PPS_TX2CK2); // Mapping USART2 TX


    // Lock System
    PPSLock();
}

/*void delay_my(uint16_t maximum) {
       for(int i = 0; i < maximum; i++) {
           __delay_ms(10);
       }
    }*/

/* 
 * init hardware required by mrf
 */
   void HW_init(void){
   
   remapAllPins();
  
    ANCON0 = 0xFF;
    ANCON1 = 0x1F;

    
    IRQ0_INT_TRIS = 1;
    IRQ1_INT_TRIS = 1;
    
    // Config IRQ Edge = Rising
    INTCON2bits.INTEDG0 = 1;
    INTCON2bits.INTEDG1 = 1;
      
    // vstup (tlacitko))
    TRISBbits.TRISB1 = 1;
    
    PHY_IRQ0 = 0; // MRF89XA
    PHY_IRQ0_En = 1; // MRF89XA
    PHY_IRQ1 = 0; // MRF89XA
    PHY_IRQ1_En = 1; // MRF89XA    
    
    Data_nCS = 1;
    Config_nCS = 1;

    Data_nCS_TRIS = 0;
    Config_nCS_TRIS = 0;

    
    INTCON2bits.RBPU = 0;
  
    SDI_TRIS = 1;
    SDO_TRIS = 0;
    SCK_TRIS = 0;
    
    SSP1STAT = 0xC0;
    SSP1CON1 = 0x21;
    
    //PHY_RESETn = 0;
    //PHY_RESETn_TRIS = 0;
    
     // reset radio
    PHY_RESETn = 1;
    PHY_RESETn_TRIS = 0;
    delay_ms(10);
    PHY_RESETn = 0;
    delay_ms(100);

    LED0_TRIS = 0;
    LED1_TRIS = 0;
    LED1 = 0;


    // casovac, ktery hlida obsah bufferu
    //documentation page 187
    // enabled timer,as 16 bit,internal instruction cycle, edge(x), not bypass prescaler, prescale 256
    T0CON=0b10010111;
    TMR0H=200;                  //wait more while configuration done
    //INTCONbits.TMR0IE=1;
    INTCONbits.TMR0IE=1;
    INTCONbits.TMR0IF=0;
    INTCON2bits.TMR0IP=0;       //use low priority on timer0 interupts 
    
    RCONbits.IPEN = 1; //enable interupt priority levels
    
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;
        
};


  void sniff_toJson(uint8_t is_rx, uint8_t len,const char* data){
  char* m_type;
  char str[]="data\0commit\0ack\0commit_ack";
  
  switch (data[0] >> 6){
    case 0: m_type= str;break;
    case 1: m_type= str+5;break;
    case 2: m_type= str+12;break;
    case 3: m_type= str+16;break;
  }
  
  
  printf("{\"direction\":\"%cx\",\"len\": %u,\"link\":{\"message_type\":\"%s\",",(is_rx?'r':'t'),len, m_type);
  
  printf("\"nid\":");
  printf("[\"0x%02X\"",data[1]);
  printf(",\"0x%02X\"",data[2]);
  printf(",\"0x%02X\"",data[3]);
  printf(",\"0x%02X\"],",data[4]);
  
  printf("\"to\":{");
  if ( data[0] & 0x20){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[5]);
    printf(",\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"]",data[8]);
  }else {
    printf("\"type\":\"cid\"");
    printf(",\"address\":\"0x%02X\"",data[5]);
  }
  printf("},");
  
  printf("\"from\":{");
  if ( data[0] & 0x10){
    printf("\"type\":\"edid\"");
    printf(",\"address\":");
    printf("[\"0x%02X\"",data[6]);
    printf(",\"0x%02X\"",data[7]);
    printf(",\"0x%02X\"",data[8]);
    printf(",\"0x%02X\"]",data[9]);
  }else{
    printf("\"type\":\"cid\"");
    if ( data[0] & 0x20){
      printf(",\"address\":\"0x%02X\"",data[9]);
    }else{
      printf(",\"address\":\"0x%02X\"",data[6]);
      printf(",\"seq num\":\"0x%02X\"",data[7]);
    }
  }
  printf("}}}\n");
  
}

/*
 * Methods needed if mrf in snifferMode
 * empty function == no sniffing
 */
  void HW_sniffRX(uint8_t len,const char* data){
  //sniff_toJson(1,len,data);
}

  void HW_sniffTX(uint8_t len,const char* data){
  sniff_toJson(0,len,data);
}



/* methods need be called when interupt ocure*/
extern   void HW_irq0occurred(void);
extern   void HW_irq1occurred(void);
extern   void HW_timeoccurred(void);
extern bool join;

#if defined(__XC8)
void interrupt low_priority low_isr(void)
#elif defined(__18CXX)
#pragma interruptlow HighISR
void HighISR(void)
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24FK__) || defined(__PIC24H__)
void _ISRFAST __attribute__((interrupt, auto_psv)) _INT1Interrupt(void)
#elif defined(__PIC32MX__)
void __ISR(_EXTERNAL_1_VECTOR, ipl4) _INT1Interrupt(void)
#else

void _ISRFAST _INT1Interrupt(void)
#endif
{  
  //printf("low priority interupt\n");
  if(INTCONbits.TMR0IE && INTCONbits.TMR0IF){
    //printf("timer interupt\n");
    HW_timeoccurred();
    TMR0H=240; //dont reorder h value is buffered, both value are writen in same time, whene change change value in HW_init()
    TMR0L=0;  //higher value shorter time, wait for (255-TMR0H)*255-TMR0L
    INTCONbits.TMR0IF=0;
  }
  PHY_IRQ1 = 0;
  PHY_IRQ0 = 0;
}



#if defined(__XC8)
    void interrupt high_isr(void)
#elif defined(__18CXX)
    #pragma interruptlow HighISR
    void HighISR(void)
#elif defined(__dsPIC30F__) || defined(__dsPIC33F__) || defined(__PIC24F__) || defined(__PIC24H__)
    void _ISRFAST __attribute__((interrupt, auto_psv)) _INT2Interrupt(void)
#elif defined(__PICC__)
    #pragma interrupt_level 0
    void interrupt HighISR(void)
#elif defined(__PIC32MX__)
    void __ISR(_EXTERNAL_2_VECTOR, ipl4) _INT2Interrupt(void)
#else
    void _ISRFAST _INT2Interrupt(void)
#endif
{
  LED0 = ~LED0;
  //printf("V PRERUSENI!\n");
  //printf("%d\n", PHY_IRQ1);
  //printf("PHY_IRQ1_En: %d\n", PHY_IRQ1_En);

  if( HW_isIRQ0Active()){
    HW_irq0occurred();
    HW_clearIRQ0();    
  } 
  
  if( HW_isIRQ1Active() ){
    HW_irq1occurred();
    HW_clearIRQ1();
  }

  LED0 = ~LED0;
}
