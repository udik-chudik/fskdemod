#include "fskdemod.h"

correct_reed_solomon *rs;
void print_out(unsigned char * packet, int length, int padding)
{
    for (int i = padding; i < padding + length; i++)
    {
        if (arguments.RAW_OUTPUT)
        {
            write(1, &packet[i], 1);
        } else {
            printf("%02X ", packet[i]);
        }
    }
}
void print_packet(unsigned char * packet, int length)
{
    if (arguments.DYNAMIC_PACKET_LENGTH)
    {
        length = packet[2];
    }
    if (arguments.REED_SOLOMON)
    {
        ssize_t encoded = correct_reed_solomon_decode(rs, &packet[3], arguments.DYNAMIC_PACKET_LENGTH ? length : length - 3, &packet[3]);
        length -= arguments.REED_SOLOMON;
        if (encoded < 0)
        {
            if (arguments.VERBOSE)
            {
                printf("Error! Cant correct code!\n");
            }
            return;
        }
    }
    print_out(packet, length - (arguments.PAYLOAD_ONLY ? 3 : 0), arguments.PAYLOAD_ONLY ? 3 : 0);
    /*
    for (int i = 0; i < length; i++)
    {
        
        if (arguments.PAYLOAD_ONLY)
        {
            if (!arguments.RAW_OUTPUT)
            {
                printf("%02X ", packet[i+3]);
            } else {
                write(1, &packet[i+3], 1);
            }
        } else {
            if (!arguments.DYNAMIC_PACKET_LENGTH)
            {
                printf("%02X ", packet[i]);
            } else {
                printf("%02X ", packet[i+3]);
            }
        }
    }
    */
}

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
float avarage(windowf buf, int length)
{
    float * rwbuf;
    windowf_read(buf, &rwbuf);
    float sum = 0;
    for (int i = 0; i < length; i++)
    {
        sum += rwbuf[i];
    }
    return sum/length;
}
float correlate(float *preambule, windowf buf, int length)
{
    float * rwbuf;
    windowf_read(buf, &rwbuf);
    float sum = 0;
    float avg = avarage(buf, length);
    for (int i = 0; i < length; i++)
    {
        sum += (rwbuf[i] - avg)*preambule[i];
    }
    return sum;
}
float power(windowf buf, int length)
{
    float * rwbuf;
    windowf_read(buf, &rwbuf);
    float sum = 0;
    float avg = avarage(buf, length);
    for (int i = 0; i < length; i++)
    {
        sum += abs(rwbuf[i] - avg);
    }
    return sum;
}
/*
char checkPreambule(float *buf, short length)
{
    char found = 1;

    for (short i = 0; i < length - 1; i++)
    {
        found &=    buf[i+1] - buf[i] < SAMPLES_PER_SYMBOL*2 &&
                    buf[i+1] - buf[i] > SAMPLES_PER_SYMBOL*0.2;
    }
    return found;
}
*/