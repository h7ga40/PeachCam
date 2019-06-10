/*
 *  TOPPERS WEBSOCKET Lite Communication Middleware
 * 
 *  Copyright (C) 2017 Cores Co., Ltd. Japan
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
 * 固定長バッファ可変長ストリーム
 */

#ifndef _WEBSOCKET_FBS_H_
#define _WEBSOCKET_FBS_H_

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEBSOCKET_MEMPOOL_GET_TMO
#define WEBSOCKET_MEMPOOL_GET_TMO	(0)			/* メモリプール取得時のタイムアウト指定 */
#endif /* WEBSOCKET_MEMPOOL_GET_TMO */

#ifndef NUM_WEBSOCKET_MEMPOOL_BLOCK
#define NUM_WEBSOCKET_MEMPOOL_BLOCK (60)		/* 電文用メモリブロックの数 */
#endif /* NUM_WEBSOCKET_MEMPOOL_BLOCK */

#ifndef WEBSOCKET_MEMPOOL_BLOCK_SIZE
#define WEBSOCKET_MEMPOOL_BLOCK_SIZE (256)	/* 電文用メモリブロックのサイズ */
#endif /* WEBSOCKET_MEMPOOL_BLOCK_SIZE */

typedef unsigned int WS_FBS_SIZE_T;	/* size_t相当  */
typedef int WS_FBS_SSIZE_T;			/* ssize_t相当 */

/* T_WS_FST_BLK管理領域サイズ */
#define DEF_WS_EDT_HDR_LEN (sizeof(intptr_t))

/* 管理領域サイズ(64byte - sizeof(T_MSG)) */
/* #define DEF_WS_FBS_LIB_HDR_LEN ((WEBSOCKET_MEMPOOL_BLOCK_SIZE + sizeof(int) - 1)>> 2) // */
#define DEF_WS_FBS_LIB_HDR_LEN (64 - DEF_WS_EDT_HDR_LEN)

/* 管理領域を含むブロックに保持するデータサイズ */
#define DEF_WS_FBS_FST_DAT_LEN (64)

/* リンクポインタ配列のサイズ(byte) */
#define DEF_WS_FBS_LNK_LEN (WEBSOCKET_MEMPOOL_BLOCK_SIZE - DEF_WS_EDT_HDR_LEN - DEF_WS_FBS_LIB_HDR_LEN - DEF_WS_FBS_FST_DAT_LEN)

/* 固定長バッファ最大サイズ */
#define DEF_WS_FBS_BUF_MAXLEN ((DEF_WS_FBS_LNK_LEN / sizeof(void *)) * WEBSOCKET_MEMPOOL_BLOCK_SIZE + DEF_WS_FBS_FST_DAT_LEN)

/* 子要素バッファのサイズ */
#define DEF_WS_FBS_SUB_BUF_LEN WEBSOCKET_MEMPOOL_BLOCK_SIZE

/* 電文の種別(内容は仮) */
typedef enum
{
	WS_FBS_TYPE_BLANK		=	0,		/* 未使用ストリーム */
	WS_FBS_TYPE_INTERNAL	=	1,		/* 内部メッセージ */
	WS_FBS_TYPE_WEBSOCKET	=	2		/* WEBSOCKET電文 */
} TA_WS_MSG_TYPE;

/* リンクポインタ配列に列挙される、子要素バッファ */
typedef struct
{
	uint8_t	payload[WEBSOCKET_MEMPOOL_BLOCK_SIZE];
} T_WS_SUB_BLK;

/* リンクポインタ配列 */
typedef union
{
	T_WS_SUB_BLK	*p_sub[DEF_WS_FBS_LNK_LEN / sizeof(T_WS_SUB_BLK *)];
	uint8_t			bin[DEF_WS_FBS_LNK_LEN];
} T_WS_FBS_LNK;

/* 管理領域(64byte - sizeof(T_MSG)) */
typedef struct
{
	uint16_t	length;			/* メッセージ長 */
	uint16_t	type;			/* メッセージタイプ */
	union{
		ID mbxid;				/* 送信元メールボックスID（内部メッセージ） */
	}sender;
	union{
		ID mbxid;				/* 送信先メールボックスID（内部メッセージ） */
	}target;
	union{
		ID mbxid;				/* 返信先メールボックスID（内部メッセージ） */
	}reply;

	int			rd;				/*	読み取りヘッド情報	*/
	int			wr;				/*	書き込みヘッド情報	*/
} T_WS_FBS_HDR;

/* 管理領域を含むブロックの構造 */
typedef struct
{
	uint8_t	_msg[DEF_WS_EDT_HDR_LEN];	/* T_MSG */
	T_WS_FBS_HDR	hdr;	/*	管理領域  64byte  - sizeof(T_MSG)	*/
	uint8_t			_gap[DEF_WS_FBS_LIB_HDR_LEN - sizeof(T_WS_FBS_HDR)];
	T_WS_FBS_LNK	lnk;	/*	リンクポインタ配列	128byte			*/
	uint8_t			payload[DEF_WS_FBS_FST_DAT_LEN];
} T_WS_FST_BLK;

/* FBS-ID */
typedef struct ws_fbs_id_strc
{
	T_WS_FST_BLK	*ptr;
} WS_FBS_ID;

/* メッセージキュー */
typedef struct ws_fbs_queue
{
	T_WS_FST_BLK	*pk_head;		/* 先頭のメッセージ */
	T_WS_FST_BLK	*pk_last;		/* 末尾のメッセージ */
} T_WS_FBS_QUEUE;

extern ID ws_mempoolid;

/* メモリブロック取得 ok:ポインタ NG:NULL */
void *_ws_fbs_mbx_get(WS_FBS_SIZE_T fa_req_size);

/* メモリブロック解放 ok:0 NG:非0 */
ER _ws_fbs_mbx_rel(void *p);

/*
 * 領域確保
 *	引数
 *		WS_FBS_SIZE_T	fa_req_size		要求バッファサイズ(byte)
 *										max: DEF_WS_FBS_BUF_MAXLEN
 *		WS_FBS_ID		*fp_id			確保したFBS-IDの格納先
 *	戻り値
 *		ER				0:ok 非0:NG
 */
ER _ws_fbs_cre(WS_FBS_SIZE_T fa_req_size, WS_FBS_ID *fp_id);

/*
 * 領域解放
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_del(WS_FBS_ID fa_id);

/*
 * 保持データの有無
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *	戻り値
 *		bool_t			0:無し, 1:あり
 */
bool_t _ws_fbs_exist_data(WS_FBS_ID fa_id);

/*
 * 保持データ長の取得
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *	戻り値
 *		WS_FBS_SSIZE_T	保持しているデータの長さ(byte) -1:NG
 */
WS_FBS_SSIZE_T _ws_fbs_get_datalen(WS_FBS_ID fa_id);

/*
 * 読み取りカーソルの位置取得
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *	戻り値
 *		WS_FBS_SIZE_T	先頭からの絶対位置 -1:NG
 */
WS_FBS_SSIZE_T _ws_fbs_get_rpos(WS_FBS_ID fa_id);

/*
 * 読み取りカーソルの位置設定
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *		WS_FBS_SIZE_T	fa_pos	設定する位置(先頭からの絶対位置)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_set_rpos(WS_FBS_ID fa_id, WS_FBS_SSIZE_T fa_pos);

/*
 * 読み取りカーソルの位置移動
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *		WS_FBS_SIZE_T	fa_seek	移動量(現状、前進のみ)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_seek_rpos(WS_FBS_ID fa_id, WS_FBS_SSIZE_T fa_seek);

/*
 * 任意指定位置の1byte読み取り
 */
int _ws_fbs_peek(WS_FBS_ID fa_id, WS_FBS_SSIZE_T fa_seek);

/*
 * 任意指定位置の1byte書き込み
 */
ER _ws_fbs_poke(WS_FBS_ID fa_id, WS_FBS_SSIZE_T fa_seek, int fa_val);

/*
 * データ追加
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *		const void *	fa_dat	追加するデータ
 *		WS_FBS_SSIZE_T	fa_len	追加する長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_add_data(WS_FBS_ID fa_id, const void *fa_dat, WS_FBS_SSIZE_T fa_len);

/*
 * データ追加(領域を自動的に拡張する)
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *		const void *	fa_dat	追加するデータ
 *		WS_FBS_SSIZE_T	fa_len	追加する長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_add_data_ex(WS_FBS_ID fa_id, const void *fa_dat, WS_FBS_SSIZE_T fa_len);

/*
 * データ取得
 *	引数
 *		ws_fbs_id型	確保したFBS-ID
 *		void *			fa_buf		取得するバッファ
 *		WS_FBS_SSIZE_T	fa_maxlen	取得する長さ(byte)
 *		WS_FBS_SSIZE_T	*p_len		取得した長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ws_fbs_get_data(WS_FBS_ID fa_id, void *fa_buf, WS_FBS_SSIZE_T fa_maxlen,
	WS_FBS_SSIZE_T *p_len);

ER ws_fbs_enqueue(T_WS_FBS_QUEUE *pk_queue, T_WS_FST_BLK *pk_buf);
ER ws_fbs_dequeue(T_WS_FBS_QUEUE *pk_queue, T_WS_FST_BLK **ppk_buf);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef _WEBSOCKET_FBS_H_ */
