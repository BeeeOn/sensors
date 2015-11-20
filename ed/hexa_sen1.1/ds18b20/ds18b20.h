#ifndef __DS18S20_H
#define __DS18S20_H

void DsDeInit(void);
void ds_prepare(void);
void ds_get_temp(int32_t *res);

#endif