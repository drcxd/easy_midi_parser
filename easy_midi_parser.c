#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VLNMASK 0x0000007f
#define VMHM 0xf0
#define VMLM 0x0f
#define DLM 0x00ff
#define DHM 0x7f00
#define PRESS "press"
#define RELEASE "release"

int read_vln(FILE *);
int parse_header(FILE *);
void parse_tracks(FILE *, int);

void test_read_vln();

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [midifile]\n", argv[0]);
    }
    FILE *file = fopen(argv[1], "r");
    int tracks = parse_header(file);
    for (int i = 0; i < tracks; ++i)
    {
        parse_tracks(file, i);
    }
    fclose(file);
    /* unit test */
    /* test_read_vln(); */
    /* parse_header(file); */
    return 0;
}

char parse_events(FILE *file)
{
    static unsigned char prev_op = 0;
    char buff[256];
    memset(buff, 0, 256);
    int delta = read_vln(file);
    printf("delta\t%d\t", delta);
    unsigned char op = fgetc(file);
    if (op == 0xff)
    {
        printf("[meta event]\t");
        char type = fgetc(file);
        int len = read_vln(file);
        switch (type)
        {
        case 0x03:
        {
            fread(buff, 1, len, file);
            printf("Sequence/Track Name: %s\n", buff);
            break;
        }
        case 0x04:
        {
            fread(buff, 1, len, file);
            printf("Instrument Name: %s\n", buff);
            break;
        }
        case 0x2f:
        {
            printf("End of Track\n");
            return 1;
            break;
        }
        case 0x51:
        {
            int mics_per_qua = 0;
            fread(((char *)&mics_per_qua + 1), 1, len, file);
            mics_per_qua = __builtin_bswap32(mics_per_qua);
            printf("%d microseconds per quarter note\n", mics_per_qua);
            break;
        }
        case 0x58:
        {
            printf("Time Signature:\n");
            unsigned char nn, dd, cc, bb;
            fread(&nn, 1, 1, file);
            fread(&dd, 1, 1, file);
            fread(&cc, 1, 1, file);
            fread(&bb, 1, 1, file);
            unsigned char deno = 1;
            while (dd > 0)
            {
                deno *= 2;
                --dd;
            }
            printf("numerator\t%hhd\t", nn);
            printf("denominator\t%hhd\t", deno);
            printf("%hhd\tMIDI clocks in a metronome click\t", cc);
            printf("%hhd\t32nd-notes in a MIDI quarter-note (24 MIDI clocks)\n", bb);
            break;
        }
        case 0x59:
        {
            printf("Key Signature:\t");
            unsigned char sf, mi;
            fread(&sf, 1, 1, file);
            fread(&mi, 1, 1, file);
            printf("sf\t%hhd\t", sf);
            printf("mi\t%hhd\n", mi);
            break;
        }
        default:
        {
            for (int i = 0; i < len; ++i)
            {
                fgetc(file);
            }
            printf("\n");
            fprintf(stderr, "Unhandled meta event type: 0x%x\n", type);
            fprintf(stderr, "curren file position: %d\n", ftell(file));
            break;
        }
        }
        prev_op = 0;
    }
    else if (op >= 0xf0)
    {
        // system common message
        printf("[sysex event]\t");
        printf("\n");
        prev_op = 0;
    }    
    else
    {
        /* channel voice message */
        printf("[midi event]\t");
        char hop = (op & VMHM) >> 4;
        char channel = (op & VMLM);
        switch (hop)
        {
        case 8: /* 0b1000 */
        {
            int pitch = fgetc(file);
            int velocity = fgetc(file);
            const char *move = RELEASE;
            printf("Note\t0x%x\t%s\tchannel\t%lld\tvelocity\t%d\n", pitch, move, channel, velocity);
            prev_op = op;
            break;
        }
        case 9: /* 0b1001 */
        {
            int pitch = fgetc(file);
            int velocity = fgetc(file);
            const char *move = PRESS;
            printf("Note\t0x%x\t%s\tchannel\t%lld\tvelocity\t%d\n", pitch, move, channel, velocity);
            prev_op = op;
            break;
        }
        case 11: /* 0b1011 */
        {
            int c = fgetc(file);
            int v = fgetc(file);
            printf("Channel Mode Message\n");
            prev_op = op;
            break;
        }
        case 12: /* 0b1100 */
        {
            int p = fgetc(file);
            printf("Program change Message\n");
            prev_op = op;
            break;
        }
        default:
        {
            /* may be it is the running status in effect */
            if (prev_op)
            {
                char prev_hop = (prev_op & VMHM) >> 4;
                char prev_channel = (prev_op & VMLM);
                switch (prev_hop)
                {
                case 8:
                {
                    int pitch = op;
                    int velocity = fgetc(file);
                    const char *move = RELEASE;
                    printf("Note\t0x%x\t%s\tchannel\t%lld\tvelocity\t%d\n", pitch, move, prev_channel, velocity);                    
                    break;
                }
                case 9:
                {
                    int pitch = op;
                    int velocity = fgetc(file);
                    const char *move = PRESS;
                    printf("Note\t0x%x\t%s\tchannel\t%lld\tvelocity\t%d\n", pitch, move, prev_channel, velocity);
                    break;
                }
                case 11:
                {
                    int c = op;
                    int v = fgetc(file);
                    printf("Channel Mode Message\n");
                    break;
                }
                case 12:
                {
                    int p = op;
                    printf("Program change Message\n");
                    break;
                }
                default:
                {
                    prev_op = 0;
                    printf("\n");
                    fprintf(stderr, "Running status Unhandled MTrk Event op: 0x%hhx\n", op);
                    fprintf(stderr, "Current file position: %d\n", ftell(file));
                    break;
                }
                }
            }
            else
            {
                /* not running status, something must be wrong */
                printf("\n");
                fprintf(stderr, "Unhandled MTrk Event op: 0x%hhx\n", op);
                fprintf(stderr, "Current file position: %d\n", ftell(file));
            }
            break;
        }
        }
    }
    return 0;
}

void parse_tracks(FILE *file, int trackn)
{
    fprintf(stderr, "starting parse track no.%d\n", trackn);
    char buffer[256];
    memset(buffer, 0, 256);
    fread(buffer, 1, 4, file);
    if (strncmp(buffer, "MTrk", 4) != 0)
    {
        fprintf(stderr, "Not a valid midi track\n");
        exit(0);
    }
    int len;
    fread(&len, 4, 1, file);
    len = __builtin_bswap32(len);
    printf("track no.%d has %d bytes\n", trackn, len);

    char end = 0;
    do
    {
        end = parse_events(file);
    }
    while (!end);
}

int parse_header(FILE* file)
{
    char buffer[256];
    memset(buffer, 0, 256);
    fread(buffer, 1, 4, file);
    if (strncmp(buffer, "MThd", 4) != 0)
    {
        fprintf(stderr, "Not a valid midi file\n");
        exit(0);
    }
    int len;
    fread(&len, 4, 1, file);
    len = __builtin_bswap32(len);
    // printf("len: 0x%08x\n", len);
    if (len != 0x06)
    {
        printf("Not a valid midi file\n");
        exit(0);
    }
    unsigned short format, ntrks, division;
    fread(&format, 2, 1, file);
    fread(&ntrks, 2, 1, file);
    fread(&division, 2, 1, file);
    // printf("format: 0x%04hx\t\tntrks: 0x%04hx\t\tdivision: 0x%04hx\n", format, ntrks, division);
    format = (format << 8) | (format >> 8);
    ntrks = (ntrks << 8) | (ntrks >> 8);
    division = (division << 8) | (division >> 8);
    // printf("format: 0x%04hx\t\tntrks: 0x%04hx\t\tdivision: 0x%04hx\n", format, ntrks, division);
    switch (format)
    {
    case 0: printf("single multi-channel track\n"); break;
    case 1: printf("one or more simultaneous tracks of a sequence\n"); break;
    case 2: printf("one or more sequentially independent single-track patterns\n"); break;
    }
    printf("number of track trunks: %d\n", ntrks);
    if (division >> 15 == 0)
    {
        unsigned short tpqn = division << 1 >> 1;
        printf("%d ticks per quarter-note\n", tpqn);
    }
    else
    {
        char SMPTE = division & DHM >> 8;
        char tpf = division & DLM;
        printf("SMPTE: %hhd\t\tticks per frame: %hhd\n", tpf);
        /* switch (SMPTE) */
        /* { */
        /* case -24: */
        /* case -25: */
        /* case -29: */
        /* case -30: */
        /* } */
    }
    return ntrks;
}

int read_vln(FILE* file)
{
    int n[4];
    int c;
    int i = 0;
    int result = 0;
    // printf("---- read vln start ----\n");
    do
    {
        c = fgetc(file);
        // printf("fgetc read %08x\n", c);
        n[i] = c;
        ++i;
    }
    while (c > 127 && i < 4);
    int j = 0;
    while (j < i)
    {
        int real = n[j] & VLNMASK;
        real <<= ((i - j - 1) * 7);
        result |= real;
        ++j;
    }
    // printf("---- read vln end ----\n");
    return result;
}

/* unit test */
void test_read_vln()
{
    FILE *file = fopen("input.txt", "r");
    for (int i = 0; i < 12; ++i)
    {
        printf("0x%08x\n", read_vln(file));
    }
    return;
}
