#include "fskdemod.h"
/*
rtl_fm -p 75 -g 49 -f 437000000 -s 20000 - | sox -t raw -r 20000 -es -b 16 -c 1 - -t raw -r 44000 - sinc 0-2000 compand 0.01,0.01 -100,0 -20 | ~/development/fskdemod/a.out
nc -l -u 7355 | ./a.out --baudrate=2400 --sampling=48000 --preambule-length=32 --detection-level=0.9 --payload-only --raw --disable-crc --reed-solomon 16 --packet-length=39

*/

int main(int argc, char *argv[])
{
    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    
    if (arguments.VERBOSE)
    {
        printf("Using:\nBAUDRATE: %i\nSAMPLING: %i\n", arguments.BAUDRATE, arguments.SAMPLING);
        printf(" --> %i SAMPLES/SYMBOL\n", (int)arguments.SAMPLING/arguments.BAUDRATE);
    }

    if (arguments.REED_SOLOMON)
    {
        rs = correct_reed_solomon_create(correct_rs_primitive_polynomial_ccsds, 1, 1, arguments.REED_SOLOMON);
    }

    short PREAMBULE_LENGTH = arguments.PREAMBULE_LENGTH;
    short PACKET_LENGTH = arguments.PACKET_LENGTH;

    SAMPLES_PER_SYMBOL = (int)arguments.SAMPLING/arguments.BAUDRATE;

    INT_BUFFER_LENGTH = (int)SAMPLES_PER_SYMBOL*0.9;
    
    if (SAMPLES_PER_SYMBOL < 2)
    {
        printf("Error: samples per symbol should be greater then 2!\n");
        return -1;
    }


    windowf wbuf = windowf_create(PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + PACKET_LENGTH*8*SAMPLES_PER_SYMBOL);
    intbuf = windowf_create(INT_BUFFER_LENGTH);

    windowf_push(wbuf, readSample());
    
    /*
        Generate preambule pattern
    */
    float preambule_test[PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL];
    for (int i = 0; i < PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL; i++)
    {
        float s = sin(i*3.14*(arguments.MANCHESTER == MANCHESTER_FULL ? 2 : 1)/SAMPLES_PER_SYMBOL);
        preambule_test[i] =  s > 0 ? 1 : (s < 0 ? -1 : 0);
    }
    /*
        Read samples and process it
    */
    while (1)
    {
        float sample = readSample();
        windowf_push(wbuf, sample);
        float * rwbuf;
        windowf_read(wbuf, &rwbuf);
        /*
            Check for a preambule pattern
        */
        if (abs(correlate(preambule_test, wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL)) >= power(wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL)*arguments.DETECTION_LEVEL)
        {
            /*
            if (arguments.VERBOSE)
            {
                printf("PREAMBULE DETECTED! => %li (samples from start)\n", samples_counter);
            }
            */
            int padding = PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL + SAMPLES_PER_SYMBOL/2;
            
            //float threshold = 0;
            
            float threshold = avarage(wbuf, PREAMBULE_LENGTH*SAMPLES_PER_SYMBOL);               
            unsigned char packet[PACKET_LENGTH];
            for (int i = 0; i < PACKET_LENGTH; i++)
            {
                unsigned char byte = 0;
                for (int j = 0; j < 8; j++)
                {
                    if (!arguments.MANCHESTER)
                    {
                            float sample = rwbuf[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL];
                        
                            if (sample < threshold)
                            {
                                byte = byte << 1 | (arguments.INVERT ? 1 : 0);
                            } else {
                                byte = byte << 1 | (arguments.INVERT ? 0 : 1);
                            }
                    } else {
                        float start = rwbuf[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL - SAMPLES_PER_SYMBOL/3];
                        float end = rwbuf[padding + i*8*SAMPLES_PER_SYMBOL + j*SAMPLES_PER_SYMBOL + SAMPLES_PER_SYMBOL/3];
                        if (start < end)
                        {
                            byte = byte << 1 | (arguments.INVERT ? 1 : 0);
                        } else {
                            byte = byte << 1 | (arguments.INVERT ? 0 : 1);
                        }
                    }
                }
                packet[i] = byte;
            }
            if (!arguments.SYNC_WORD_CHECK_DISABLE && packet[0]==(arguments.SYNC_WORD >> 8) && packet[1] == (arguments.SYNC_WORD & 0xFF)) {
                if (!arguments.CRC_CHECK_DISABLE)
                {
                    //Check for CRC
                    unsigned short received_crc, calculated_crc;
                    uint8_t calculated_packet_length;
                    calculated_packet_length = PACKET_LENGTH;
                    if (arguments.DYNAMIC_PACKET_LENGTH)
                    {
                        calculated_packet_length = packet[2];
                        received_crc = packet[3 + calculated_packet_length];
                        received_crc = received_crc << 8 | packet[3 + calculated_packet_length + 1];
                        calculated_crc =  crc16(&packet[2 + 1], calculated_packet_length);
                        //printf("%x %x\n", received_crc, calculated_crc);
                    } else {
                        received_crc = packet[PACKET_LENGTH-2];
                        received_crc = received_crc << 8 | packet[PACKET_LENGTH-1];
                        calculated_crc =  crc16(&packet[2], PACKET_LENGTH-2-2);
                    }
                    
                    if (received_crc == calculated_crc)
                    {
                        print_packet(packet, PACKET_LENGTH);
                        if (!arguments.RAW_OUTPUT)
                        {
                            printf("\n");
                        }
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
                    if (!arguments.RAW_OUTPUT)
                    {
                        printf("\n");
                    }
                    //If valid packet has been detected, fill buffer with new data
                    for (int i = 0; i < (PACKET_LENGTH*8*SAMPLES_PER_SYMBOL); i++)
                    {
                        windowf_push(wbuf, readSample());
                    }
                }
                
                
            } else if (!arguments.SYNC_WORD_CHECK_DISABLE) {
                if (arguments.VERBOSE)
                {
                    //printf("SYNC_WORD CHECK FILED\n");
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