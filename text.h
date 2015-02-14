/*
* 4k intro (c) pinebit 2010
* (updated in 2015 for GitHub)
*/

#ifndef PINE_TEXT_H
#define PINE_TEXT_H

#define TEXTURE_SX      512
#define TEXTURE_SY      128
#define TEXT_FONT       "Verdana"

extern unsigned char text_luma[TEXTURE_SX * TEXTURE_SY];

/* Init text_luma[] array */
void text_init(void);

/* Render text into luminance texture: text_luma[] */
void text_render(const char *text);

#endif /* PINE_TEXT_H */

