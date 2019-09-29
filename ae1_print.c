#include <stdio.h>
#include <stdint.h>
#include "ae1.h"

void main(void)
{
    uint8_t buf[20+3];
    while (1)
    {
        for (int i = 0; i<sizeof(buf) - 1; i++)
        {
            buf[i] = buf[i+1];
        }
        if (read(0, buf + sizeof(buf) - 1, 1) <=0 )
        {
            exit(0);
        }
        //printf("%02x\n\n\n", buf[22]);
        if (buf[0] == 0x2d && buf[1] == 0xd4)
        {
            ae1_data * data = (ae1_data *)(buf + 3);
            printf("\n\n");
            printf("Timespamp => %li\n", data->timestamp);
            printf("Temperature BMP => %li\n", data->temperature_bmp);
            printf("Temperature SI => %li\n", data->temperature_si);
            printf("Temperature MCU => %li\n", data->temperature_mcu);
            printf("Temperature NTC => %li\n", data->temperature_ntc);
            printf("Supply voltage => %li\n", data->voltage_si);
            printf("Illumination => %li\n", data->illumination);
            printf("Pressure => %li\n", (int)data->pressure);
            

        }
        
    }
}