#include "mbed.h"
#include "draw_font.h"

void get_bitmap_font(const uint8_t *string, uint8_t *bitmap_data, uint32_t *use_chars)
{
	uint32_t len, code;
	uint8_t i, j, k;
	uint32_t totalj, totalk;

	*use_chars = 0;
	len = 0;
	if ((string[0] & 0x80) == 0) { len = 1; }
	else if ((string[0] & 0xE0) == 0xC0) { len = 2; }
	else if ((string[0] & 0xF0) == 0xE0) { len = 3; }
	else if ((string[0] & 0xF8) == 0xF0) { len = 4; }
	else { return; }

	j = k = totalj = totalk = 0;

	if (len == 1) {
		code = string[0];
		memcpy(bitmap_data, &UTF8_1B_CODE_BITMAP[code][0], FONT_WIDTH * FONT_HEIGHT / 8);
		*use_chars = 1;
		return;
	}

	if (len == 2) {
		code = string[0];
		// 1バイト目サーチ
		for (i = 0; i < UTF8_CODE_2B_1_NUM; i++) {
			if (Utf8CodeTable_2B_1st[i][0] == code) {
				code = string[1];
				for (j = 0; j < Utf8CodeTable_2B_1st[i][1]; j++) {
					if (UTF8_2B_CODE_BITMAP[totalk].code == code) {
						memcpy(bitmap_data, UTF8_2B_CODE_BITMAP[totalk].bitmap, FONT_WIDTH * FONT_HEIGHT / 8);
						*use_chars = 2;
						return;
					}
					totalk++;
				}
			}
			else {
				totalk += Utf8CodeTable_2B_1st[i][1];
			}
		}
		return;
	}

	if (len == 3) {
		code = string[0];
		// 1バイト目サーチ
		for (i = 0; i < UTF8_CODE_3B_1_NUM; i++) {
			if (Utf8CodeTable_3B_1st[i][0] == code) {
				code = string[1];
				// 2バイト目サーチ
				for (j = 0; j < Utf8CodeTable_3B_1st[i][1]; j++) {
					if (Utf8CodeTable_3B_2nd[totalj][0] == code) {
						code = string[2];
						// 3バイト目サーチ
						for (k = 0; k < Utf8CodeTable_3B_2nd[totalj][1]; k++) {
							if (UTF8_3B_CODE_BITMAP[totalk].code == code) {
								memcpy(bitmap_data, UTF8_3B_CODE_BITMAP[totalk].bitmap, FONT_WIDTH * FONT_HEIGHT / 8);
								*use_chars = 3;
								return;
							}
							totalk++;
						}
						return;
					}
					else {/*読み飛ばすbitmap個数を蓄積*/
						totalk += Utf8CodeTable_3B_2nd[totalj][1];
					}
					totalj++;
				}
			}
			else {/*読み飛ばすbitmap個数を蓄積*/
				for (j = 0; j < Utf8CodeTable_3B_1st[i][1]; j++) {
					totalk += Utf8CodeTable_3B_2nd[totalj][1];
					totalj++;
				}
			}
		}
		return;
	}
}

void lcd_drawPixel(LCD_Handler_t *hlcd, int16_t x, int16_t y, uint16_t color)
{
	uint16_t *p_bottom_left_pos = (uint16_t *)& hlcd->_buffer[0];
	p_bottom_left_pos[x + y * hlcd->_width] = color;
}

void lcd_drawFastVLine(LCD_Handler_t *hlcd, int16_t x, int16_t y,
	int16_t h, uint16_t color)
{
	uint16_t *p_bottom_left_pos = (uint16_t *)& hlcd->_buffer[0];
	uint16_t *p_frame_buf;
	int i;

	p_frame_buf = &p_bottom_left_pos[x + y * hlcd->_width];

	for (i = 0; i < h; i++, p_frame_buf += hlcd->_width) {
		*p_frame_buf = color;
	}
}

void lcd_drawFastHLine(LCD_Handler_t *hlcd, int16_t x, int16_t y,
	int16_t w, uint16_t color)
{
	uint16_t *p_bottom_left_pos = (uint16_t *)& hlcd->_buffer[0];
	uint16_t *p_frame_buf;
	int i;

	p_frame_buf = &p_bottom_left_pos[x + y * hlcd->_width];

	for (i = 0; i < w; i++, p_frame_buf++) {
		*p_frame_buf = color;
	}
}

void lcd_drawFont(LCD_Handler_t *hlcd, uint8_t *bitmap_data, int x, int y, uint16_t color, uint16_t back_color)
{
	uint16_t * p_bottom_left_pos = (uint16_t *)&hlcd->_buffer[0];
	uint16_t * p_frame_buf;
	int i, j, b;
	uint8_t *bitmap = bitmap_data;

	b = 0x80;
	p_frame_buf = &p_bottom_left_pos[x + y * hlcd->_width];

	for (i = 0; i < FONT_HEIGHT; i++, p_frame_buf += hlcd->_width - FONT_WIDTH) {
		for (j = 0; j < FONT_WIDTH; j++, p_frame_buf++) {
			if ((*bitmap & b) != 0) {
				*p_frame_buf = color;
			}
			else {
				*p_frame_buf = back_color;
			}
			b >>= 1;
			if (b == 0) {
				b = 0x80;
				bitmap++;
			}
		}
	}
}

void lcd_drawFontHalf(LCD_Handler_t *hlcd, uint8_t *bitmap_data, int x, int y, uint16_t color, uint16_t back_color)
{
	uint16_t * p_bottom_left_pos = (uint16_t *)&hlcd->_buffer[0];
	uint16_t * p_frame_buf;
	int i, j, b;
	uint8_t *bitmap = bitmap_data;

	b = 0x80;
	p_frame_buf = &p_bottom_left_pos[x + y * hlcd->_width];

	for (i = 0; i < FONT_HEIGHT; i++, p_frame_buf += hlcd->_width - FONT_HALF_WIDTH) {
		for (j = 0; j < FONT_HALF_WIDTH; j++, p_frame_buf++) {
			if ((*bitmap & b) != 0) {
				*p_frame_buf = color;
			}
			else {
				*p_frame_buf = back_color;
			}
			b >>= 1;
			if (b == 0) {
				b = 0x80;
				bitmap++;
			}
		}
	}
}

void lcd_drawString(LCD_Handler_t *hlcd, const char *string, int x, int y, uint16_t color, uint16_t back_color)
{
	uint32_t current_top, use_chars, for_3B_hankaku_code;
	uint8_t bitmap_data[FONT_WIDTH * FONT_HEIGHT / 8], ctrl_code;
	int len = strlen(string);
	const uint8_t *code = (const uint8_t *)string;

	hlcd->cursor_x = x;
	hlcd->cursor_y = y;

	current_top = 0;
	while (current_top < len) {
		memset(bitmap_data, 0x0, FONT_WIDTH * FONT_HEIGHT / 8);
		ctrl_code = code[current_top];
		get_bitmap_font(&code[current_top], bitmap_data, &use_chars);
		if (use_chars == 0)
			return;

		//3バイトコード半角文字用
		if (use_chars == 3) {
			for_3B_hankaku_code = 0;
			for_3B_hankaku_code = ((code[current_top] << 16) |
				(code[current_top + 1] << 8) |
				(code[current_top + 2]));
		}

		current_top += use_chars;

		//1バイトコード半角文字
		if (use_chars == 1) {
			if (ctrl_code == 0x0D) { // CR
				hlcd->cursor_x = X_LINE_TO_PIX(hlcd, 0);
				continue;
			}
			if (ctrl_code == 0x0A) { // LF
				hlcd->cursor_y = hlcd->cursor_y + FONT_HEIGHT;
				continue;
			}

			if (hlcd->cursor_x + FONT_HALF_WIDTH > hlcd->_width) {
				hlcd->cursor_x = X_LINE_HALF_TO_PIX(hlcd, 0);
				hlcd->cursor_y = hlcd->cursor_y + FONT_HEIGHT;
			}
			lcd_drawFontHalf(hlcd, bitmap_data, hlcd->cursor_x, hlcd->cursor_y, color, back_color);
			hlcd->cursor_x += FONT_HALF_WIDTH;
			continue;
		}

		//3バイトコード半角文字
		if (use_chars == 3) {
			if (((0xEFBDA1 <= for_3B_hankaku_code) && (for_3B_hankaku_code <= 0xEFBDBF)) ||
				((0xEFBE80 <= for_3B_hankaku_code) && (for_3B_hankaku_code <= 0xEFBE9F))) {
				//3バイトコード半角文字
				if (hlcd->cursor_x + FONT_HALF_WIDTH > hlcd->_width) {
					hlcd->cursor_x = X_LINE_HALF_TO_PIX(hlcd, 0);
					hlcd->cursor_y = hlcd->cursor_y + FONT_HEIGHT;
				}
				lcd_drawFontHalf(hlcd, bitmap_data, hlcd->cursor_x, hlcd->cursor_y, color, back_color);
				hlcd->cursor_x += FONT_HALF_WIDTH;
				continue;
			}
		}

		//全角文字
		if (hlcd->cursor_x + FONT_WIDTH > hlcd->_width) {
			hlcd->cursor_x = X_LINE_TO_PIX(hlcd, 0);
			hlcd->cursor_y = hlcd->cursor_y + FONT_HEIGHT;
		}
		lcd_drawFont(hlcd, bitmap_data, hlcd->cursor_x, hlcd->cursor_y, color, back_color);
		hlcd->cursor_x += FONT_WIDTH;
	}
}
