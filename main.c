#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <liquid/liquid.h>
/*
rtl_fm -p 75 -g 49 -f 437000000 -s 20000 - | sox -t raw -r 20000 -es -b 16 -c 1 - -t raw -r 44000 - sinc 0-2000 compand 0.01,0.01 -100,0 -20 | ~/development/fskdemod/a.out
*/
long unsigned int samples_counter = 0;
int SAMPLES_PER_SYMBOL;
int INT_BUFFER_LENGTH;
windowf intbuf;

float readSample()
{
    /*
        Read 2 bytes from STDIN assuming 16 bit signed int low endian
        return short => sample
    */
    unsigned char buf[2];
    if (read(0, &buf, sizeof(buf)) <=0 )
    {
        exit(0);
    }
    samples_counter++;
    short t = (buf[1] << 8) | buf[0];
    windowf_push(intbuf, t);
    float * r;
    windowf_read(intbuf, &r);
    float sum = 0;
    for (int i = 0; i < INT_BUFFER_LENGTH; i++)
    {
        sum += r[i];
    }
    return sum/INT_BUFFER_LENGTH;
}

char checkPreambule(float *buf, short length)
{
    char found = 1;

    for (short i = 0; i < length - 1; i++)
    {
        found &=    buf[i+1] - buf[i] < SAMPLES_PER_SYMBOL*1.8 &&
                    buf[i+1] - buf[i] > SAMPLES_PER_SYMBOL*0.2;
    }
    return found;
}


int main()
{
    int BAUDRATE = 2000;
    int SAMPLING = 44000;
    short PREAMBULE_LENGTH = 32;
    short PACKET_LENGTH = 22;


    SAMPLES_PER_SYMBOL = (int)SAMPLING/BAUDRATE;

    INT_BUFFER_LENGTH = (int)SAMPLES_PER_SYMBOL*0.91;

    if (SAMPLES_PER_SYMBOL < 2)
    {
        printf("Error: samples per symbol should be greater then 2!\n");
        return -1;
    }
    if (SAMPLING % BAUDRATE)
    {
        printf("Error: samples per symbol shoud be integer!\n");
        return -1;
    }
    windowf wbuf = windowf_create(PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + PACKET_LENGTH*8*SAMPLES_PER_SYMBOL);
    windowf pbuf = windowf_create(PREAMBULE_LENGTH + 1);
    intbuf = windowf_create(INT_BUFFER_LENGTH);

    windowf_push(wbuf, readSample());
    
    while (1)
    {
        windowf_push(wbuf, readSample());
        float * rwbuf;
        windowf_read(wbuf, &rwbuf);
        if (rwbuf[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL - 1]*rwbuf[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL] < 0)
        {
            //Zero cross detected

            windowf_push(pbuf, samples_counter);
            float * r;
            windowf_read(pbuf, &r);
            char t = checkPreambule(r, PREAMBULE_LENGTH + 1);
            if (t)
            {
                //printf("PREAMBULE DETECTED! => %li\n", samples_counter);

                int padding = PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + SAMPLES_PER_SYMBOL/2;
                
                float threshold = 0;
                float last_extremum = 0;
                float max = 0;
                float min = 0;
                for (int j = 0; j < PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL; j++)
                {
                    if (rwbuf[j] > max)
                    {
                        max = rwbuf[j];
                    }
                    if (rwbuf[j] < min)
                    {
                        min = rwbuf[j];
                    }
                }                
                unsigned char packet[PACKET_LENGTH];
                for (int i = 0; i < PACKET_LENGTH; i++)
                {
                    unsigned char byte = 0;
                    for (int j = 0; j < 8; j++)
                    {
                        float sample = rwbuf[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL];
                        
                        if (sample < threshold)
                        {
                            byte = byte << 1;
                        } else {
                            byte = byte << 1 | 1;
                        }
                        threshold *= 0.9f;
                        if ( (last_extremum - sample)*(last_extremum - sample) > (max-min)*(max-min)/16 )
                        {
                            //If there was extremum change
                            threshold = (last_extremum + sample)/2;
                            last_extremum = sample;
                            
                        }
                    }
                    packet[i] = byte;
                }
                if (packet[0]==0x2d && packet[1] == 0xd4) {
                    
                    for (int i = 0; i < PACKET_LENGTH; i++)
                    {
                        printf("%02X ", packet[i]);
                    }
                    //If valid packet has been detected, fill buffer with new data
                    for (int i = 0; i < (PACKET_LENGTH*8*SAMPLES_PER_SYMBOL); i++)
                    {
                        windowf_push(wbuf, readSample());
                    }
                    printf("\n");
                } 
            } 
        }
    }
}