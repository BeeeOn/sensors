#include "include.h"

#define    SDA_DATA      PORTDbits.RD0 //hodnoty na pine
#define    SCL_DATA      PORTDbits.RD1  

#define    SDA_OUT       TRISDbits.TRISD0 //nastaveny pin ako OUT
#define    SCL_OUT       TRISDbits.TRISD1

 #define _XTAL_FREQ 8000000     


typedef enum{
    LOW = 0,
    HIGH = 1,
}etI2cLevel;

void I2c_Init()
//==============================================================================
{   
    SDA_OUT = 1; // I2C-bus idle mode SDA released (input)
    SCL_OUT = 1; // I2C-bus idle mode SCL released (input)
    
    
    SSP2ADD = 0x05;  // 333KHz
}

void sht_delay_ms(int ms)
//===========================================================================
{
	for(; ms > 0; ms--)
	{
		Delay100TCYx(20);
	}
}

uint8_t SHT21_ReadUserRegister(uint8_t *pRegisterValue)
//===========================================================================
{
    uint8_t checksum;  //variable for checksum byte
    uint8_t error=0;  //variable for error code
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_W);
    error |= WriteI2C2 (USER_REG_R);
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_R);
    *pRegisterValue = ReadI2C2();
    AckI2C2();
    checksum=ReadI2C2();
    NotAckI2C2();
    StopI2C2();
    return error;
}

uint8_t SHT21_WriteUserRegister(uint8_t *pRegisterValue)
//===========================================================================
{
    uint8_t error=0;  //variable for error code
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_W);
    error |= WriteI2C2 (USER_REG_W);
    error |= WriteI2C2 (*pRegisterValue);
    StopI2C2();
    return error;
}

uint8_t SHT21_MeasureHM(etSHT21MeasureType eSHT21MeasureType, uint8_t *u8H, uint8_t *u8L)
{
    uint8_t data[2]; //data array for checksum verification
    uint8_t error=0; //error variable
    //-- write I2C sensor address and command --
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_W); // I2C Adr
    switch(eSHT21MeasureType)
    {
        case SHT_HUMIDITY: error |= WriteI2C2 (TRIG_RH_MEASUREMENT_HM); break;
        case SHT_TEMP : error |= WriteI2C2 (TRIG_T_MEASUREMENT_HM);break;
    }
	//-- wait until hold master is released --
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_R);
	IdleI2C2();
    data[0] = ReadI2C2();
    AckI2C2();

    data[1] = ReadI2C2();
    NotAckI2C2();

    (*u8H) = data[0];
    (*u8L) = data[1];
    StopI2C2();
    return error;
}

uint8_t SHT21_SoftReset()
{
    uint8_t error=0; //error variable
    StartI2C2();
    error |= WriteI2C2 (I2C_ADR_W); // I2C Adr
    error |= WriteI2C2 (SOFT_RESET); // Command
    StopI2C2();
    ////  delay_ms(20); // wait till sensor has restarted
    return error;
}


uint16_t SHT21_CalcRH(uint8_t u8H, uint8_t u8L)
{
   uint16_t humidityRH=0x0000; // variable for result
   uint16_t res=0x0000; // variable for result
   uint16_t pom;

    u8L = u8L & 0xFC;
    // &= ~0x0003; // clear bits [1..0] (status bits)
    //-- calculate relative humidity [%RH] --
    pom = 128*u8H; // shift left 8 bits

    pom = pom + u8L; // get 16-bit number
    humidityRH = 0x0000 + pom; 

    res = humidityRH / 262;
    res = res - 6;
    res = res * 100;
    humidityRH = humidityRH % 262;
    humidityRH = humidityRH * 100;
    humidityRH = humidityRH / 262;
    res += humidityRH;
    return res;
    // RH= -6 + 125 * SRH/2^16
}


int32_t SHT21_CalcTemperatureC(uint8_t u8H, uint8_t u8L)
{
    uint16_t temp=0x0000; // variable for result
    int32_t res=0; // variable for result
    uint16_t pom=0x0000;

    u8L = u8L & 0xFC;
    // &= ~0x0003; // clear bits [1..0] (status bits)
    //-- calculate temperature --
    pom = (128*u8H);

    pom = pom + u8L;
    temp = pom;

    res = 0 + (temp / 187);

    res = res * 100;
    temp = temp % 187;
    temp = temp * 100;
    temp = temp / 187;
    
    res = res + temp;
    res = res - 4685;
    
    return res;
    //T= -46.85 + 175.72 * ST/2^16
}

void SHT21_get_data(int16_t *Temperature, int16_t *Humidity){
    OpenI2C2(MASTER, SLEW_OFF);
    int16_t error;
    uint8_t userRegister;  //variable for user register
    uint8_t u8H,u8L;

    error = 0;  // error status
    // ---Reset sensor by command ---
    error |= SHT21_SoftReset();  

    // ---Set Resolution e.g. RH 10bit, Temp 13bit ---
    error |= SHT21_ReadUserRegister(&userRegister);  //get actual user reg
    userRegister = (userRegister & ~SHT21_RES_MASK) | SHT21_RES_10_13BIT;
    error |= SHT21_WriteUserRegister(&userRegister); //write changed user reg
    // ---measure humidity with "Hold Master Mode (HM)"  ---
    error |= SHT21_MeasureHM(SHT_HUMIDITY, &u8H, &u8L);
    (*Humidity) = SHT21_CalcRH(u8H,u8L);
    
    // ---measure temperature with "Hold Master Mode (HM)"  ---
    error |= SHT21_MeasureHM(SHT_TEMP, &u8H, &u8L); //OK1
	(*Temperature) = SHT21_CalcTemperatureC(u8H,u8L);
    //return error;
    CloseI2C2();
}
