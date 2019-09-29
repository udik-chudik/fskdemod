#include <stdio.h>
#include <stdint.h>
#include "ae1.h"

void main(void)
{
    uint8_t buf[20];
    while (1)
    {
        if (read(0, &buf, sizeof(buf)) <=0 )
        {
            exit(0);
        }
        ae1_data * data = (ae1_data *)buf;
        printf("Timespamp => %li\n", data->timestamp);
    }
}