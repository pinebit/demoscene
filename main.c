/*
 * pinebit 4k intro
 * (c) 2010 pinebit
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL\gl.h>
#include "config.h"
#include "demo.h"
#include "midi.h"

/* Windows specific handlers */
static HDC         h_dc  = NULL;
static HGLRC       h_rc  = NULL;
static HWND        h_wnd = NULL;
static HINSTANCE   h_instance;
static BOOL        is_fullscreen = IS_FULLSCREEN;

/* Main window proc function */
LRESULT CALLBACK wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
        case WM_KEYDOWN: PostQuitMessage(0); return 0;
    }

    // Pass All Unhandled Messages To DefWindowProc
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/* Demo window creation: winapi + opengl */
static BOOL create_window()
{
    UINT      pixel_format;
    WNDCLASS  w_class;
    DWORD     w_ex_style;
    DWORD     w_style;
    RECT      w_rect;

    static PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0,
        PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    const int base_w = is_fullscreen ? GetSystemMetrics(SM_CXFULLSCREEN) : DW_WIDTH;
    const int base_h = is_fullscreen ? GetSystemMetrics(SM_CYFULLSCREEN) : DW_HEIGHT;
    w_rect.left   = 0;
    w_rect.right  = base_w;
    w_rect.top    = 0;
    w_rect.bottom = base_h;

    h_instance             = GetModuleHandle(NULL);
    w_class.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    w_class.lpfnWndProc    = (WNDPROC)wnd_proc;
    w_class.cbClsExtra     = 0;
    w_class.cbWndExtra     = 0;
    w_class.hInstance      = h_instance;
    w_class.hIcon          = NULL;
    w_class.hCursor        = NULL;
    w_class.hbrBackground  = NULL;
    w_class.lpszMenuName   = NULL;
    w_class.lpszClassName  = RS_CLASS;

    if (!RegisterClass(&w_class)) return FALSE;

    if (is_fullscreen)
    {
        w_ex_style = WS_EX_APPWINDOW | WS_EX_TOPMOST;
        w_style    = WS_POPUP;
        ShowCursor(FALSE);
    }
    else
    {
        w_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        w_style    = WS_DLGFRAME | WS_SYSMENU;
    }

    AdjustWindowRectEx(&w_rect, w_style, FALSE, w_ex_style);

    // Create The Window
    if (!(h_wnd = CreateWindowEx(w_ex_style, RS_CLASS, RS_CAPTION,
                                 w_style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                 0, 0,
                                 w_rect.right - w_rect.left,
                                 w_rect.bottom - w_rect.top,
                                 NULL, NULL, h_instance, NULL ))) return FALSE;
    
    if (!(h_dc = GetDC(h_wnd)) ||
        !(pixel_format = ChoosePixelFormat(h_dc, &pfd)) ||
        !SetPixelFormat(h_dc, pixel_format, &pfd)) return FALSE;

    ShowWindow(h_wnd, is_fullscreen ? SW_SHOWMAXIMIZED : SW_SHOW);
    SetForegroundWindow(h_wnd);
    SetFocus(h_wnd);

    return demo_init(h_wnd, base_w, base_h);
}

/* Entry point */
void WINAPI WinMainCRTStartup()
{
    MSG msg;

    /* Init subsystems */
    if (!create_window())
    {
        MessageBox(NULL, "OpenGL error", RS_CAPTION, MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    if (!midi_init())
    {
        MessageBox(NULL, "MIDI error", RS_CAPTION, MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    /* Start midi and demo in dedicated threads */
    if (!midi_play() || !demo_run())
    {
        demo_fini();
        midi_fini();
        MessageBox(NULL, "Demo failed to run", RS_CAPTION, MB_OK | MB_ICONERROR);
        ExitProcess(2);
    }

    /* Thread #0 - processing Windows messages with blocking on queue */
    while (GetMessage(&msg, NULL, 0, 0) && midi_position != MIDI_FINISHED)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    demo_fini();
    midi_fini();
    ExitProcess(0);
}

/* TODO: remove in release. needed for ms linker. */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WinMainCRTStartup();
    return 0;
}
