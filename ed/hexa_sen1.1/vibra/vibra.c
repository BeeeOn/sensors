#include "include.h"

uint8_t count_interrupt = 0; //pocet interruptov pri traseni 
extern bool accel_int;

void Vibra_Init(void) {
    
   // Config INT0 Edge = Falling
    SA02_VIBRA_INTPIN_TRIS = 1;
    SA02_VIBRA_INTPIN_EDGE = 0;
    SA02_VIBRA_INTPIN_IF = 0;
    SA02_VIBRA_INTPIN_IE = 1;  // enable interrupt
}

void add_Count_shakes(void) {
    printf("IO: %d --\n", count_interrupt);
    count_interrupt++;
}

bool is_shakes(void) {
    if (count_interrupt >= SA02_VIBRA_MAX_SHAKE) {
        count_interrupt = 0;
        printf("counter vynulovany \n");
        return true;
    }
    else 
        return false;
}

//
//Funkcie navyse
//
void Vibra_ADCInit()
{
    // Konfiguracia kanalu ako VBG
	 ADCON1=0b10001010;               
}

//Funkcia precita zo zadaneho kanalu data
int Vibra_ADCRead(unsigned char ch)
{
    //nespravny kanal 
    if(ch>13) 
        return 0;  
   
    ADCON0=0x00;
    
    //vyberie sa ADC kanal 
    ADCON0=(ch<<2);

    //Prepnutie do ADC modu 
    ADON=1;
    
    //zaciatok konvezie a caka sa pokial sa neskonci 
    GODONE=1;  
    while(GODONE); 

    //vypnutie ADC modu 
    ADON=0;

    return ADRES;
}

void Vibra_show_location() {
    Vibra_Init();
    while(1) {
        printf("%d\n", Vibra_ADCRead(12));
        delay_ms(100);
    }
}

extern uint8_t timeout;

int abs(int a)
{
	if(a < 0)
		return -a;
	return a;
}

uint8_t BCDToDecimal (uint8_t bcdByte)
{
  return (((bcdByte & 0xF0) >> 4) * 10) + (bcdByte & 0x0F);
}

bool isInInterval (uint16_t start_sec, uint16_t current_sec)
{
    if (abs(current_sec - start_sec) < SHAKING_INTERVAL) 
    {
        return TRUE;
    }
    else
        return FALSE;
}


void StartTimer(uint16_t seconds)
{
    BYTE sec, min, hour;
    PIE3bits.RTCCIE = 0;
    // max time is 18 hours
    if (seconds > HOURS_18)
        seconds = HOURS_18;

    // get seconds, minutes, hours
    sec = seconds % 60;
    min = (seconds/60) % 60;
    hour = (seconds/(60*60)) % 24;

    // decimal to BCD
    sec = (sec/10)*16 + sec%10;
    min = (min/10)*16 + min%10;
    hour = (hour/10)*16 + hour%10;

    RtccWrOn();
    mRtccOff();

    // set start time to "zero" (begining of RTC)
    RTCCFGbits.RTCPTR1 = 1;
    RTCCFGbits.RTCPTR0 = 1;
    RTCVALL = 0;
    RTCVALH = 0;

    RTCVALL = 0;
    RTCVALH = 0;

    RTCVALL = 0;
    RTCVALH = 0;

    RTCVALL = 0;
    RTCVALH = 0;

    mRtccAlrmDisable();
    // set alarm
    ALRMCFGbits.ALRMPTR0 = 1;
    ALRMCFGbits.ALRMPTR1 = 1;
    ALRMVALL = 0;
    ALRMVALH = 0;

    ALRMVALL = 0;
    ALRMVALH = 0;

    ALRMVALL = hour;
    ALRMVALH = 0;
    ALRMVALL = sec;
    ALRMVALH = min;

    // set mask for alarm
    if (seconds == 1)
        RtccSetAlarmRpt(RTCC_RPT_SEC, FALSE);
    else if (seconds < 10)
        RtccSetAlarmRpt(RTCC_RPT_TEN_SEC, FALSE);
    else if (seconds < 60)
        RtccSetAlarmRpt(RTCC_RPT_MIN, FALSE);
    else if (seconds < 600)
        RtccSetAlarmRpt(RTCC_RPT_TEN_MIN, FALSE);
    else if (seconds < 3600)
        RtccSetAlarmRpt(RTCC_RPT_HOUR, FALSE);
    else
        RtccSetAlarmRpt(RTCC_RPT_DAY, FALSE);

    mRtccAlrmEnable();
    PIR3bits.RTCCIF = 0;
    PIE3bits.RTCCIE = 1;

    mRtccOn();
    mRtccWrOff();


}