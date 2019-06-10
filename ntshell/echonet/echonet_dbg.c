/*
 *  TOPPERS ECHONET Lite Communication Middleware
 * 
 *  Copyright (C) 2014 Cores Co., Ltd. Japan
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
 *  @(#) $Id$
 */

/*
 *		ECHONET Lite タスク デバッグ出力
 */

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include <sil.h>
#include <ctype.h>

#include "syssvc/serial.h"
#include "syssvc/syslog.h"

#include <netinet/in.h>
#include <netinet/in_itron.h>

#include "echonet.h"
#include "echonet_dbg.h"
#include "echonet_fbs.h"
#include "echonet_task.h"

#ifdef ECN_DBG_WAI_ENA
#define _ECN_DBG_WAI_SYSLOG _ecn_dbg_wai_syslog()
#else
#define _ECN_DBG_WAI_SYSLOG
#endif

#if defined(ECN_DBG_PUT_ENA) || defined(ECN_CAP_PUT_ENA)
#if defined(ECN_DBG_WAI_ENA)
static ER _ecn_dbg_wai_syslog(void);
/*
 *  syslog()バッファに余裕ができるまで待つ
 */
static ER _ecn_dbg_wai_syslog(void)
{
	T_SYSLOG_RLOG	a_rlog;
	ER				a_ret;
	int				a_ct = 0;
#ifndef ECN_DBG_WAI_SYSLOG_RETRY
 #ifdef LOGTASK
  #define ECN_DBG_WAI_SYSLOG_RETRY 10
 #else
  #define ECN_DBG_WAI_SYSLOG_RETRY 0
 #endif
#endif

	for (;;) {
		a_ret = syslog_ref_log(&a_rlog);
		if (a_ret < 0)
			return a_ret;
		if (1 <= a_rlog.count) {
			a_ct = 0;
		} else
#if (defined ECN_DBG_WAI_SYSLOG_RETRY) && (0 < ECN_DBG_WAI_SYSLOG_RETRY)
		if (ECN_DBG_WAI_SYSLOG_RETRY <= ++a_ct)
#endif
		{
/*			a_ret = dly_tsk(100);
			if (a_ret < 0 && a_ret != E_RLWAI)
				return a_ret; // */
			break;
		}

		a_ret = dly_tsk(1);
		if (a_ret < 0 && a_ret != E_RLWAI)
			return a_ret;
	}
	return 0;
}
#endif /* #if defined(ECN_DBG_WAI_ENA) */

/*
 * デバッグ出力
 * 例：		ECN_DBG_PUT("task start");
 */
int _ecn_dbg_dbgput(const char *fp_srcloc, const char *fp_form, ...)
{
	va_list v;
	char	*p_buf = 0;
	ID		tskid;
#ifdef ECN_DBG_PUT_USE_STATIC
 #ifndef SEM_ECN_DBG_PUT
	/* 256byteバッファ8本ローテーション */
	static char				a_buf[8][256];
	static volatile uint_t	a_buf_idx = 0;
	uint_t					a_buf_idx_a;
 #else
	/* セマフォを用いて、1本のバッファ上に文字列を並べて使う */
	static char				a_buf[1024];
	static volatile uint_t	a_buf_pos = 0;
	size_t					a_buf_len;
  #define ECN_DBG_PUT_BUF_MAX a_buf_len
 #endif /* #ifdef SEM_ECN_DBG_PUT */
#endif /* #ifdef ECN_DBG_PUT_USE_STATIC */
#ifndef ECN_DBG_PUT_BUF_MAX
 #define ECN_DBG_PUT_BUF_MAX 255
#endif

	get_tid(&tskid);

	if (!fp_srcloc)
		fp_srcloc = "(null)";

#ifdef SEM_ECN_DBG_PUT
	syscall(wai_sem(SEM_ECN_DBG_PUT));
#endif

#ifdef ECN_DBG_PUT_USE_STATIC
 #ifndef SEM_ECN_DBG_PUT
	a_buf_idx_a = ++a_buf_idx;
	p_buf = a_buf[a_buf_idx_a & 0x07];
 #else
	if (sizeof(a_buf) - a_buf_pos < 80)	/* 残りが80byte未満になったら、先頭に戻る */
		a_buf_pos = 0;
	p_buf = a_buf + a_buf_pos;
	a_buf_len = sizeof(a_buf) - a_buf_pos - 1;
	if (80 * 2 <= a_buf_len)	/* 80byte*2以上の余白があるうちは、末尾80byteを残す */
		a_buf_len -= 80;
 #endif /* #ifdef SEM_ECN_DBG_PUT */
#else
	p_buf = (char *)_ecn_fbs_dtq_get(255);
	if (!p_buf) {
		syslog(LOG_NOTICE, "%s[%d]: (+dtq fault) %s", fp_srcloc, (int)tskid, fp_form);
 #ifdef SEM_ECN_DBG_PUT
		syscall(sig_sem(SEM_ECN_DBG_PUT));
 #endif /* #ifdef SEM_ECN_DBG_PUT */
		return 1;
	}
#endif /* #ifdef ECN_DBG_PUT_USE_STATIC */

	va_start(v, fp_form);

#ifdef _MSC_VER
	vsnprintf_s(p_buf, ECN_DBG_PUT_BUF_MAX, ECN_DBG_PUT_BUF_MAX, fp_form, v);
#else
	vsprintf(p_buf, fp_form, v);
#endif
#ifdef ECN_DBG_PUT_USE_STATIC
 #ifdef SEM_ECN_DBG_PUT
	a_buf_len = strlen(p_buf) + 1;
	/* 残りが80byte未満になったら */
	if (sizeof(a_buf) - a_buf_pos - a_buf_len < 80) {
		/* 先頭に戻る */
		a_buf_pos = 0;
	} else {
		a_buf_pos += a_buf_len;
	}
 #endif /* #ifdef SEM_ECN_DBG_PUT */
#endif

	_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */

#ifdef SEM_ECN_DBG_PUT
	syscall(sig_sem(SEM_ECN_DBG_PUT));
#endif

	syslog(LOG_NOTICE, "%s[%d]: %s", fp_srcloc, (int)tskid, p_buf);

	va_end(v);

#ifndef ECN_DBG_PUT_USE_STATIC
	_ecn_fbs_dtq_rel(p_buf);
#endif
	return 0;
}

/* ECN_ENOD_IDの文字列変換 */
const char *_ecn_dbg_enod2str(ECN_ENOD_ID fa_enod_id)
{
/*	#define _ECN_FBS_DBG_ENOD2STR_BUF_MAX_BIT (4)
/*	*/
#ifdef _ECN_FBS_DBG_ENOD2STR_BUF_MAX_BIT
	static volatile uint8_t a_idx_vol = 0;
	static char a_buf[1 << _ECN_FBS_DBG_ENOD2STR_BUF_MAX_BIT][sizeof("ENOD_REMOTE_ID()")+5];
	uint8_t a_idx;
#endif

	if (fa_enod_id == ENOD_NOT_MATCH_ID)
		return "ENOD_NOT_MATCH_ID";
	if (fa_enod_id == ENOD_MULTICAST_ID)
		return "ENOD_MULTICAST_ID";
	if (fa_enod_id == ENOD_LOCAL_ID)
		return "ENOD_LOCAL_ID";
	if (fa_enod_id == ENOD_API_ID)
		return "ENOD_API_ID";
	if (fa_enod_id == ENOD_REMOTE_ID)
		return "ENOD_REMOTE_ID(0)";
	if (fa_enod_id == ((ENOD_REMOTE_ID) + 1))
		return "ENOD_REMOTE_ID(1)";
	if (fa_enod_id == ((ENOD_REMOTE_ID) + 2))
		return "ENOD_REMOTE_ID(2)";
	if (fa_enod_id == ((ENOD_REMOTE_ID) + 3))
		return "ENOD_REMOTE_ID(3)";
	if (fa_enod_id == ((ENOD_REMOTE_ID) + 4))
		return "ENOD_REMOTE_ID(4)";
	if (fa_enod_id == ((ENOD_REMOTE_ID) + 5))
		return "ENOD_REMOTE_ID(5)";
#ifdef _ECN_FBS_DBG_ENOD2STR_BUF_MAX_BIT
	if (ENOD_REMOTE_ID <= fa_enod_id) {
		/* return "ENOD_REMOTE_ID"; */
		a_idx = (++a_idx_vol) & ((1 << _ECN_FBS_DBG_ENOD2STR_BUF_MAX_BIT) - 1);
#ifdef _MSC_VER
		sprintf_s(a_buf[a_idx], sizeof(*a_buf), "ENOD_REMOTE_ID(%d)", fa_enod_id - ENOD_REMOTE_ID);
#else
		sprintf(a_buf[a_idx], "ENOD_REMOTE_ID(%d)", fa_enod_id - ENOD_REMOTE_ID);
#endif
		return a_buf[a_idx];
	}
#endif

	return "?";
}

/*
 * バイナリダンプ出力
 */
void _ecn_dbg_bindmp(const uint8_t *buffer, size_t len)
{
	/* static領域のバッファを切り替える個数(2^n) */
	#define _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT (4)
/*	*/
#ifdef SEM_ECN_DBG_BINDMP
 #ifdef _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT
  #undef _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT
  /* セマフォ(SEM_ECN_DBG_BINDMP)が定義されている場合、バッファ切り替えは不要なのでundef */
 #endif
#endif
#ifndef _ECN_FBS_DBG_BINDMP_BUFLEN
 /* バッファサイズ(bindmp中の1行が収まるサイズ) */
 #define _ECN_FBS_DBG_BINDMP_BUFLEN (80)
#endif
#ifdef _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT
	/* static領域のバッファを切り替えるための管理用変数 */
	static volatile uint8_t a_idx_vol = 0;
	/* static領域のバッファ切り替え領域 */
	static char a_buf_area[1 << _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT][_ECN_FBS_DBG_BINDMP_BUFLEN];
	uint8_t a_idx;
	char *a_buf;
#else
	/* static領域のバッファ領域 */
	static char a_buf[_ECN_FBS_DBG_BINDMP_BUFLEN];
#endif
	const uint8_t *p = buffer;			/* 読み取りポインタ */
	int i, a_blk, a_blk_max, a_pos = 0;
	const T_ECN_EDT_HDR *p_req_esv;
	const T_ECN_INTERNAL_MSG *p_im;

#ifdef SEM_ECN_DBG_BINDMP
	syscall(wai_sem(SEM_ECN_DBG_BINDMP));
#endif

	if (len != 256) {
		a_blk_max = 1;
	} else {
		a_blk_max = ((T_ECN_FST_BLK *)buffer)->hdr.wr.blk
						+ (((T_ECN_FST_BLK *)buffer)->hdr.wr.pos ? 1 : 0);
		if (!a_blk_max)
			a_blk_max = 1;
	}
	#ifndef ECN_DBG_BINDMP_MAXLEN
	 /* bindmp出力の最大サイズ */
	 #define ECN_DBG_BINDMP_MAXLEN 1600
	#endif
	if (ECN_DBG_BINDMP_MAXLEN < len) {
		len = ECN_DBG_BINDMP_MAXLEN;
	}

#ifdef _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT
	a_idx = (++a_idx_vol) & ((1 << _ECN_FBS_DBG_BINDMP_BUF_MAX_BIT) - 1);
	a_buf = a_buf_area[a_idx];
	/* #define a_buf (a_buf_area[a_idx]) */
#endif

	for (a_blk = 0; p && a_blk < a_blk_max; p = ((T_ECN_FST_BLK *)buffer)->lnk.p_sub[a_blk++]->dat) {
		if (0 < a_blk) {
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, "p_sub[%d] (0x%08X)", a_blk - 1, p);
		}
		memset(a_buf, 0, _ECN_FBS_DBG_BINDMP_BUFLEN);
		for (i = 0; i < (int)len; i++) {
			if (i % 16 == 0) {
				if (a_buf[0]) {
					_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
					syslog(LOG_NOTICE, "%s", a_buf);
				}
#ifdef _MSC_VER
				sprintf_s(a_buf, _ECN_FBS_DBG_BINDMP_BUFLEN, "[%08X]:", i);
#else
				sprintf(a_buf, "[%08X]:", i);
#endif
				a_pos = strlen(a_buf);
			} else {
				a_buf[a_pos++] = (i % 16 == 8 ? '-':' ');
			}
#ifdef _MSC_VER
			sprintf_s(a_buf + a_pos, _ECN_FBS_DBG_BINDMP_BUFLEN - a_pos, "%02X", p[i]);
#else
			sprintf(a_buf + a_pos, "%02X", p[i]);
#endif
			a_pos += 2;
		}
		if (a_buf[0]) {
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, "%s", a_buf);
		}
		if (a_blk_max == 1)
			break;
	}

	if (len == 256) {
		switch (((const T_ECN_FST_BLK *)buffer)->hdr.type) {
		case ECN_MSG_ECHONET:
			p_req_esv = &((const T_ECN_FST_BLK *)buffer)->d.t_esv;
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, " sender: %d %s, target: %d %s",
				((T_ECN_FST_BLK *)buffer)->hdr.sender, _ecn_dbg_enod2str(((T_ECN_FST_BLK *)buffer)->hdr.sender),
				((T_ECN_FST_BLK *)buffer)->hdr.target, _ecn_dbg_enod2str(((T_ECN_FST_BLK *)buffer)->hdr.target));
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, " ecn_hdr: 0x%02X 0x%02X 0x%04X", p_req_esv->ecn_hdr.ehd1, p_req_esv->ecn_hdr.ehd2, p_req_esv->ecn_hdr.tid);
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, " edata: 0x%06X 0x%06X 0x%02X 0x%02X (%s)",
				p_req_esv->edata.seoj.eojx1 << 16 | p_req_esv->edata.seoj.eojx2 << 8 | p_req_esv->edata.seoj.eojx3,
				p_req_esv->edata.deoj.eojx1 << 16 | p_req_esv->edata.deoj.eojx2 << 8 | p_req_esv->edata.deoj.eojx3,
				p_req_esv->edata.esv, p_req_esv->edata.opc, _ecn_dbg_esv2str(p_req_esv->edata.esv));
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, " ecn_prp: 0x%02X 0x%02X", p_req_esv->ecn_prp.epc, p_req_esv->ecn_prp.pdc);
			_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
			syslog(LOG_NOTICE, " cur(blk/pos): wr %d/%d, rd %d/%d",
				(int)((T_ECN_FST_BLK *)buffer)->hdr.wr.blk,
				(int)((T_ECN_FST_BLK *)buffer)->hdr.wr.pos,
				(int)((T_ECN_FST_BLK *)buffer)->hdr.rd.blk,
				(int)((T_ECN_FST_BLK *)buffer)->hdr.rd.pos);
		break;
		case ECN_MSG_INTERNAL:
			p_im = (const T_ECN_INTERNAL_MSG *)&((const T_ECN_FST_BLK *)buffer)->payload[0];
			syslog(LOG_NOTICE, " a_im.command: %d:%s", p_im->command,
				(p_im->command == ECN_INM_NOTIFY_INSTANCELIST ? "ECN_INM_NOTIFY_INSTANCELIST":
				(p_im->command == ECN_INM_RELEASE_WAIT ? "ECN_INM_RELEASE_WAIT" : "?")));
			memset(a_buf, 0, _ECN_FBS_DBG_BINDMP_BUFLEN);
			a_blk_max = ((const T_ECN_FST_BLK *)buffer)->hdr.wr.blk * 64
					+ ((const T_ECN_FST_BLK *)buffer)->hdr.wr.pos;
			if (0 < a_blk_max) {
				if (64 < a_blk_max)
					a_blk_max = 64;
				if (_ECN_FBS_DBG_BINDMP_BUFLEN < a_blk_max)
					a_blk_max = _ECN_FBS_DBG_BINDMP_BUFLEN;
				for (i = offsetof(T_ECN_INTERNAL_MSG, data); i < a_blk_max; i++) {
					a_blk = ((const T_ECN_FST_BLK *)buffer)->payload[i];
					if (!isprint(a_blk) || !isascii(a_blk))
						a_blk = '.';
					a_buf[i - 1] = a_blk;
				}
				syslog(LOG_NOTICE, " a_im.data: [%s]", &a_buf[offsetof(T_ECN_INTERNAL_MSG, data) - 1]);
			}
			break;
		default:
			syslog(LOG_NOTICE, " invalid type: %d", ((const T_ECN_FST_BLK *)buffer)->hdr.type);
		}
	}
	else if (8 <= len
	&& ((const T_ECN_FST_BLK *)buffer)->bin[0] == 0x10
	&& ((const T_ECN_FST_BLK *)buffer)->bin[1] == 0x81) {
		p_req_esv = (const T_ECN_EDT_HDR *)&((const T_ECN_FST_BLK *)buffer)->bin[0];
		_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
		syslog(LOG_NOTICE, " ecn_hdr: 0x%02X 0x%02X 0x%04X", p_req_esv->ecn_hdr.ehd1, p_req_esv->ecn_hdr.ehd2, p_req_esv->ecn_hdr.tid);
		_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
		syslog(LOG_NOTICE, " edata: 0x%06X 0x%06X 0x%02X 0x%02X (%s)",
			p_req_esv->edata.seoj.eojx1 << 16 | p_req_esv->edata.seoj.eojx2 << 8 | p_req_esv->edata.seoj.eojx3,
			p_req_esv->edata.deoj.eojx1 << 16 | p_req_esv->edata.deoj.eojx2 << 8 | p_req_esv->edata.deoj.eojx3,
			p_req_esv->edata.esv, p_req_esv->edata.opc, _ecn_dbg_esv2str(p_req_esv->edata.esv));
		_ECN_DBG_WAI_SYSLOG; /* syslog()バッファに余裕ができるまで待つ */
		syslog(LOG_NOTICE, " ecn_prp: 0x%02X 0x%02X", p_req_esv->ecn_prp.epc, p_req_esv->ecn_prp.pdc);
	}
#ifdef SEM_ECN_DBG_BINDMP
	syscall(sig_sem(SEM_ECN_DBG_BINDMP));
#endif
}

/*
 * ECHONET Liteサービスコード文字列変換
 */
const char *_ecn_dbg_esv2str(uint8_t fa_esv)
{
	switch (fa_esv) {
	case ESV_SET_I_SNA:		return "プロパティ値書き込み要求不可応答";			/* 0x50 */
	case ESV_SET_C_SNA:		return "プロパティ値書き込み要求不可応答";			/* 0x51	*/
	case ESV_GET_SNA:		return "プロパティ値読み出し不可応答";				/* 0x52	*/
	case ESV_INF_SNA:		return "プロパティ値通知不可応答";					/* 0x53 */
	case ESV_SET_GET_SNA:	return "プロパティ値書き込み・読み出し不可応答";	/* 0x5E	*/
	case ESV_SET_I:			return "プロパティ値書き込み要求（応答不要）";		/* 0x60 */
	case ESV_SET_C:			return "プロパティ値書き込み要求（応答要）";		/* 0x61 */
	case ESV_GET:			return "プロパティ値読み出し要求";					/* 0x62 */
	case ESV_INF_REQ:		return "プロパティ値通知要求";						/* 0x63 */
	case ESV_SET_GET:		return "プロパティ値書き込み・読み出し要求";		/* 0x6E */
	case ESV_SET_RES:		return "プロパティ値書き込み応答";					/* 0x71 */
	case ESV_GET_RES:		return "プロパティ値読み出し応答";					/* 0x72 */
	case ESV_INF:			return "プロパティ値通知";							/* 0x73 */
	case ESV_INFC:			return "プロパティ値通知（応答要）";				/* 0x74 */
	case ESV_SET_GET_RES:	return "プロパティ値書き込み・読み出し応答";		/* 0x7E */
	case ESV_INFC_RES:		return "プロパティ値通知応答";						/* 0x7A */
	default:
		return "(unknown)";
	}
}
#endif /* #if defined(ECN_DBG_PUT_ENA) || defined(ECN_CAP_PUT_ENA) */
