/*
 * pinebit 4k intro
 * (c) 2010 pinebit
 */

#ifndef PINE_ATOM_H
#define PINE_ATOM_H

#define ATOM_SIZE       16

/* Vector */
typedef struct
{
    float   x;
    float   y;
    float   z;
    float   w;
} vector_t;

/* Color */
typedef struct
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} color_t;

/* Atom element */
typedef struct
{
    vector_t    position;
    color_t     color;
    float       angle;
    float       speed;
    int         state;
} atom_t;

/* Generate atom's texture */
unsigned int atom_gen_texture();

/* Sort atoms in according with distances */
void atom_sort(float *dist, int *items, int L, int R);

#endif /* PINE_ATOM_H */

