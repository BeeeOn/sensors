#ifndef SHT21LIB_H
#define	SHT21LIB_H

// sensor command
typedef enum{
 TRIG_T_MEASUREMENT_HM = 0xE3, // command trig. temp meas. hold master
 TRIG_RH_MEASUREMENT_HM = 0xE5, // command trig. humidity meas. hold master
 TRIG_T_MEASUREMENT_POLL = 0xF3, // command trig. temp meas. no hold master
 TRIG_RH_MEASUREMENT_POLL = 0xF5, // command trig. humidity meas. no hold master
 USER_REG_W = 0xE6, // command writing user register
 USER_REG_R = 0xE7, // command reading user register
 SOFT_RESET = 0xFE // command soft reset
}etSHT21Command;

typedef enum {
 SHT21_RES_12_14BIT = 0x00, // RH=12bit, T=14bit
 SHT21_RES_8_12BIT = 0x01, //  RH= 8bit, T=12bit
 SHT21_RES_10_13BIT = 0x80, // RH=10bit, T=13bit
 SHT21_RES_11_11BIT = 0x81, // RH=11bit, T=11bit
 SHT21_RES_MASK = 0x81 // Mask for res. bits (7,0) in user reg.
} etSHT21Resolution;

typedef enum{
 SHT_HUMIDITY = 0x00,
 SHT_TEMP = 0x01
}etSHT21MeasureType;

typedef enum{
 I2C_ADR_W = 128, // sensor I2C address + write bit
 I2C_ADR_R = 129 // sensor I2C address + read bit
}etI2cHeader;

typedef enum{
ACK_ERROR = 0x01,
TIME_OUT_ERROR = 0x02,
CHECKSUM_ERROR = 0x04,
UNIT_ERROR = 0x08
}etError;

void I2c_Init (void);

uint8_t SHT21_ReadUserRegister(uint8_t *pRegisterValue);
//==============================================================================
// reads the SHT21 user register (8bit)
// input : -
// output: *pRegisterValue
// return: error
//==============================================================================
uint8_t SHT21_WriteUserRegister(uint8_t *pRegisterValue);
//=====================================================
// writes the SHT21 user register (8bit)
// input : *pRegisterValue
// output: -
// return: error
//==============================================================================

uint8_t SHT21_MeasureHM(etSHT21MeasureType eSHT21MeasureType,  uint8_t *u8H, uint8_t *u8L);
//==============================================================================
// measures humidity or temperature. This function waits for a hold master until
// measurement is ready or a timeout occurred.
// input: eSHT21MeasureType
// output:  u8H, u8L - high and low part of 16-bit raw humidity / temperature value
// return: error
// note: timing for timeout may be changed
//==============================================================================
uint8_t SHT21_SoftReset();
//==============================================================================
// performs a reset
// input: -
// output: -
// return: error
//==============================================================================
uint16_t SHT21_CalcRH(uint8_t u8H, uint8_t u8L);
//==============================================================================
// calculates the relative humidity
// input:  u8H, u8L - high and low part of 16-bit raw humidity
// return: pHumidity relative humidity [%RH]
//==============================================================================
int32_t SHT21_CalcTemperatureC(uint8_t u8H, uint8_t u8L);
//==============================================================================
// calculates temperature
// input:  u8H, u8L - high and low part of 16-bit raw temperature
// return: temperature [°C]
//==============================================================================

void SHT21_get_data(int16_t *Humidity, int16_t *Temperature);




#endif