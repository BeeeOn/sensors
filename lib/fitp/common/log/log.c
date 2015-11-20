#include "include.h"

#define USE_AND_MASKS

#define OUT 0
#define IN 1

#define RX_PIN           LATDbits.LATD5
#define TX_PIN           LATDbits.LATD4
#define RX_PIN_TRIS      TRISDbits.TRISD5
#define TX_PIN_TRIS      TRISDbits.TRISD4

  void setPortDirection(){
    TX_PIN_TRIS = OUT;
    RX_PIN_TRIS = IN;
}

  void LOG_init(){
  
  //remapPins();
  setPortDirection();
  
  Open2USART(
     USART_TX_INT_OFF &
     USART_RX_INT_OFF & 
     USART_ASYNCH_MODE &
     USART_EIGHT_BIT & 
     USART_CONT_RX &
     USART_BRGH_HIGH, 8);
  
   while(Busy2USART());
   printf("\n\n\nLoging successfuly configured\n*****************************\n");
}


// printing single char by printf() function
void putch(unsigned char c) {
    putc2USART(c);
    while(Busy2USART());
}