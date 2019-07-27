#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <liquid/liquid.h>
#include <argp.h>
#include <stdbool.h>
#include <math.h>
/*
rtl_fm -p 75 -g 49 -f 437000000 -s 20000 - | sox -t raw -r 20000 -es -b 16 -c 1 - -t raw -r 44000 - sinc 0-2000 compand 0.01,0.01 -100,0 -20 | ~/development/fskdemod/a.out
*/
const char *argp_program_version = "argp-ex3 1.0";
const char *argp_program_bug_address = "<bug-gnu-utils@gnu.org>";
static char doc[] = "G/FSK demodulator for Si4432";
static char args_doc[] = "ARG1 ARG2";
static struct argp_option options[] = {
  {"baudrate",  'b', "BAUD", 0,  "Symbols baudrate (default 2000)" },
  {"sampling",  's', "SAMPLING", 0,  "Input sampling rate (default 44000)" },
  {"preambule-length", 'p', "PREAMBULE_LENGTH", 0,  "Preambule length in bits (default 32)" },
  {"packet-length", 'l', "PACKET_LENGTH", 0, "Packet total length (exclude preambule) in bytes (default 22)" },
  {"detection-level", 'd', "DETECTION_LEVEL", 0, "Preambule correlation level (default 0.99). Should be lower for low SNR" },
  {"disable-crc", 'c', 0, 0, "Disable CRC checking (default enabled)" },
  {"disable-sync", 'y', 0, 0, "Disable sync word checking (default enabled)" },
  {"invert", 'i', 0, 0, "Invert 0 and 1 (default disabled)" },
  {"verbose", 'v', 0, 0, "Print all avaible information" },
  { 0 }
};
long unsigned int samples_counter = 0;
int SAMPLES_PER_SYMBOL;
int INT_BUFFER_LENGTH;
windowf intbuf;
uint16_t crc16(unsigned char* pData, int length)
{
    uint8_t i;
    uint16_t wCrc = 0x0000;
    while (length--) {
        wCrc ^= *(unsigned char *)pData++ << 8;
        for (i=0; i < 8; i++)
            wCrc = wCrc & 0x8000 ? (wCrc << 1) ^ 0x8005 : wCrc << 1;
    }
    return wCrc & 0xffff;
}
void print_packet(unsigned char * packet, int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%02X ", packet[i]);
    }
}
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
        found &=    buf[i+1] - buf[i] < SAMPLES_PER_SYMBOL*2 &&
                    buf[i+1] - buf[i] > SAMPLES_PER_SYMBOL*0.2;
    }
    return found;
}
struct arguments
{
    int BAUDRATE;
    int SAMPLING;
    short PREAMBULE_LENGTH;
    short PACKET_LENGTH;
    bool CRC_CHECK_DISABLE;
    bool SYNC_WORD_CHECK_DISABLE;
    bool INVERT;
    bool VERBOSE;
    float DETECTION_LEVEL;
};

static error_t parse_opt(int key, char * arg, struct argp_state * state)
{
    struct arguments *arguments = state->input;
    switch (key)
    {
        case 'b':
            arguments->BAUDRATE = atoi(arg);
            break;
        case 's':
            arguments->SAMPLING = atoi(arg);
            break;
        case 'l':
            arguments->PACKET_LENGTH = atoi(arg);
            break;
        case 'p':
            arguments->PREAMBULE_LENGTH = atoi(arg);
            break;
        case 'c':
            arguments->CRC_CHECK_DISABLE = true;
            break;
        case 'y':
            arguments->SYNC_WORD_CHECK_DISABLE = true;
            break;
        case 'i':
            arguments->INVERT = true;
            break;
        case 'v':
            arguments->VERBOSE = true;
            break;
        case 'd':
            arguments->DETECTION_LEVEL = atof(arg);
            break;
        case ARGP_KEY_ARG:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
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

static struct argp argp = { options, parse_opt, args_doc, doc };
int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.BAUDRATE = 2000;
    arguments.SAMPLING = 44000;
    arguments.PREAMBULE_LENGTH = 32;
    arguments.PACKET_LENGTH = 22;
    arguments.SYNC_WORD_CHECK_DISABLE = false;
    arguments.CRC_CHECK_DISABLE = false;
    arguments.INVERT = false;
    arguments.VERBOSE = false;
    arguments.DETECTION_LEVEL = 0.99;
    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    
    if (arguments.VERBOSE)
    {
        printf("Using:\nBAUDRATE: %i\nSAMPLING: %i\n", arguments.BAUDRATE, arguments.SAMPLING);
        printf(" --> %i SAMPLES/SYMBOL\n", (int)arguments.SAMPLING/arguments.BAUDRATE);
    }

    int BAUDRATE = arguments.BAUDRATE;
    int SAMPLING = arguments.SAMPLING;
    short PREAMBULE_LENGTH = arguments.PREAMBULE_LENGTH;
    short PACKET_LENGTH = arguments.PACKET_LENGTH;


    SAMPLES_PER_SYMBOL = (int)SAMPLING/BAUDRATE;

    INT_BUFFER_LENGTH = (int)SAMPLES_PER_SYMBOL*0.9;
    
    if (SAMPLES_PER_SYMBOL < 2)
    {
        printf("Error: samples per symbol should be greater then 2!\n");
        return -1;
    }
    /*
    if (SAMPLING % BAUDRATE)
    {
        printf("Error: samples per symbol shoud be integer!\n");
        return -1;
    }
    */

    windowf wbuf = windowf_create(PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + PACKET_LENGTH*8*SAMPLES_PER_SYMBOL);
    windowf pbuf = windowf_create(PREAMBULE_LENGTH + 1);
    intbuf = windowf_create(INT_BUFFER_LENGTH);

    windowf_push(wbuf, readSample());
    
    /*
        Generate preambule pattern
    */
    float preambule_test[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL];
    for (int i = 0; i < PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL; i++)
    {
        float s = sin(3.14/2 + i*3.14/SAMPLES_PER_SYMBOL);
        preambule_test[i] =  s > 0 ? 1 : (s < 0 ? -1 : 0);
        //printf("%i %f\n", i, preambule_test[i]);
    }
    while (1)
    {
        float sample = readSample() ;
        windowf_push(wbuf, sample);
        float * rwbuf;
        windowf_read(wbuf, &rwbuf);
        //printf("%li %f %f %f\n", samples_counter, correlate(preambule_test, wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL), sample*100, power(wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL));
        if (1/*rwbuf[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL - 1]*rwbuf[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL] < 0*/)
        {
            //Zero cross detected

            //windowf_push(pbuf, samples_counter);
            //float * r;
            //windowf_read(pbuf, &r);
            //char t = checkPreambule(r, PREAMBULE_LENGTH + 1);
            if (abs(correlate(preambule_test, wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL)) >= power(wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL)*arguments.DETECTION_LEVEL)
            {
                if (arguments.VERBOSE)
                {
                    printf("PREAMBULE DETECTED! => %li (samples from start)\n", samples_counter);
                }

                int padding = PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + SAMPLES_PER_SYMBOL/2;
                
                float threshold = 0;
                float last_extremum = 0;
                float max = 0;
                float min = 0;
                /*
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
                */
                threshold = avarage(wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL);               
                unsigned char packet[PACKET_LENGTH];
                for (int i = 0; i < PACKET_LENGTH; i++)
                {
                    unsigned char byte = 0;
                    for (int j = 0; j < 8; j++)
                    {
                        float sample = rwbuf[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL];
                        
                        if (sample < threshold)
                        {
                            byte = byte << 1 | (arguments.INVERT ? 1 : 0);
                        } else {
                            byte = byte << 1 | (arguments.INVERT ? 0 : 1);
                        }
                        /*
                        threshold *= 0.9f;
                        if ( (last_extremum - sample)*(last_extremum - sample) > (max-min)*(max-min)/16 )
                        {
                            //If there was extremum change
                            threshold = (last_extremum + sample)/2;
                            last_extremum = sample;
                            
                        }
                        */
                    }
                    packet[i] = byte;
                }
                if (!arguments.SYNC_WORD_CHECK_DISABLE && packet[0]==0x2d && packet[1] == 0xd4) {
                    //unsigned short received_crc = (*(unsigned short *)&packet[PACKET_LENGTH-2]);
                    if (!arguments.CRC_CHECK_DISABLE)
                    {
                        //Check for CRC
                        unsigned short received_crc = packet[PACKET_LENGTH-2];
                        received_crc = received_crc << 8 | packet[PACKET_LENGTH-1];
                        if (received_crc == crc16(&packet[2], PACKET_LENGTH-2-2))
                        {
                            print_packet(packet, PACKET_LENGTH);
                            printf("\n");
                            //If valid packet has been detected, fill buffer with new data
                            for (int i = 0; i < (PACKET_LENGTH*8*SAMPLES_PER_SYMBOL); i++)
                            {
                                windowf_push(wbuf, readSample());
                            }
                        } else {
                            if (arguments.VERBOSE)
                            {
                                printf("Packet detected, but CRC check filed");
                                printf("\n");
                            }
                        }
                    } else {
                        print_packet(packet, PACKET_LENGTH);
                        printf("\n");
                        //If valid packet has been detected, fill buffer with new data
                        for (int i = 0; i < (PACKET_LENGTH*8*SAMPLES_PER_SYMBOL); i++)
                        {
                            windowf_push(wbuf, readSample());
                        }
                    }
                    
                    
                } else if (!arguments.SYNC_WORD_CHECK_DISABLE) {
                    if (arguments.VERBOSE)
                    {
                        printf("SYNC_WORD CHECK FILED\n");
                    }
                }
                if (arguments.SYNC_WORD_CHECK_DISABLE)
                {
                    print_packet(packet, PACKET_LENGTH);
                    printf("\n");
                    //If valid packet has been detected, fill buffer with new data
                    for (int i = 0; i < (PACKET_LENGTH*8*SAMPLES_PER_SYMBOL); i++)
                    {
                        windowf_push(wbuf, readSample());
                    }
                }
            } 
        }
    }
}