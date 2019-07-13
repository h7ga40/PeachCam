/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 * 
 *  Copyright (C) 2000-2003 by Embedded and Real-Time Systems Laboratory
 *                              Toyohashi Univ. of Technology, JAPAN
 *  Copyright (C) 2004-2011 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Information Science, Nagoya Univ., JAPAN
 *  Copyright (C) 2015-2019 Cores Co., Ltd. Japan
 *
 *  上記著作権者は，以下の(1)～(4)の条件を満たす場合に限り，本ソフトウェ
 *  ア（本ソフトウェアを改変したものを含む．以下同じ）を使用・複製・改
 *  変・再配布（以下，利用と呼ぶ）することを無償で許諾する．
 *  (1) 本ソフトウェアをソースコードの形で利用する場合には，上記の著作
 *      権表示，この利用条件および下記の無保証規定が，そのままの形でソー
 *      スコード中に含まれていること．
 *  (2) 本ソフトウェアを，ライブラリ形式など，他のソフトウェア開発に使
 *      用できる形で再配布する場合には，再配布に伴うドキュメント（利用
 *      者マニュアルなど）に，上記の著作権表示，この利用条件および下記
 *      の無保証規定を掲載すること．
 *  (3) 本ソフトウェアを，機器に組み込むなど，他のソフトウェア開発に使
 *      用できない形で再配布する場合には，次のいずれかの条件を満たすこ
 *      と．
 *    (a) 再配布に伴うドキュメント（利用者マニュアルなど）に，上記の著
 *        作権表示，この利用条件および下記の無保証規定を掲載すること．
 *    (b) 再配布の形態を，別に定める方法によって，TOPPERSプロジェクトに
 *        報告すること．
 *  (4) 本ソフトウェアの利用により直接的または間接的に生じるいかなる損
 *      害からも，上記著作権者およびTOPPERSプロジェクトを免責すること．
 *      また，本ソフトウェアのユーザまたはエンドユーザからのいかなる理
 *      由に基づく請求からも，上記著作権者およびTOPPERSプロジェクトを
 *      免責すること．
 *
 *  本ソフトウェアは，無保証で提供されているものである．上記著作権者お
 *  よびTOPPERSプロジェクトは，本ソフトウェアに関して，特定の使用目的
 *  に対する適合性も含めて，いかなる保証も行わない．また，本ソフトウェ
 *  アの利用により直接的または間接的に生じたいかなる損害に関しても，そ
 *  の責任を負わない．
 *
 *  @(#) $Id: fbs_string.c 1971 2019-07-10 04:36:59Z coas-nagasima $
 */

#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include "fbs_string.h"

/*
 *  数値を文字列に変換
 */
#define CONVERT_BUFLEN	((sizeof(uintptr_t) * CHAR_BIT + 2) / 3)
										/* uintptr_t型の数値の最大文字数 */
static ER
convert(ECN_FBS_ID fbs, ECN_FBS_SSIZE_T *pos, uintptr_t val, uint_t radix, const char *radchar,
			uint_t width, bool_t minus, bool_t padzero)
{
	char	buf[CONVERT_BUFLEN];
	uint_t	i, j;
	ER ret;

	i = 0U;
	do {
		buf[i++] = radchar[val % radix];
		val /= radix;
	} while (i < CONVERT_BUFLEN && val != 0);

	if (minus && width > 0) {
		width -= 1;
	}
	if (minus && padzero) {
		ret = _ecn_fbs_poke(fbs, *pos, '-');
		if(ret != E_OK)
			return ret;
		(*pos)++;
	}
	for (j = i; j < width; j++) {
		ret = _ecn_fbs_poke(fbs, *pos, padzero ? '0' : ' ');
		if(ret != E_OK)
			return ret;
		(*pos)++;
	}
	if (minus && !padzero) {
		ret = _ecn_fbs_poke(fbs, *pos, '-');
		if(ret != E_OK)
			return ret;
		(*pos)++;
	}
	while (i > 0U) {
		ret = _ecn_fbs_poke(fbs, *pos, buf[--i]);
		if(ret != E_OK)
			return ret;
		(*pos)++;
	}
	return E_OK;
}

/*
 *  文字列整形出力
 */
static const char raddec[] = "0123456789";
static const char radhex[] = "0123456789abcdef";
static const char radHEX[] = "0123456789ABCDEF";

ECN_FBS_SSIZE_T
fbs_printf(ECN_FBS_ID fbs, ECN_FBS_SSIZE_T *pos, const char *format, ...)
{
	char		c;
	bool_t		lflag;
	uint_t		width;
	bool_t		padzero;
	intptr_t	val;
	const char	*str, *head;
	va_list	ap;
	ER ret;

	va_start(ap, format);

	while ((c = *format++) != '\0') {
		if (c != '%') {
			ret = _ecn_fbs_poke(fbs, *pos, c);
			if (ret != E_OK)
				return ret;
			(*pos)++;
			continue;
		}

		lflag = false;
		width = 0U;
		padzero = false;
		if ((c = *format++) == '0') {
			padzero = true;
			c = *format++;
		}
		while ('0' <= c && c <= '9') {
			width = width * 10U + c - '0';
			c = *format++;
		}
		if (c == 'l') {
			lflag = true;
			c = *format++;
		}
		switch (c) {
		case 'd':
			val = lflag ? (intptr_t) va_arg(ap, long_t)
						: (intptr_t) va_arg(ap, int_t);
			if (val >= 0) {
				ret = convert(fbs, pos, (uintptr_t) val, 10U, raddec,
										width, false, padzero);
				if (ret != E_OK)
					return ret;
			}
			else {
				ret = convert(fbs, pos, (uintptr_t)(-val), 10U, raddec,
										width, true, padzero);
				if (ret != E_OK)
					return ret;
			}
			break;
		case 'u':
			val = lflag ? (intptr_t) va_arg(ap, ulong_t)
						: (intptr_t) va_arg(ap, uint_t);
			ret = convert(fbs, pos, (uintptr_t) val, 10U, raddec, width, false, padzero);
			if (ret != E_OK)
				return ret;
			break;
		case 'x':
		case 'p':
			val = lflag ? (intptr_t) va_arg(ap, ulong_t)
						: (intptr_t) va_arg(ap, uint_t);
			ret = convert(fbs, pos, (uintptr_t) val, 16U, radhex, width, false, padzero);
			if (ret != E_OK)
				return ret;
			break;
		case 'X':
			val = (intptr_t)va_arg(ap, void *);
			ret = convert(fbs, pos, (uintptr_t) val, 16U, radHEX, width, false, padzero);
			if (ret != E_OK)
				return ret;
			break;
		case 'c':
			ret = _ecn_fbs_poke(fbs, *pos, (char)(intptr_t)va_arg(ap, int));
			if (ret != E_OK)
				return ret;
			(*pos)++;
			break;
		case 's':
			str = head = (const char *)va_arg(ap, const char *);
			while ((c = *str++) != '\0') {
				ret = _ecn_fbs_poke(fbs, *pos, c);
				if (ret != E_OK)
					return ret;
				(*pos)++;
			}
			break;
		case '%':
			ret = _ecn_fbs_poke(fbs, *pos, '%');
			if (ret != E_OK)
				return ret;
			(*pos)++;
			break;
		case '\0':
			format--;
			break;
		default:
			break;
		}
	}
	va_end(ap);

	return E_OK;
}
