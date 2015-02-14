/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL\gl.h>
#include "atom.h"

#define ATOM_SIZE_2     (ATOM_SIZE / 2)

unsigned int atom_gen_texture()
{
    color_t particle[ATOM_SIZE][ATOM_SIZE];
    int     x;
    int     y;
    GLuint  tid;
    color_t white = { 255, 255, 255, 255 };

    for (y = 0; y < ATOM_SIZE; ++y)
    {
        for (x = 0; x < ATOM_SIZE; ++x)
        {
            int dx = (x - ATOM_SIZE_2) * (x - ATOM_SIZE_2);
            int dy = (y - ATOM_SIZE_2) * (y - ATOM_SIZE_2);
            int v = ATOM_SIZE_2 * ATOM_SIZE_2 - (dx + dy) - 16;

            if (v < 0) v = 0;
            v <<= 2;

            particle[x][y] = white;
            particle[x][y].a = (unsigned char)v;
        }
    }

    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, ATOM_SIZE, ATOM_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, particle);

    return tid;
}

void atom_sort(float *dist, int *items, int L, int R)
{
    int   m = (L + R) >> 1;
    float mediana = dist[m];
    int   LL = L;
    int   RR = R;

    while (LL < RR)
    {
        while (dist[LL] < mediana) LL++;
        while (dist[RR] > mediana) RR--;

        if (LL <= RR)
        {
            int   temp_i = items[LL];
            float temp_d = dist[LL];

            dist[LL] = dist[RR];
            dist[RR] = temp_d;

            items[LL] = items[RR];
            items[RR] = temp_i;

            LL++;
            RR--;
        }
    }
    if (LL<R)   atom_sort(dist, items, LL, R);
    if (RR > L) atom_sort(dist, items, L, RR);
}
