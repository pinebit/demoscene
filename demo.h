/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#ifndef PINE_DEMO_H
#define PINE_DEMO_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Init demo subsystem. Return TRUE if successfull. */
BOOL demo_init(HWND hwnd, int base_w, int base_h);

/* Stop demo and destroy the working thread. */
void demo_fini(void);

/* Start demo in a dedicated thread. Return TRUE if successfull. */
BOOL demo_run(void);

#endif /* PINE_DEMO_H */

