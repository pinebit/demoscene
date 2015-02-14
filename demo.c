/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#include "demo.h"
#include "config.h"
#include "midi.h"
#include "text.h"
#include "atom.h"

#include <GL\gl.h>

/*** Internal types **********************************************************/

/* Optional init function of state */
typedef void (*init_proc_t)(void);

/* Main state procedure. Takes time pos in [0..1]. */
typedef void (*exec_proc_t)(float);

typedef struct
{
    /*
     * Timings below is calculated from demo begin (=0) in ms,
     * and being synchronized with MIDI playing time/position.
     * The timeframes may overlap to achive multiple passes / complex gfx.
     */

    DWORD        time_in;       /* Time to enter state (ms)              */
    DWORD        duration;      /* State duration (ms)                   */
    init_proc_t  proc_init;     /* Once call on enter state, may be NULL */
    exec_proc_t  proc_run;      /* Runs after init till exit state       */
    int          in_state;      /* =1 if state is entered                */
} state_t;

static void rain_init(void);
static void rain_run(float);
static void wave_run(float);
static void flash_run(float);
static void rend_a_init(void);
static void rend_b_init(void);
static void rend_c_init(void);
static void rend_d_init(void);
static void rend_e_init(void);

/*** Internal demo data ******************************************************/

#define DEMO_END  0xFFFFFFFF
#define V_PI      3.14159265358f
#define V_2_PI    6.28318530717f
#define V_PI_2    1.57079632679f

/* Hardcoded demo flow */
static state_t demo_states[] =
{
    {    13500,   300,  rend_a_init,   flash_run, 0 },  /* Render text 1                   */
    {    30000,   300,  rend_b_init,   flash_run, 0 },  /* Render text 2                   */
    {    45000,   300,  rend_c_init,   flash_run, 0 },  /* Render text 3                   */
    {    58000,   300,  rend_d_init,   flash_run, 0 },  /* Render text 4                   */
    {    71000,   300,  rend_e_init,   flash_run, 0 },  /* Render text 5                   */
    {      500, 83000,         NULL,    wave_run, 0 },  /* Simple wave on background       */
    {        0, 81000,    rain_init,    rain_run, 0 },  /* Complex particle illusion       */
    { DEMO_END,     0,         NULL,        NULL, 0 }   /* End of table marker             */
};

static HWND   demo_wnd    = NULL;
static HANDLE demo_thread = NULL;
static int    demo_w;
static int    demo_h;
static DWORD  demo_time = 0;
static int    demo_seed = 0xBADC0FFE;

/* Atom Structures */
#define RAIN_ATOMS          0x2000
static GLuint               atom_tid  = 0;
static atom_t rain_atoms[RAIN_ATOMS];
static float  rain_dist[RAIN_ATOMS];
static int    rain_ipos[RAIN_ATOMS];
static float  rain_incline = 0.0f;
static float  rain_incline_to = 0.0f;

/*** Local functions *********************************************************/

/* Dummy pseudo-random generators */
static float perlin(int n)
{
    float v = 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
    return v;
}

static unsigned char perlin_byte(int n)
{
    float fpn = perlin(++n) * 255.0f;
    if (fpn < 0.0f) fpn = -fpn;
    return (char)(fpn);
}

static float vcos_int(float x)
{
    const float c1 =  0.9999932946f;
    const float c2 = -0.4999124376f;
    const float c3 =  0.0414877472f;
    const float c4 = -0.0012712095f;

    x = x * x;
    return (c1 + x * (c2 + x * (c3 + c4 * x)));
}

static float vcos(float x)
{
    int   quad;

    if (x < 0.0f) x = -x;
    while (x > V_2_PI) x -= V_2_PI;

    quad = (int)(x / V_PI_2);
    if (quad == 0) return vcos_int(x);
    if (quad == 1) return -vcos_int(V_PI - x);
    if (quad == 2) return -vcos_int(x - V_PI);
    
    return vcos_int(V_2_PI - x);
}

static float vsin(float x)
{
    return vcos(V_PI_2 - x);
}

void rain_init()
{
    /* Fix demo_h to get HD screen ratio */
    float r = 9.0f / 16.0f;
    int fixed_demo_h = (int)(demo_w * r);
    glViewport(0, (demo_h - fixed_demo_h) / 2, demo_w, fixed_demo_h);
    demo_h = fixed_demo_h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -r, r, 1.8, 6.0); 
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_TEXTURE_2D);
    atom_tid = atom_gen_texture();
    text_init();
    glBindTexture(GL_TEXTURE_2D, atom_tid);
}

void rain_run(float t)
{
    float camera = -t * 360.0f * 9.0f + 60.0f;
    float cam_x;
    float cam_y;
    float flash_z = -2.0f;
    float flash_v = 0.0f;
    int   a, n, anum = RAIN_ATOMS;

    /* Set camera */
    glLoadIdentity();
    glRotatef(-60.0f + vsin(t * 300.0f) / 2.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-135.0f, 0.0f, 0.0f, 1.0f);
    if (midi_position & 0x00040000)
    {
        rain_incline_to = 20.0f * vsin(t * 110.0f);
    }
    else
    {
        rain_incline_to = 0.0f;
    }

    if (rain_incline != rain_incline_to)
    {
        rain_incline += (rain_incline_to - rain_incline) / 50.0f;
    }
    glRotatef(rain_incline, 1.0f, 1.0f, 0.0f);
    glTranslatef(-2.2f, -2.2f, -2.0f);
    glScaled(1.2, 1.2, 1.2);

    glRotatef(camera, 0.0f, 0.0f, 1.0f);
    camera = camera / 360.f * V_PI * 2.0f;
    cam_x  = 3.0f * vcos(camera);
    cam_y  = 3.0f * vsin(camera);

    if (t < 0.1f)
    {
        anum = (int)(RAIN_ATOMS * t * 2.0f);
        if (anum < 0) anum = 0;
    }

    /* Process atoms rain */
    demo_seed = (demo_seed << 1) + (demo_seed >> 1);
    n = demo_seed;
    if (midi_position & 0x00020000)
    {
        flash_z = (midi_position & 0xFF & demo_seed) / 32.0f;
    }
    else
    {
        flash_z = -2.0f;
    }

    if (midi_position & 0x00050000)
    {
        flash_v = -V_2_PI * (midi_position & 0xFF) / 32.0f + V_PI;
    }
    else
    {
        flash_v = 100.0f;
    }

    for (a = 0; a < anum; ++a)
    {
        int tx;
        int ty;

        if (rain_atoms[a].state == 0)
        {
            float dr = perlin(++n) * 0.01f - 0.02f;
            rain_atoms[a].angle      = perlin(++n) * V_PI;
            rain_atoms[a].position.x = dr + vcos(rain_atoms[a].angle);
            rain_atoms[a].position.y = dr + vsin(rain_atoms[a].angle);

            rain_atoms[a].position.z = 2.0f + perlin(++n);
            rain_atoms[a].speed      = (perlin(++n) + 1.0f) / 100.0f + 0.01f;
            rain_atoms[a].position.w = 0.02f + perlin(++n) * 0.005f;

            rain_atoms[a].color.r = perlin_byte(++n);
            rain_atoms[a].color.g = perlin_byte(++n) / 2;
            rain_atoms[a].color.b = perlin_byte(++n) / 2;
            rain_atoms[a].color.a = 150;

            rain_atoms[a].state = 1;
        }
        
        ty = 0;
        if (rain_atoms[a].position.z < 1.0f && rain_atoms[a].position.z > 0.0f)
        {
            ty = (int)(rain_atoms[a].position.z * TEXTURE_SY);
        }
        
        tx = 0;
        if (rain_atoms[a].angle > -V_PI_2 && rain_atoms[a].angle < V_PI_2)
        {
            tx = (int)(TEXTURE_SX * (rain_atoms[a].angle + V_PI_2) / V_PI);
        }
        
        if (text_luma[ty * TEXTURE_SX + tx] > 0)
        {
            rain_atoms[a].position.z -= rain_atoms[a].speed / 10.0f;
            if (rain_atoms[a].color.a < 200) rain_atoms[a].color.a += 4;
        }
        else
        {
            rain_atoms[a].position.z -= rain_atoms[a].speed;
            if (rain_atoms[a].color.a > 50) rain_atoms[a].color.a -= 4;
        }

        if (rain_atoms[a].position.z < -1.0f)
        {
            rain_atoms[a].state = 0;
        }

        if (rain_atoms[a].position.z < flash_z + 0.01f && 
            rain_atoms[a].position.z > flash_z - 0.01f)
        {
            if (flash_z < -1.0f)
            {
                rain_atoms[a].color.a = 100;
            }
            else
            {
                rain_atoms[a].color.a = 230;
            }
        }

        if (t > 0.9f)
        {
            flash_v = 100.0f;
            rain_atoms[a].color.a /= 2;
        }

        if (rain_atoms[a].angle < flash_v + 0.05f &&
            rain_atoms[a].angle > flash_v - 0.05f)
        {
            rain_atoms[a].color.a = 200;
        }

        rain_dist[a] =  (cam_x - rain_atoms[a].position.x) * (cam_x - rain_atoms[a].position.x);
        rain_dist[a] += (cam_y - rain_atoms[a].position.y) * (cam_y - rain_atoms[a].position.y);
        rain_dist[a] += (2.0f  - rain_atoms[a].position.z) * (2.0f  - rain_atoms[a].position.z);
        rain_dist[a] = -rain_dist[a];
        rain_ipos[a] = a;
    }

    while (a++ < anum)
    {
        rain_dist[a] = 0.0f;
        rain_ipos[a] = 0;
    }

    atom_sort(rain_dist, rain_ipos, 0, anum);
    
    camera *= 57.2957795f;
    camera += 45.0f;
    for (a = 0; a < anum; ++a)
    {
        atom_t *p_atom = &rain_atoms[rain_ipos[a]];

        glColor4ub(p_atom->color.r, p_atom->color.g, p_atom->color.b, p_atom->color.a);

        glPushMatrix();
        
        glTranslatef(p_atom->position.x, p_atom->position.y, p_atom->position.z);
        glScalef(p_atom->position.w, p_atom->position.w, p_atom->position.w);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glRotatef(camera, 0.0f, 1.0f, 0.0f);
        
        glBegin(GL_QUADS);
            glTexCoord2i(0, 0); glVertex2i(-1, -1);
            glTexCoord2i(1, 0); glVertex2i( 1, -1);
            glTexCoord2i(1, 1); glVertex2i( 1,  1);
            glTexCoord2i(0, 1); glVertex2i(-1,  1);
        glEnd();

        glPopMatrix();
    }
}

void flash_run(float t)
{
    float color = 1.0f - t * t;
    glClearColor(color, color, color, 1.0f);
}

void rend_a_init(void)
{
    text_render("INTRO in 4K");
}

void rend_b_init(void)
{
    text_render("idea, code, music");
}

void rend_c_init(void)
{
    text_render("Andrei Smirnov");
}

void rend_d_init(void)
{
    text_render("pinebit@gmail.com");
}

void rend_e_init(void)
{
    text_render("THANK YOU!");
}

void wave_run(float t)
{
    float x;
    float y;

    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -3.0f);

    for (y = -1.0f; y < 1.0f; y += 0.05f)
    {
        for (x = -2.0f; x < 2.0f; x += 0.05f)
        {
            float h = vsin(y * t * 150.0f) + vcos(x * t * 100.0f);
            h /= 10.0f;

            if (t < 0.9f)
            {
                float fc = midi_position / h;
                glColor4f(fc, fc, fc, 0.1f);
            }
            else
            {
                glColor4f(1.0f - t, 1.0f - t, 1.0f - t, 1.0f - t);
            }

            glBegin(GL_QUADS);
                glTexCoord2i(0, 0); glVertex3f(x, y, h);
                glTexCoord2i(1, 0); glVertex3f(x + 0.09f, y, h);
                glTexCoord2i(1, 1); glVertex3f(x + 0.09f, y + 0.09f, h);
                glTexCoord2i(0, 1); glVertex3f(x, y + 0.09f, h);
            glEnd();
        }
    }
}

static DWORD WINAPI demo_proc(LPVOID not_used)
{
    HGLRC   hrc;
    HDC     hdc = GetWindowDC(demo_wnd);
    static  DWORD fps_time    = 0;
    static  DWORD pre_time    = 0;
    static  int   demo_frames = 0;
    int     i = 0;
    
#if (ENABLE_VSYNC == 1)
    /* This works great on NVidia, but not for ATI/Intel... So strange... */
    PFNWGLSWAPINTERVALEXTPROC p_swap_func = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (p_swap_func)
    {
        p_swap_func(ENABLE_VSYNC);
    }
#endif

    if (!(hrc = wglCreateContext(hdc)) || !wglMakeCurrent(hdc, hrc))
    {
        return 1;
    }

    while (i < RAIN_ATOMS)
    {
        rain_atoms[i++].state = 0;
    }

    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
    glViewport(0, 0, demo_w, demo_h);
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(hdc);

    /* SetThreadPriority(demo_thread, THREAD_PRIORITY_ABOVE_NORMAL); */
    Sleep(DEMO_DELAY);
    demo_time = 0;
    pre_time = fps_time = GetTickCount();

    while (midi_position != MIDI_FINISHED)
    {
        /* Calculate elapsed time */
        DWORD now = GetTickCount();
        DWORD delta = now - fps_time;

#if (LIMIT_FPS == TRUE)
        /* Frame rate limitation */
        while (pre_time == now)
        {
            /* Bad: spin lock.. */
            now = GetTickCount();
        }
#endif

#if (SHOW_FPS == TRUE && IS_FULLSCREEN == FALSE)
        /* This code will not go to release */
        if (now < fps_time)
        {
            delta = (0xFFFFFFFF - fps_time) + now;
        }

        if (delta > 1000)
        {
            int d = 1000;
            int p = 5;
            char fps[] = "FPS:     ";
            if (demo_frames > 10000) demo_frames = 10000;
            if (demo_frames > 0)    demo_frames--;
            while (d != 0)
            {
                fps[p++] = '0' + (char)(demo_frames / d);
                demo_frames -= (demo_frames / d) * d;
                d /= 10;
            }
            SetWindowText(demo_wnd, fps);
            demo_frames = 0;
            fps_time = now;
        }
#endif

        /* Time sync between MIDI and demo */

        /* delta is the time elapsed since the previous frame */
        delta = now - pre_time;
        if (now < pre_time)
        {
            delta = (0xFFFFFFFF - pre_time) + now;
        }

        /* demo_time is global demo clock which is sync'ed with MIDI */
        demo_time += delta;

        /* Now sync if we exceed midi_interval or note has been suddenly changed */
        if (demo_time < midi_time)
        {
            // Demo delays...
            demo_time = midi_time;
        }
        
        if (demo_time - midi_time > midi_interval)
        {
            unsigned int midi_pre_pos = midi_position;

            // MIDI delays... Skip frames until MIDI moves. "coz therez nothin' else to do."
            while (midi_pre_pos == midi_position)
            {
                Sleep(1);
            }

            demo_time = midi_time;
        }

        pre_time = GetTickCount();

        /* Go through all states */
        glClear(GL_COLOR_BUFFER_BIT);

        for (now = 0; demo_states[now].time_in != DEMO_END; now++)
        {
            if (demo_time >= demo_states[now].time_in &&
                demo_time <= demo_states[now].duration + demo_states[now].time_in)
            {
                float t = (float)(demo_time - demo_states[now].time_in) /
                          (float)(demo_states[now].duration);

                if (!demo_states[now].in_state)
                {
                    if (demo_states[now].proc_init != NULL)
                    {
                        demo_states[now].proc_init();
                    }

                    demo_states[now].in_state = 1;
                }

                demo_states[now].proc_run(t);
            }
            else
            {
                if (demo_states[now].in_state)
                {
                    /* Final pass with 1.0 */
                    demo_states[now].proc_run(1.0f);
                }

                demo_states[now].in_state = 0;
            }
        }

        glFlush();
        SwapBuffers(hdc);
        demo_frames++;
    };

    ReleaseDC(demo_wnd, hdc);
    SendMessage(demo_wnd, WM_KEYDOWN, 0, 0);

    demo_thread = NULL;
    return 0;
}

/*** Global functions ********************************************************/

/* Init demo subsystem. Return TRUE if successfull. */
BOOL demo_init(HWND hwnd, int base_w, int base_h)
{
    if (!hwnd || base_w < 32 || base_h < 32)
    {
        return FALSE;
    }

    demo_wnd = hwnd;
    demo_w   = base_w;
    demo_h   = base_h;

    return TRUE;
}

/* Stop demo and destroy the working thread. */
void demo_fini()
{
    demo_wnd = NULL;

    if (demo_thread != NULL)
    {
        TerminateThread(demo_thread, 0);
    }
}

/* Start demo in a dedicated thread. Return TRUE if successfull. */
BOOL demo_run()
{
    DWORD demo_thread_id;

    /* Starting in a dedicated thread */
    demo_thread = CreateThread(NULL, 0, &demo_proc, NULL, 0, &demo_thread_id);
    return (demo_thread != NULL);
}
