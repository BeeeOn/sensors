#ifndef SPIEEPROM_H
#define SPIEEPROM_H

void spieepromInit(void);
void spieepromRead(uint8_t *dest, uint16_t addr, uint16_t count);
void spieepromWrite(uint8_t *source, uint16_t addr, uint16_t count);

#endif
