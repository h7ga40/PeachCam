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
 * 固定長バッファ可変長ストリーム
 */

#include <kernel.h>
#include <string.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include "echonet_fbs.h"
#ifdef ECHONET_USE_MALLOC
#include <stdlib.h>
#endif

static ER get_message(T_ECN_FST_BLK **ppk_msg, int size)
{
	void * blk;
#ifndef ECHONET_USE_MALLOC
	ER ret;
#endif

	*ppk_msg = NULL;

	if ((size <= 0) || (size > DEF_ECN_FBS_BUF_MAXLEN))
		return E_PAR;

#ifndef ECHONET_USE_MALLOC
	ret = tget_mpf(ecn_mempoolid, &blk, ECHONET_MEMPOOL_GET_TMO);
	if (ret != E_OK)
		return ret;
#else
	blk = malloc(ECHONET_MEMPOOL_BLOCK_SIZE);
	if (blk == NULL)
		return E_NOMEM;
#endif

	memset(blk, 0, ECHONET_MEMPOOL_BLOCK_SIZE);
	*ppk_msg = (T_ECN_FST_BLK *)(blk);
	(*ppk_msg)->hdr.length = (uint16_t)size;

	return E_OK;
}

static void * get_block(T_ECN_FST_BLK *pk_msg, int pos, bool_t exp, int *size)
{
#ifndef ECHONET_USE_MALLOC
	ER ret;
#endif
	void * buf;
	void * blk;
	int no;
	T_ECN_SUB_BLK *mblk;
	int temp;

	if ((pos < 0) || (!exp && (pos >= pk_msg->hdr.length))) {
		return NULL;
	}

	if (pos < DEF_ECN_FBS_FST_DAT_LEN) {
		buf = (void *)&pk_msg->payload[pos];
		temp = DEF_ECN_FBS_FST_DAT_LEN - pos;
		if (temp < *size) {
			*size = temp;
		}
	}
	else {
		pos -= DEF_ECN_FBS_FST_DAT_LEN;
		no = pos / DEF_ECN_FBS_SUB_BUF_LEN;

		mblk = pk_msg->lnk.p_sub[no];

		pos -= no * DEF_ECN_FBS_SUB_BUF_LEN;
		temp = DEF_ECN_FBS_SUB_BUF_LEN - pos;
		if (temp < *size) {
			*size = temp;
		}
		if (exp) {
			if (mblk == NULL) {
#ifndef ECHONET_USE_MALLOC
				ret = get_mpf(ecn_mempoolid, &blk);
				if (ret != E_OK)
					return NULL;
#else
				blk = malloc(ECHONET_MEMPOOL_BLOCK_SIZE);
				if (blk == NULL)
					return NULL;
#endif
				mblk = (T_ECN_SUB_BLK *)(blk);

				pk_msg->lnk.p_sub[no] = mblk;
			}

			temp = pos + *size + no * DEF_ECN_FBS_SUB_BUF_LEN + DEF_ECN_FBS_FST_DAT_LEN;
			if (pk_msg->hdr.length < temp) {
				pk_msg->hdr.length = (uint16_t)temp;
			}
		}

		buf = &mblk->payload[pos];
	}

	return buf;
}

static ER release_message(T_ECN_FST_BLK *pk_msg)
{
#ifndef ECHONET_USE_MALLOC
	ER ret;
#endif
	int i;

	for (i = 0; i < 32; i++) {
		void * blk = pk_msg->lnk.p_sub[i];
		if (blk == NULL)
			continue;

#ifndef ECHONET_USE_MALLOC
		ret = rel_mpf(ecn_mempoolid, blk);
		if (ret != E_OK)
			return ret;
#else
		free(blk);
#endif
	}

#ifndef ECHONET_USE_MALLOC
	return rel_mpf(ecn_mempoolid, pk_msg);
#else
	free(pk_msg);

	return E_OK;
#endif
}

static int read_message(T_ECN_FST_BLK *pk_msg, int pos, void * dst, int size)
{
	int len, rest = size;
	void * buf;

	if (size <= 0)
		return 0;

	len = rest;
	buf = get_block(pk_msg, pos, false, &len);
	if ((buf == NULL) || (len <= 0))
		return 0;

	if((pos + len) > pk_msg->hdr.length)
		len = pk_msg->hdr.length - pos;

	for (; ; ) {
		memcpy(dst, buf, len);
		dst = (void *)((intptr_t)dst + len);
		rest -= len;
		pos += len;

		if (rest <= 0)
			break;

		len = rest;
		buf = get_block(pk_msg, pos, false, &len);
		if ((buf == NULL) || (len <= 0))
			return size - rest;
	}

	return size;
}

static int write_message(const void * src, T_ECN_FST_BLK *pk_msg, int pos, int size)
{
	int len, rest = size;
	void * buf;

	if (size <= 0)
		return 0;

	len = rest;
	buf = get_block(pk_msg, pos, true, &len);
	if ((buf == NULL) || (len <= 0))
		return 0;

	for (; ; ) {
		memcpy(buf, src, len);
		src = (void *)((intptr_t)src +len);
		rest -= len;
		pos += len;

		if (rest <= 0)
			break;

		len = rest;
		buf = get_block(pk_msg, pos, true, &len);
		if ((buf == NULL) || (len <= 0))
			return size - rest;
	}

	if (pk_msg->hdr.length < pos)
		pk_msg->hdr.length = (uint16_t)pos;

	return size;
}

static int copy_message(T_ECN_FST_BLK *pk_src, int spos, T_ECN_FST_BLK *pk_dst, int dpos, int size)
{
	int dlen, slen, len, rest = size;
	void * dbuf, *sbuf;

	if (size <= 0)
		return 0;

	dlen = rest;
	dbuf = get_block(pk_dst, dpos, true, &dlen);
	if (dbuf == NULL)
		return 0;

	slen = rest;
	sbuf = get_block(pk_src, spos, false, &slen);
	if (sbuf == NULL)
		return 0;

	for (; ; ) {
		len = (dlen < slen) ? dlen : slen;

		if (len == 0)
			return size - rest;

		memcpy(dbuf, sbuf, len);

		dpos += len;
		spos += len;
		rest -= len;

		if (rest <= 0)
			break;

		dlen = rest;
		dbuf = get_block(pk_dst, dpos, true, &dlen);
		if (dbuf == NULL)
			return size - rest;

		slen = rest;
		sbuf = get_block(pk_src, spos, false, &slen);
		if (sbuf == NULL)
			return size - rest;
	}

	return size;
}

/* メモリブロック取得 ok:ポインタ NG:NULL */
void *_ecn_fbs_dtq_get(ECN_FBS_SIZE_T fa_req_size)
{
	void *result;
#ifndef ECHONET_USE_MALLOC
	ER ret;
#endif

	if ((fa_req_size <= 0) || (fa_req_size > ECHONET_MEMPOOL_BLOCK_SIZE))
		return NULL;

#ifndef ECHONET_USE_MALLOC
	ret = get_mpf(ecn_mempoolid, &result);
	if (ret != E_OK)
		return NULL;
#else
	result = malloc(ECHONET_MEMPOOL_BLOCK_SIZE);
	if (result == NULL)
		return NULL;
#endif

	memset(result, 0, ECHONET_MEMPOOL_BLOCK_SIZE);

	return result;
}

/* メモリブロック解放 */
ER _ecn_fbs_dtq_rel(void *p)
{
#ifndef ECHONET_USE_MALLOC
	return rel_mpf(ecn_mempoolid, p);
#else
	free(p);

	return E_OK;
#endif
}

/* 領域確保 */
ER _ecn_fbs_cre(ECN_FBS_SIZE_T fa_req_size, ECN_FBS_ID *fp_id)
{
	return get_message(&fp_id->ptr, fa_req_size);
}

/* 領域解放 */
ER _ecn_fbs_del(ECN_FBS_ID fa_id)
{
	return release_message(fa_id.ptr);
}

/* 保持データの有無 */
bool_t _ecn_fbs_exist_data(ECN_FBS_ID fa_id)
{
	return fa_id.ptr->hdr.length > fa_id.ptr->hdr.wr;
}

/* 保持データ長の取得 */
ECN_FBS_SSIZE_T _ecn_fbs_get_datalen(ECN_FBS_ID fa_id)
{
	return fa_id.ptr->hdr.length;
}

/* 読み取りカーソルの位置取得 */
ECN_FBS_SSIZE_T _ecn_fbs_get_rpos(ECN_FBS_ID fa_id)
{
	return fa_id.ptr->hdr.rd;
}

/* 読み取りカーソルの位置設定 */
ER _ecn_fbs_set_rpos(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_pos)
{
	if (fa_id.ptr->hdr.length <= (unsigned int)fa_pos)	/* 位置指定が大きすぎる */
		return E_PAR;

	fa_id.ptr->hdr.rd = fa_pos;

	return E_OK;
}

/* 読み取りカーソルの位置移動 */
ER _ecn_fbs_seek_rpos(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek)
{
	fa_id.ptr->hdr.rd += fa_seek;
	if (fa_id.ptr->hdr.rd > fa_id.ptr->hdr.length)
		fa_id.ptr->hdr.rd = fa_id.ptr->hdr.length;

	return E_OK;
}

/* 任意指定位置の1byte読み取り */
int _ecn_fbs_peek(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek)
{
	uint8_t result = 0;
	int ret;

	ret = read_message(fa_id.ptr, fa_seek, &result, 1);

	return (ret == 1) ? result : -1;
}

/* 任意指定位置の1byte書き込み */
ER _ecn_fbs_poke(ECN_FBS_ID fa_id, ECN_FBS_SSIZE_T fa_seek, int fa_val)
{
	uint8_t result = fa_val;
	int ret;

	ret = write_message(&result, fa_id.ptr, fa_seek, 1);

	return (ret == 1) ? E_OK : E_PAR;
}

/* データ追加 */
ER _ecn_fbs_add_data(ECN_FBS_ID fa_id, const void *fa_dat, ECN_FBS_SSIZE_T fa_len)
{
	if((fa_id.ptr->hdr.wr + fa_len) > fa_id.ptr->hdr.length)
		return E_PAR;

	fa_id.ptr->hdr.wr += write_message(fa_dat, fa_id.ptr, fa_id.ptr->hdr.wr, fa_len);
	return E_OK;
}

/* データ追加(領域を自動的に拡張する) */
ER _ecn_fbs_add_data_ex(ECN_FBS_ID fa_id, const void *fa_dat, ECN_FBS_SSIZE_T fa_len)
{
	fa_id.ptr->hdr.wr += write_message(fa_dat, fa_id.ptr, fa_id.ptr->hdr.wr, fa_len);
	return E_OK;
}

/* データ取得 */
ER _ecn_fbs_get_data(ECN_FBS_ID fa_id, void *fa_buf, ECN_FBS_SSIZE_T fa_maxlen, ECN_FBS_SSIZE_T *p_len)
{
	int ret;

	ret = read_message(fa_id.ptr, fa_id.ptr->hdr.rd, fa_buf, fa_maxlen);
	fa_id.ptr->hdr.rd += ret;
	*p_len = ret;

	return E_OK;
}

ER ecn_fbs_enqueue(T_ECN_FBS_QUEUE *pk_queue, T_ECN_FST_BLK *pk_buf)
{
	loc_cpu();

	*((T_ECN_FST_BLK **)pk_buf->_msg) = NULL;
	if (pk_queue->pk_head != NULL) {
		*((T_ECN_FST_BLK **)pk_queue->pk_last->_msg) = pk_buf;
	}
	else {
		pk_queue->pk_head = pk_buf;
	}
	pk_queue->pk_last = pk_buf;

	unl_cpu();

	return E_OK;
}

ER ecn_fbs_dequeue(T_ECN_FBS_QUEUE *pk_queue, T_ECN_FST_BLK **ppk_buf)
{
	ER ercd;

	loc_cpu();

	if (pk_queue->pk_head != NULL) {
		*ppk_buf = pk_queue->pk_head;
		pk_queue->pk_head = *((T_ECN_FST_BLK **)(*ppk_buf)->_msg);
		ercd = E_OK;
	}
	else {
		ercd = E_TMOUT;
	}

	unl_cpu();

	return ercd;
}
