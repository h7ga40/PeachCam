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
 *  @(#) $Id: echonet_fbs.h 1484 2018-03-30 12:24:59Z coas-nagasima $
 */

/*
 * 固定長バッファ可変長ストリーム
 */

#ifndef TOPPERS_ECHONET_FBS_H
#define TOPPERS_ECHONET_FBS_H

#include <kernel.h>
#include "echonet.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ECHONET_MEMPOOL_GET_TMO
#define ECHONET_MEMPOOL_GET_TMO	(0)			/* メモリプール取得時のタイムアウト指定 */
#endif /* ECHONET_MEMPOOL_GET_TMO */

#ifndef NUM_ECHONET_MEMPOOL_BLOCK
#define NUM_ECHONET_MEMPOOL_BLOCK (60)		/* 電文用メモリブロックの数 */
#endif /* NUM_ECHONET_MEMPOOL_BLOCK */

#ifndef ECHONET_MEMPOOL_BLOCK_SIZE
#define ECHONET_MEMPOOL_BLOCK_SIZE (256)	/* 電文用メモリブロックのサイズ */
#endif /* ECHONET_MEMPOOL_BLOCK_SIZE */

typedef unsigned int ECN_FBS_SIZE_T;	/* size_t相当  */
typedef int ECN_FBS_SSIZE_T;			/* ssize_t相当 */

/* T_EDATA管理領域サイズ */
#ifndef T_MSG
typedef intptr_t T_MSG;
#endif
#define DEF_ECN_EDT_HDR_LEN (sizeof(T_MSG))

/* 管理領域サイズ(64byte - sizeof(T_MSG)) */
/* #define DEF_ECN_FBS_LIB_HDR_LEN ((ECHONET_MEMPOOL_BLOCK_SIZE + sizeof(int) - 1)>> 2) // */
#define DEF_ECN_FBS_LIB_HDR_LEN (64 - DEF_ECN_EDT_HDR_LEN)

/* 管理領域を含むブロックに保持するデータサイズ */
#define DEF_ECN_FBS_FST_DAT_LEN (64)

/* リンクポインタ配列のサイズ(byte) */
#define DEF_ECN_FBS_LNK_LEN (ECHONET_MEMPOOL_BLOCK_SIZE - DEF_ECN_EDT_HDR_LEN - DEF_ECN_FBS_LIB_HDR_LEN - DEF_ECN_FBS_FST_DAT_LEN)

/* 固定長バッファ最大サイズ */
#define DEF_ECN_FBS_BUF_MAXLEN ((DEF_ECN_FBS_LNK_LEN / sizeof(void *)) * ECHONET_MEMPOOL_BLOCK_SIZE + DEF_ECN_FBS_FST_DAT_LEN)

/* 子要素バッファのサイズ */
#define DEF_ECN_FBS_SUB_BUF_LEN ECHONET_MEMPOOL_BLOCK_SIZE

/* 電文の種別(内容は仮) */
typedef enum
{
	ECN_FBS_TYPE_BLANK		=	0,		/* 未使用ストリーム */
	ECN_FBS_TYPE_INTERNAL	=	1,		/* 内部メッセージ */
	ECN_FBS_TYPE_ECHONET	=	2		/* ECHONET電文 */
} TA_ECN_MSG_TYPE;

/* リンクポインタ配列に列挙される、子要素バッファ */
typedef struct
{
	uint8_t	payload[ECHONET_MEMPOOL_BLOCK_SIZE];
} T_ECN_SUB_BLK;

/* リンクポインタ配列 */
typedef union
{
	T_ECN_SUB_BLK	*p_sub[DEF_ECN_FBS_LNK_LEN / sizeof(T_ECN_SUB_BLK *)];
	uint8_t			bin[DEF_ECN_FBS_LNK_LEN];
} T_ECN_FBS_LNK;

/* 管理領域(64byte - sizeof(T_MSG)) */
typedef struct
{
	uint16_t	length;			/* メッセージ長 */
	uint16_t	type;			/* メッセージタイプ */
	union{
		ECN_ENOD_ID id;			/* 送信元ノードID（ECHONET電文） */
		ID dtqid;				/* 送信元メールボックスID（内部メッセージ） */
	}sender;
	union{
		ECN_ENOD_ID id;			/* 送信先ノードID（ECHONET電文） */
		ID dtqid;				/* 送信先メールボックスID（内部メッセージ） */
	}target;
	union{
		ECN_ENOD_ID id;			/* 返信先ノードID（ECHONET電文） */
		ID dtqid;				/* 返信先メールボックスID（内部メッセージ） */
	}reply;

	int			rd;				/*	読み取りヘッド情報	*/
	int			wr;				/*	書き込みヘッド情報	*/
} T_ECN_FBS_HDR;

/* 管理領域を含むブロックの構造 */
typedef struct
{
	uint8_t	_msg[DEF_ECN_EDT_HDR_LEN];	/* T_MSG */
	T_ECN_FBS_HDR	hdr;	/*	管理領域  64byte  - sizeof(T_MSG)	*/
	uint8_t			_gap[DEF_ECN_FBS_LIB_HDR_LEN - sizeof(T_ECN_FBS_HDR)];
	T_ECN_FBS_LNK	lnk;	/*	リンクポインタ配列	128byte			*/
	uint8_t			payload[DEF_ECN_FBS_FST_DAT_LEN];
} T_ECN_FST_BLK;

/* FBS-ID */
typedef struct ecn_fbs_id_strc
{
	T_ECN_FST_BLK	*ptr;
} ECN_FBS_ID;

/* メッセージキュー */
typedef struct ecn_fbs_queue
{
	T_ECN_FST_BLK	*pk_head;		/* 先頭のメッセージ */
	T_ECN_FST_BLK	*pk_last;		/* 末尾のメッセージ */
} T_ECN_FBS_QUEUE;

/* メモリブロック取得 ok:ポインタ NG:NULL */
void *_ecn_fbs_dtq_get(ECN_FBS_SIZE_T fa_req_size);

/* メモリブロック解放 ok:0 NG:非0 */
ER _ecn_fbs_dtq_rel(void *p);

/*
 * 領域確保
 *	引数
 *		ECN_FBS_SIZE_T	fa_req_size		要求バッファサイズ(byte)
 *										max: DEF_ECN_FBS_BUF_MAXLEN
 *		ECN_FBS_ID		*fp_id			確保したFBS-IDの格納先
 *	戻り値
 *		ER				0:ok 非0:NG
 */
ER _ecn_fbs_cre(ECN_FBS_SIZE_T fa_req_size, ECN_FBS_ID *fp_id);

/*
 * 領域解放
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_del(ECN_FBS_ID fa_id);

/*
 * 保持データの有無
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *	戻り値
 *		bool_t			0:無し, 1:あり
 */
bool_t _ecn_fbs_exist_data(ECN_FBS_ID fa_id);

/*
 * 保持データ長の取得
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *	戻り値
 *		ECN_FBS_SSIZE_T	保持しているデータの長さ(byte) -1:NG
 */
ECN_FBS_SSIZE_T _ecn_fbs_get_datalen(ECN_FBS_ID fa_id);

/*
 * 読み取りカーソルの位置取得
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *	戻り値
 *		ECN_FBS_SIZE_T	先頭からの絶対位置 -1:NG
 */
ECN_FBS_SSIZE_T _ecn_fbs_get_rpos(ECN_FBS_ID fa_id);

/*
 * 読み取りカーソルの位置設定
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *		ECN_FBS_SIZE_T	fa_pos	設定する位置(先頭からの絶対位置)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_set_rpos(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_pos);

/*
 * 読み取りカーソルの位置移動
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *		ECN_FBS_SIZE_T	fa_seek	移動量(現状、前進のみ)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_seek_rpos(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek);

/*
 * 任意指定位置の1byte読み取り
 */
int _ecn_fbs_peek(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek);

/*
 * 任意指定位置の1byte書き込み
 */
ER _ecn_fbs_poke(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek, int fa_val);

/*
 * データ追加
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *		const void *	fa_dat	追加するデータ
 *		ECN_FBS_SSIZE_T	fa_len	追加する長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_add_data(ECN_FBS_ID fa_id, const void *fa_dat, ECN_FBS_SSIZE_T fa_len);

/*
 * データ追加(領域を自動的に拡張する)
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *		const void *	fa_dat	追加するデータ
 *		ECN_FBS_SSIZE_T	fa_len	追加する長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_add_data_ex(ECN_FBS_ID fa_id, const void *fa_dat, ECN_FBS_SSIZE_T fa_len);

/*
 * データ取得
 *	引数
 *		ecn_fbs_id型	確保したFBS-ID
 *		void *			fa_buf		取得するバッファ
 *		ECN_FBS_SSIZE_T	fa_maxlen	取得する長さ(byte)
 *		ECN_FBS_SSIZE_T	*p_len		取得した長さ(byte)
 *	戻り値
 *		ER				0:ok, 非0:NG
 */
ER _ecn_fbs_get_data(ECN_FBS_ID fa_id, void *fa_buf, ECN_FBS_SSIZE_T fa_maxlen,
	ECN_FBS_SSIZE_T *p_len);

ER ecn_fbs_enqueue(T_ECN_FBS_QUEUE *pk_queue, T_ECN_FST_BLK *pk_buf);
ER ecn_fbs_dequeue(T_ECN_FBS_QUEUE *pk_queue, T_ECN_FST_BLK **ppk_buf);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* #ifndef TOPPERS_ECHONET_FBS_H */
