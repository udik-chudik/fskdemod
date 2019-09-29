#include <stdint.h>
#ifndef AE1
#define AE1
#pragma pack(1)
typedef struct __AE1_DATA
{
    uint32_t timestamp;
    int32_t temperature_bmp;
    uint32_t pressure;
    uint8_t temperature_si;
    uint8_t voltage_si;
    uint16_t illumination;
    uint16_t temperature_ntc;
    uint16_t temperature_mcu;
} ae1_data;

#endif