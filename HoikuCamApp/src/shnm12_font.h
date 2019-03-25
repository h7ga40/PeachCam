#ifndef SHNM16_FONT_H
#define SHNM16_FONT_H

#include <stdint.h>

#define FONT_HALF_WIDTH			6
#define FONT_WIDTH				12
#define FONT_HEIGHT				12

struct utf8_code_bitmap {
	const uint8_t code;
	const uint8_t bitmap[FONT_WIDTH * FONT_HEIGHT / 8];
};

#define UTF8_CODE_2B_1_NUM		6
#define UTF8_CODE_3B_1_NUM		9
#define UTF8_CODE_3B_2_NUM		343		/*半角追加2/2*/

#define UTF8_1B_CODE_BITMAP_NUM	256
#define UTF8_2B_CODE_BITMAP_NUM	122											/*"￢","＼","￠""￡"移動)*/
#define UTF8_3B_CODE_BITMAP_NUM	6757	/*半角追加1/2*/	/*半角追加2/2*/		/*"￢","＼","￠""￡"追加)*/

// 1バイトコード
extern const uint8_t UTF8_1B_CODE_BITMAP[UTF8_1B_CODE_BITMAP_NUM][FONT_HALF_WIDTH * FONT_HEIGHT / 8];

// 2バイトコード　1バイト目
extern const uint8_t Utf8CodeTable_2B_1st[UTF8_CODE_2B_1_NUM][2];
// 2バイトコード 2バイト目 bitmapデータ
extern const struct utf8_code_bitmap UTF8_2B_CODE_BITMAP[UTF8_2B_CODE_BITMAP_NUM];

// 3バイトコード 1バイト目
extern const uint8_t Utf8CodeTable_3B_1st[UTF8_CODE_3B_1_NUM][2];
// 3バイトコード 2バイト目
extern const uint8_t Utf8CodeTable_3B_2nd[UTF8_CODE_3B_2_NUM][2];

// 3バイトコード 3バイト目 bitmapデータ
extern const struct utf8_code_bitmap UTF8_3B_CODE_BITMAP[UTF8_3B_CODE_BITMAP_NUM];

#endif /* SHNM16_FONT_H */
