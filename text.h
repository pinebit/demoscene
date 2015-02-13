/*
 * pinebit 4k intro
 * (c) 2010 pinebit
 */

#ifndef PINE_TEXT_H
#define PINE_TEXT_H

#define TEXTURE_SX      512
#define TEXTURE_SY      128
#define TEXT_FONT       "Arial"

extern unsigned char text_luma[TEXTURE_SX * TEXTURE_SY];

/* Init text_luma[] array */
void text_init(void);

/* Render text into luminance texture: text_luma[] */
void text_render(const char *text);

#endif /* PINE_TEXT_H */

