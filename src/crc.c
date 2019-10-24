#include "fskdemod.h"


uint16_t crc16(unsigned char* pData, int length)
{
    /*
        CRC-16/BUYPASS (CRC16 IBM)
    */
    uint8_t i;
    uint16_t wCrc = 0x0000;
    while (length--) {
        wCrc ^= *(unsigned char *)pData++ << 8;
        for (i=0; i < 8; i++)
            wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ 0x8005 : wCrc << 1;
    }
    return wCrc & 0xffff;
}