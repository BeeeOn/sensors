#ifndef HW_LAYER_H
#define HW_LAYER_H

//pre xc8
#ifndef X86

#define _XTAL_FREQ 8000000

#define LED0                PORTDbits.RD7
#define LED0_TRIS           TRISDbits.TRISD7
#define LED1                PORTDbits.RD6
#define LED1_TRIS           TRISDbits.TRISD6

#define IRQ1_INT_PIN        PORTCbits.RC6
#define IRQ1_INT_TRIS       TRISCbits.TRISC6
#define PHY_IRQ1            INTCON3bits.INT1IF
#define PHY_IRQ1_En         INTCON3bits.INT1IE

void delay_ms(uint16_t t);
void goSleep(void);
void HW_init(void);
void HW_ReInit(void);
void HW_DeInit(void);
uint16_t GetBatteryStatus(uint16_t *value);

//pre x86
#else
void delay_ms(uint16_t t);
void goSleep(void);
  void HW_init(void);
void HW_ReInit(void);
void HW_DeInit(void);
uint16_t GetBatteryStatus(uint16_t *value);
#endif

 


#endif

