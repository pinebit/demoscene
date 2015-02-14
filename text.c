/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL\gl.h>
#include "config.h"
#include "text.h"

unsigned char text_luma[TEXTURE_SX * TEXTURE_SY];

void text_init(void)
{
    int i = TEXTURE_SX * TEXTURE_SY;
    while (--i >= 0) text_luma[i] = 0;
}

void text_render(const char *text)
{
    /* Create DC for painting into memory */
    BITMAPINFO bi;
    HBITMAP    temp_bitmap;
    BYTE*      p_bits = NULL;
    HDC        temp_dc = CreateCompatibleDC(GetDC(0));

    if (!temp_dc)
    {
        return;
    }

    bi.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biBitCount     = 24;
    bi.bmiHeader.biWidth        = TEXTURE_SX;
    bi.bmiHeader.biHeight       = TEXTURE_SY;
    bi.bmiHeader.biCompression  = BI_RGB;
    bi.bmiHeader.biPlanes       = 1;

    temp_bitmap = CreateDIBSection(temp_dc, &bi, DIB_RGB_COLORS, (void**)&p_bits, 0, 0);
    if (!temp_bitmap || !p_bits)
    {
        return;
    }

    if (SelectObject(temp_dc, temp_bitmap))
    {
        HFONT temp_font;
        RECT  rt;

        /* Clean up the surface with black color */
        rt.left     = 0;
        rt.top      = 0;
        rt.right    = TEXTURE_SX - 1;
        rt.bottom   = TEXTURE_SY - 1;
        FillRect(temp_dc, &rt, (HBRUSH)GetStockObject(BLACK_BRUSH));

        temp_font = CreateFont(TEXTURE_SY / 2, 0, 0, 0, FW_BOLD,
                               FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                               DRAFT_QUALITY, DEFAULT_PITCH, TEXT_FONT);

        /* It doesn't need to keep the previous font, since dc is private */
        if (SelectObject(temp_dc, temp_font))
        {
            int i = 0;

            SetTextColor(temp_dc, RGB(255, 255, 255));
            SetBkColor(temp_dc, RGB(0,0,0));
            DrawText(temp_dc, text, -1, &rt, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
            GdiFlush();

            while (i < TEXTURE_SX * TEXTURE_SY)
            {
                text_luma[i] = p_bits[i*3];
                i++;
            }
        }
    }

    DeleteObject(temp_bitmap);
    DeleteDC(temp_dc);
}
