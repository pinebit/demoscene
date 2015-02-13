/*
 * pinebit 4k intro
 * (c) 2010 pinebit
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "config.h"
#include "midi.h"

/*** MIDI commands helper ****************************************************
    * 8n nn vv - Note Off
    * 9n nn vv - Note On
    * An nn pp - Key Pressure - Polyphonic Aftertouch
    * Bn cc vv - Control Change
    * Cn pp - Program Change
    * Dn pp - Channel Pressure - Channel Aftertouch
    * En ll mm - Pitch Bend Change
******************************************************************************/

/* Global music definitions */
#define MIDI_BEAT            80
#define MIDI_TRANSPOSE       0
#define MIDI_CHANNELS        4
#define MELODY_LEN           14
#define PATTERN_LEN          32

/* Channel data */
typedef struct
{
    DWORD           message;        /* Short MIDI message */
    unsigned char   instrument;     /* Program/Instrument number */
    unsigned char   volume;         /* Channel volume */
    signed   char   transpose;      /* Transposition */
} midi_channel_t;

/* Exported data */
unsigned int midi_position  = 0;
unsigned int midi_interval  = 0;
unsigned int midi_time      = 0;

/* Local data */
static HMIDIOUT midi_device = NULL;
static HANDLE   midi_thread = NULL;
static DWORD    midi_pre_time = 0;

static midi_channel_t channels[MIDI_CHANNELS] = 
{
    { 0x00000090, 87, 40, -12 },        /* 87 - Lead 8 (bass + lead) */
    { 0x00000091,  2, 50,   0 },        /*  2 - Electric Grand Piano */
    { 0x00000092, 44, 40,  12 },        /* 44 - Tremolo Strings      */
    { 0x00000093,  0, 60,  12 }         /*  0 - Acoustic Grand Piano */
};

/* Pattern is just a set of keys */
static const char* patterns[] =
{
/* 1 */  "9@9@9@9@<C<C<C<C7>7>7>7>5<5<4;4;",
/* 2 */  "   @   @   C   C   >   >  5<  4;",
/* 3 */  "9@  9@  <C  <C  7>  7>  < <<; ;;",
/* 4 */  "99  99  <<  <<  77  77  55  44  ",
/* 5 */  " @ @ @9@ C C C<C > > >7> <5< ;4;",
/* 6 */  "9       <       7       5   4   "
};

/* Melody format: 0xAB, where A = pattern, B = key shift */
static const unsigned char melody[MIDI_CHANNELS][MELODY_LEN] =
{
{ 0x10, 0x10, 0x10, 0x40, 0x30, 0x50, 0x10, 0x40, 0x66, 0x36, 0x16, 0x66, 0x00, 0x00 },
{ 0x00, 0x00, 0x40, 0x2C, 0x2C, 0x3C, 0x40, 0x10, 0x36, 0x56, 0x16, 0x00, 0x00, 0x00 },
{ 0x00, 0x60, 0x40, 0x5C, 0x00, 0x3C, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x60, 0x50, 0x6C, 0x6C, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x16, 0x36, 0x00 }
};

/*
{ 0x10, 0x10, 0x10, 0x10, 0x60, 0x40, 0x30, 0x50, 0x10, 0x40, 0x5C, 0x4C, 0x66, 0x16, 0x36, 0x16, 0x66, 0x00 },
{ 0x00, 0x00, 0x40, 0x50, 0x10, 0x2C, 0x2C, 0x3C, 0x40, 0x10, 0x3C, 0x00, 0x36, 0x00, 0x56, 0x16, 0x00, 0x00 },
{ 0x00, 0x60, 0x40, 0x10, 0x20, 0x5C, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x16, 0x56, 0x00, 0x00, 0x00, 0x00 },
{ 0x00, 0x60, 0x50, 0x00, 0x00, 0x6C, 0x6C, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x16, 0x36 }
*/

static int pattern_shift = 0;
static int melody_shift  = 0;

BOOL midi_init()
{
    /* Open default MIDI device or die */
    int rc = midiOutOpen(&midi_device, 0, 0, 0, CALLBACK_NULL);
    if (rc != MMSYSERR_NOERROR)
    {
        return FALSE;
    }

    /* Set instruments for channels */
    for (rc = 0; rc < MIDI_CHANNELS; ++rc)
    {
        DWORD command = (0x000000C0 | rc) | (channels[rc].instrument << 8);
        midiOutShortMsg(midi_device, command);

        /* Accord expansion via pitch */
        midiOutShortMsg(midi_device, command | 4);
        command = (0xE0 | rc | 4) | (0x4600 << 8);
        midiOutShortMsg(midi_device, command);
    }

    midi_interval = 60000 / MIDI_BEAT / 4;

    return TRUE;
}

void midi_fini()
{
    if (midi_thread != NULL)
    {
        TerminateThread(midi_thread, 0);
    }

    midiOutReset(midi_device);
    midiOutClose(midi_device);
}

static void midi_pace()
{
    int cn;

    for (cn = 0; cn < MIDI_CHANNELS; ++cn)
    {
        char key;
        int  pattern = ((melody[cn][melody_shift] & 0xF0) >> 4) - 1;

        if (pattern < 0)
        {
            /* Reset previous key */
            channels[cn].message &= 0x0000FFFF;
            midiOutShortMsg(midi_device, channels[cn].message);
            midiOutShortMsg(midi_device, channels[cn].message | 4);
            continue;
        }

        key = patterns[pattern][pattern_shift];

        if (key != ' ')
        {
            DWORD new_message;

            key += melody[cn][melody_shift] & 0x0F;
            key += channels[cn].transpose + MIDI_TRANSPOSE;
            new_message = (0x00000090 | cn) | (key << 8) | (channels[cn].volume << 16);

            /* Reset previous key if note is different */
            if (new_message != channels[cn].message)
            {
                channels[cn].message &= 0x0000FFFF;
                midiOutShortMsg(midi_device, channels[cn].message);
                midiOutShortMsg(midi_device, channels[cn].message | 4);
            }

            /* Set new key */
            channels[cn].message = new_message;
            midiOutShortMsg(midi_device, channels[cn].message);
            midiOutShortMsg(midi_device, channels[cn].message | 4);
        }
    }

    if (++pattern_shift >= PATTERN_LEN)
    {
        pattern_shift = 0;
        melody_shift++;

        if (melody_shift >= MELODY_LEN)
        {
            /* The last chord is playing.. */
            Sleep(midi_interval);
            pattern_shift = melody_shift = 0xFFFF;
        }
    }

    midi_time += midi_interval;
    midi_position = (melody_shift << 16) | pattern_shift;
}

static DWORD WINAPI midi_play_proc(LPVOID not_used)
{
    Sleep(PLAY_DELAY);

    midi_pre_time = GetTickCount();

    while (midi_position != MIDI_FINISHED)
    {
        /* Calculate elapsed time */
        DWORD now   = GetTickCount();
        DWORD delta = now - midi_pre_time;

        if (now < midi_time)
        {
            delta = (0xFFFFFFFF - midi_pre_time) + now;
        }

        delta = midi_interval - delta;

        if (delta > 10)
        {
            Sleep(delta);
        }

#if (PLAY_MUSIC == TRUE)
        midi_pace();
#endif

        midi_pre_time = GetTickCount();
    }

    midi_thread = NULL;
    return 0;
}

BOOL midi_play()
{
    DWORD midi_thread_id;

    /* Starting playback in a dedicated thread */
    midi_thread = CreateThread(NULL, 0, &midi_play_proc, NULL, 0, &midi_thread_id);
    SetThreadPriority(midi_thread, THREAD_PRIORITY_TIME_CRITICAL);
    return (midi_thread != NULL);
}
