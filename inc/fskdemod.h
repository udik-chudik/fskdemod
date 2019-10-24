#include <argp.h>
#include <stdbool.h>
#include <correct.h>
#include <liquid/liquid.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef __FSKDEMOD
#define __FSKDEMOD
enum manchester_mode
{
    MANCHESTER_DISABLED = 0,
    MANCHESTER_FULL = 1
};
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
    unsigned short SYNC_WORD;
    enum manchester_mode MANCHESTER;
    bool DYNAMIC_PACKET_LENGTH;
    bool PAYLOAD_ONLY;
    bool RAW_OUTPUT;
    short REED_SOLOMON;
};

extern struct arguments arguments;
extern struct argp argp;

extern correct_reed_solomon *rs;

extern int SAMPLES_PER_SYMBOL;
extern int INT_BUFFER_LENGTH;
extern windowf intbuf;


error_t parse_opt(int key, char * arg, struct argp_state * state);
void print_packet(unsigned char * packet, int length);
float readSample();

float avarage(windowf buf, int length);
float correlate(float *preambule, windowf buf, int length);
float power(windowf buf, int length);
//char checkPreambule(float *buf, short length);

uint16_t crc16(unsigned char* pData, int length);


#endif