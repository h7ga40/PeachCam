#ifndef DRAW_FONT_H
#define DRAW_FONT_H

#include "shnm12_font.h"

#define DISP_X_ADJ	0
#define DISP_Y_ADJ	0
#define X_ZENKAKU_CHARACTERS(hlcd)	((hlcd)->_width / FONT_WIDTH)
#define X_HANKAKU_CHARACTERS(hlcd)	((hlcd)->_width / FONT_HALF_WIDTH)
#define Y_CHARACTERS(hlcd)			((hlcd)->_height / FONT_HEIGHT)
#define X_LINE_TO_PIX(hlcd, x)		((FONT_WIDTH*(x))-DISP_X_ADJ)
#define X_LINE_HALF_TO_PIX(hlcd, x)	((FONT_HALF_WIDTH*(x))-DISP_X_ADJ)
#define Y_ROW_TO_PIX(hlcd, y)		((FONT_HEIGHT*(y))-DISP_Y_ADJ)

#ifdef __cplusplus
extern "C" {
#endif

void get_bitmap_font(const uint8_t *string, uint8_t *bitmap_data, uint32_t *use_chars);
void lcd_drawFont(uint8_t *bitmap_data, int x, int y, uint16_t color, uint16_t back_color);
void lcd_drawFontHalf(uint8_t *bitmap_data, int x, int y, uint16_t color, uint16_t back_color);
void lcd_drawString(const char *string, int x, int y, uint16_t color, uint16_t back_color );

#ifdef __cplusplus
}
#endif

#endif /* DRAW_FONT_H */
