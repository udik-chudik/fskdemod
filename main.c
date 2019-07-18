#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <liquid/liquid.h>

long unsigned int samples_counter = 0;
int SAMPLES_PER_SYMBOL;
int INT_BUFFER_LENGTH = 20;
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
    return sum;
}

char checkPreambule(float *buf, short length)
{
    char found = 1;

    for (short i = 0; i < length - 1; i++)
    {
        found &=    buf[i+1] - buf[i] < SAMPLES_PER_SYMBOL*1.3 &&
                    buf[i+1] - buf[i] > SAMPLES_PER_SYMBOL*0.7;
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

    if (SAMPLES_PER_SYMBOL < 2)
    {
        printf("Error: samples per symbol should be greater then 2!\n");
        return -1;
    }
    
    windowf wbuf = windowf_create(PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + PACKET_LENGTH*8*SAMPLES_PER_SYMBOL);
    windowf pbuf = windowf_create(PREAMBULE_LENGTH + 1);
    intbuf = windowf_create(INT_BUFFER_LENGTH);

    

    int co = 0;
    float prev, next;

    prev = readSample();
    windowf_push(wbuf, prev);
    while (1)
    {

        next = readSample();
        windowf_push(wbuf, next);
        if (prev*next < 0)
        {
            //Zero cross detected

            windowf_push(pbuf, samples_counter);
            float * r;
            windowf_read(pbuf, &r);
            char t = checkPreambule(r, PREAMBULE_LENGTH + 1);
            if (t)
            {
                //printf("DETECTED! => %li\n", samples_counter);
                
                
                int padding = PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + 10;
                for (int j = 0; j < PACKET_LENGTH*8*SAMPLES_PER_SYMBOL; j++)
                {
                    windowf_push(wbuf, readSample());
                }
                
                float * r;
                windowf_read(wbuf, &r);
                float max = 0;
                for (int j = 0; j < PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL; j++)
                {
                    if (r[j] > max)
                    {
                        max = r[j];
                    }
                }
                //printf("%f\n" , max);
                float prev_sample = 0;
                for (int i = 0; i < PACKET_LENGTH; i++)
                {
                    unsigned char byte = 0;
                    for (int j = 0; j < 8; j++)
                    {
                        float AVG = max*0.8;
                        float sample = r[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL];
                        //float prev_sample = r[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL - SAMPLES_PER_SYMBOL];
                        float p = 0;
                        //p = 0;
                        if (j || i)
                        {
                            if (prev_sample > 0)
                            {
                                p = prev_sample - AVG;
                            } else {
                                p = prev_sample + AVG;
                            }
                        }
                        if (sample < p)
                        {
                            byte = byte << 1 | 1;
                            prev_sample = sample;
                        } else {
                            byte = byte << 1;
                        }
                    }
                    printf("%x ", byte);
                }
                printf("\n");
                
            }
            //printf("%i\n", t);
            
        }
        prev = next;
        co++;
        if (samples_counter > 20000)
            return 0;
    }
}