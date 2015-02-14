/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#ifndef PINE_MIDI_H
#define PINE_MIDI_H

#define MIDI_FINISHED   0xFFFFFFFF

/* Read-only counter. Reflects currently playing position:
 * HIWORD: pattern
 * LOWORD: position in pattern
 * if == MIDI_FINISHED then music is over.
 */
extern unsigned int midi_position;

/* Read-only time interval between notes (pace). */
extern unsigned int midi_interval;

/* Read-only playing time adjusted to notes. */
extern unsigned int midi_time;

/* Init MIDI subsystem. Return TRUE if successfull. */
BOOL midi_init(void);

/* Close MIDI device and destroy the playing thread. */
void midi_fini(void);

/* Start playing in a dedicated thread. Return TRUE if successfull. */
BOOL midi_play(void);

#endif /* PINE_MIDI_H */
