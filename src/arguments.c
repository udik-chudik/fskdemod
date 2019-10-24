#include "fskdemod.h"


struct arguments arguments = {
    BAUDRATE: 2000,
    SAMPLING: 44000,
    PREAMBULE_LENGTH: 32,
    PACKET_LENGTH: 22,
    SYNC_WORD_CHECK_DISABLE: false,
    CRC_CHECK_DISABLE: false,
    INVERT: false,
    VERBOSE: false,
    DETECTION_LEVEL: 0.99,
    SYNC_WORD: 0x2dd4,
    MANCHESTER: MANCHESTER_DISABLED,
    DYNAMIC_PACKET_LENGTH: false,
    PAYLOAD_ONLY: false,
    RAW_OUTPUT: false,
    REED_SOLOMON: 0,
};

const char *argp_program_version = "1.0";
const char *argp_program_bug_address = "<udikchudik@gmail.com>";
static char doc[] = "G/FSK demodulator";
static char args_doc[] = "ARG1 ARG2";
static struct argp_option options[] = {
  {"baudrate",              'b', "BAUD", 0,  "Symbols baudrate (default 2000)" },
  {"sampling",              's', "SAMPLING", 0,  "Input sampling rate (default 44000)" },
  {"preambule-length",      'p', "PREAMBULE_LENGTH", 0,  "Preambule length in bits (default 32)" },
  {"packet-length",         'l', "PACKET_LENGTH", 0, "Packet total length (exclude preambule) in bytes (default 22)" },
  {"detection-level",       'd', "DETECTION_LEVEL", 0, "Preambule correlation level (default 0.99). Should be lower for low SNR" },
  {"disable-crc",           'c', 0, 0, "Disable CRC checking (default enabled)" },
  {"disable-sync",          'y', 0, 0, "Disable sync word checking (default enabled)" },
  {"invert",                'i', 0, 0, "Invert 0 and 1 (default disabled)" },
  {"verbose",               'v', 0, 0, "Print all avaible information" },
  {"sync",                  'w', "SYNC_WORD", 0, "Sync word folowed after preambule (default 0x2dd4)" },
  {"manchester",            'm', "MANCHESTER", 0, "Manchester type: 0 - disabled (do not use manchester), 1 - full packet with preambule is manchester encoded (default 0)" },
  {"dynamic-packet-length", 'a', 0, 0, "Enable dynamic packet length (default: false)" },
  {"payload-only",          'z', 0, 0, "Only payload output (default: false)" },
  {"raw",                   'x', 0, 0, "Raw output (default: false)" },
  {"reed-solomon",          'e', "REED_SOLOMON", 0, "Enable Reed Solomon error correction decoding (default: disabled)" },
  { 0 }
};

struct argp argp = { options, parse_opt, args_doc, doc };

error_t parse_opt(int key, char * arg, struct argp_state * state)
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
        case 'a':
            arguments->DYNAMIC_PACKET_LENGTH = true;
            break;
        case 'w':
            arguments->SYNC_WORD = (unsigned short)strtol(arg, NULL, 16) & 0xffff;
            break;
        case 'z':
            arguments->PAYLOAD_ONLY = true;
            break;
        case 'x':
            arguments->RAW_OUTPUT = true;
            break;
        case 'm':
            switch (atoi(arg))
            {
            case 0:
                arguments->MANCHESTER = MANCHESTER_DISABLED;
                break;
            case 1:
                arguments->MANCHESTER = MANCHESTER_FULL;
                break;
            
            default:
                return ARGP_ERR_UNKNOWN;
            }
            break;
        case 'e':
            arguments->REED_SOLOMON = atoi(arg);
            break;
        case ARGP_KEY_ARG:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}