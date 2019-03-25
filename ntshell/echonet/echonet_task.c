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
 *  @(#) $Id: echonet_task.c 1698 2018-11-13 09:55:37Z coas-nagasima $
 */

/*
 *		ECHONET Lite タスク
 */

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <t_syslog.h>
#include <t_stdlib.h>

#include "syssvc/serial.h"
#include "syssvc/syslog.h"

#include "echonet.h"
#include "echonet_fbs.h"
#include "echonet_task.h"
#include "echonet_dbg.h"
#ifdef ECHONET_CONTROLLER_EXTENTION
#include "echonet_agent.h"
#endif

#ifndef ECHONET_TASK_GET_TIMER
#define ECHONET_TASK_GET_TIMER	TMO_FEVR
#endif /* ECHONET_TASK_GET_TIMER */

#ifndef ECHONET_TASK_PROGRESS
#define ECHONET_TASK_PROGRESS(timer)
#endif /* ECHONET_TASK_PROGRESS */

#ifndef ECHONET_TASK_TIMEOUT
#define ECHONET_TASK_TIMEOUT
#endif /* ECHONET_TASK_TIMEOUT */

/* アプリケーションが要求した電文のシーケンス番号 */
static uint16_t g_api_tid;

/* 受信メッセージを開放するか否か */
static bool_t g_release_esv;

ER _ecn_tsk_snd_dtq(ECN_FBS_ID fa_rsp_fbs, bool_t from_app)
{
	ER	a_ret;
	T_MSG *msg = (T_MSG *)fa_rsp_fbs.ptr;
	ID dtqid = ecn_udp_dataqueueid;
	int i;
#ifdef ECN_DBG_PUT_ENA
	intptr_t a_fbs_sub1 = (intptr_t)fa_rsp_fbs.ptr->lnk.p_sub[0];
#endif
	if (from_app && (((T_EDATA *)fa_rsp_fbs.ptr)->hdr.edata.esv == ESV_INFC)) {
		g_api_tid = ((T_EDATA *)fa_rsp_fbs.ptr)->hdr.ecn_hdr.tid;
		dtqid = ecn_svc_dataqueueid;
	}
	else {
		switch (fa_rsp_fbs.ptr->hdr.target.id) {
		case ENOD_MULTICAST_ID:
			if (from_app)
				g_api_tid = ((T_EDATA *)fa_rsp_fbs.ptr)->hdr.ecn_hdr.tid;

			dtqid = ecn_udp_dataqueueid;
			break;
		case ENOD_LOCAL_ID:
			if (from_app)
				dtqid = ecn_svc_dataqueueid;
			else
				dtqid = ecn_api_dataqueueid;
			break;
		case ENOD_API_ID:
			dtqid = ecn_api_dataqueueid;
			break;
		default:
			if (from_app)
				g_api_tid = ((T_EDATA *)fa_rsp_fbs.ptr)->hdr.ecn_hdr.tid;

			i = fa_rsp_fbs.ptr->hdr.target.id;
			if (i < ENOD_REMOTE_ID || tnum_enodadr <= i)
				return E_NOEXS;
			i += - ENOD_REMOTE_ID + 1;
			if (from_app && (i < tnum_enodid)) {
				/* 非同期のリモートノードはサービス処理タスクで処理する */
				switch (eobjcb_table[i].profile->eobjatr) {
				case EOBJ_SYNC_REMOTE_NODE:
					dtqid = ecn_udp_dataqueueid;
					break;
				case EOBJ_ASYNC_REMOTE_NODE:
					dtqid = ecn_svc_dataqueueid;
					break;
				default:
					return E_SYS;
				}
			}
			else {
				dtqid = ecn_udp_dataqueueid;
			}
			break;
		}
	}
#ifdef ECN_DBG_PUT_ENA
	ECN_DBG_PUT("snd_dtq(%d, 0x%08X-0x%08X)", dtqid, (intptr_t)msg, a_fbs_sub1);
	_ecn_dbg_bindmp((const uint8_t *)msg, 256);
#endif
	a_ret = snd_dtq(dtqid, (intptr_t)msg);
#ifdef ECN_DBG_PUT_ENA
	ECN_DBG_PUT("snd_dtq(%d, 0x%08X-0x%08X) result = %d:%s", dtqid, (intptr_t)msg, a_fbs_sub1, a_ret, itron_strerror(a_ret));
#endif
	return a_ret;
}

static void _ecn_tsk_int_module_init(intptr_t fa_exinf);
static void _ecn_tsk_int_startup(intptr_t fa_exinf);
static uint16_t _ecn_tsk_new_tid(void);
static void _ecn_tsk_int_msg(intptr_t fa_exinf, ECN_FBS_ID fa_fbs_id);
static void _ecn_tsk_ecn_msg(intptr_t fa_exinf, ECN_FBS_ID fa_fbs_id);

static void _ecn_tsk_eoj_set(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp, bool_t fa_update,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv,
	ECN_FBS_ID *fa_fbs_anno);
static void _ecn_tsk_eoj_get(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_forward, ATR fa_access,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv);
static void _ecn_tsk_eoj_res(ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv,
	ECN_SRV_CODE fa_sna_esv);
static void _ecn_tsk_eoj_set_get(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv,
	ECN_FBS_ID *fa_fbs_anno);
static void _ecn_tsk_eoj_set_get_res(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp,
	ECN_FBS_ID fa_fbs_id, ECN_FBS_ID *fa_fbs_anno);

/* モジュール初期化フラグ */
static uint8_t		m_eobjlist_need_init = 1;

/* シーケンス番号 */
static uint16_t		g_current_tid = 1;

/*
 *  ECHONET Lite タスクの初期化（コンストラクタ相当）
 */
void echonet_task_init(intptr_t exinf)
{
	if (m_eobjlist_need_init)
		_ecn_tsk_int_module_init(exinf);
}

/*
 * モジュール初期化
 */
static void _ecn_tsk_int_module_init(intptr_t exinf)
{
	ID	tskid;

	if (!m_eobjlist_need_init)
		return;

	get_tid(&tskid);
	/* ECN_DBG_PUT("[ECHONET MainTask:%d] started", tskid); */
	syslog(LOG_NOTICE, "[ECHONET MainTask:%d] started", tskid);

	m_eobjlist_need_init = 0;

#ifdef ECHONET_CONTROLLER_EXTENTION
	ecn_agent_init();
#endif
}

/*
 *  ECHONET Lite タスクの本体
 */
void echonet_task(intptr_t fa_exinf)
{
	ER a_ret, a_ret2;
	union
	{
		T_MSG		*p_msg;
		ECN_FBS_ID	fbs_id;
	} a_mdt;
	SYSTIM a_prev, a_now;
	int a_timer;
#ifdef ECHONET_CONTROLLER_EXTENTION
	int a_timera;
#endif

	_ecn_tsk_int_startup(fa_exinf);

	a_ret2 = get_tim(&a_now);
	if (a_ret2 != E_OK) {
		ECN_DBG_PUT_2("get_tim() result = %d:%s", a_ret2, itron_strerror(a_ret2));
		return;
	}

	/* メッセージループ */
	for (;;) {
		a_prev = a_now;

		a_timer = ECHONET_TASK_GET_TIMER;
#ifdef ECHONET_CONTROLLER_EXTENTION
		a_timera = ecn_agent_get_timer();
		if(a_timer == TMO_FEVR){
			a_timer = a_timera;
		}
		else if((a_timera != TMO_FEVR) && (a_timera < a_timer)){
			a_timer = a_timera;
		}
#endif

		a_ret = trcv_dtq(ecn_svc_dataqueueid, (intptr_t *)&a_mdt.p_msg, a_timer);
		if ((a_ret != E_OK) && (a_ret != E_TMOUT)) {
			ECN_DBG_PUT_2("trcv_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
			break;
		}

		a_ret2 = get_tim(&a_now);
		if (a_ret2 != E_OK) {
			ECN_DBG_PUT_2("get_tim() result = %d:%s", a_ret2, itron_strerror(a_ret2));
			break;
		}

		a_timer = a_now - a_prev;
		ECHONET_TASK_PROGRESS(a_timer);
#ifdef ECHONET_CONTROLLER_EXTENTION
		ecn_agent_progress(a_timer);
#endif

		if (a_ret == E_OK) {
			ECN_DBG_PUT_1("trcv_dtq() dtq recv (%d byte)", _ecn_fbs_get_datalen(a_mdt.fbs_id));
			g_release_esv = true;

			switch (a_mdt.fbs_id.ptr->hdr.type) {
			/* 内部使用メッセージ */
			case ECN_MSG_INTERNAL:
				_ecn_tsk_int_msg(fa_exinf, a_mdt.fbs_id);
				break;

			/* ECHONET用メッセージ */
			case ECN_MSG_ECHONET:
				_ecn_tsk_ecn_msg(fa_exinf, a_mdt.fbs_id);
				break;

			default:
				syslog(LOG_WARNING, "echonet_task() a_fbs_id.ptr->k.hdr.k.t_edt.type:0x%02X undefined.", a_mdt.fbs_id.ptr->hdr.type);
			}

			/* 領域解放 */
			if (g_release_esv)
				_ecn_fbs_del(a_mdt.fbs_id);
		}

		ECHONET_TASK_TIMEOUT;
#ifdef ECHONET_CONTROLLER_EXTENTION
		ecn_agent_timeout();
#endif
	}
}

/*
 * シーケンス番号生成
 */
static uint16_t _ecn_tsk_new_tid(void)
{
	return g_current_tid++;
}

/*
 * 要求電文作成
 * 引数
 * ECN_FBS_ID		*fp_fbs_id		取得したFBS_IDの格納先
 * ID				fa_seoj			送信元のECHONETオブジェクトID
 * ID				fa_deoj			宛先のECHONETオブジェクトID
 * uint8_t			fa_epc			プロパティコード
 * uint8_t			fa_pdc			プロパティ値データサイズ
 * const void		*p_edt			プロパティ値データ
 * ECN_SRV_CODE		fa_esv			ECHONET Light サービスコード
 */
ER _ecn_tsk_mk_esv(ECN_FBS_ID *fp_fbs_id, ID fa_seoj, ID fa_deoj,
	uint8_t fa_epc, uint8_t fa_pdc, const void *p_edt, ECN_SRV_CODE fa_esv)
{
	ER				a_ret;
	int				a_size, i;
	ECN_FBS_ID		a_fbs_id = { 0 };	/* 要求電文用メモリ				*/
	T_ECN_EDT_HDR	a_ecn_hdp;			/* ecn_hdr+edata+ecn_prp 14byte	*/
	ID				a_enodid;
	const EOBJINIB *a_eobj;
	const EOBJINIB *a_enod;

	if (!fp_fbs_id)
		return E_PAR; /* 取得したFBS_IDの格納先がNULL */
	if (ECHONET_MEMPOOL_BLOCK_SIZE <= fa_pdc)
		return E_PAR; /* プロパティ値サイズが大きすぎる */
	if (!p_edt && 0 < fa_pdc)
		return E_PAR; /* プロパティ値サイズが1以上なのにデータポインタがNULL */

	if (fa_seoj <= EOBJ_NULL || tmax_eobjid < fa_seoj)
		return E_NOEXS; /* ECHONETオブジェクトIDが未定義 */
#ifndef ECHONET_CONTROLLER_EXTENTION
	if (fa_deoj < EOBJ_NULL || tmax_eobjid < fa_deoj)
		return E_NOEXS; /* ECHONETオブジェクトIDが未定義 */
#else
	if (fa_deoj < EOBJ_NULL || (tmax_eobjid + TNUM_AEOBJID) < fa_deoj)
		return E_NOEXS; /* ECHONETオブジェクトIDが未定義 */
#endif
	if (fa_deoj == EOBJ_NULL && fa_esv == ESV_INFC)
		return E_NOEXS; /* ECHONETオブジェクトIDが未定義 */

	/* 要求最小サイズの取得 */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 要求電文用メモリの取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_id);
	if (a_ret != E_OK || !a_fbs_id.ptr) { /* 確保失敗 */
		ECN_DBG_PUT_3("_ecn_fbs_cre(%d) result = %d:%s",
			a_size,
			a_ret, itron_strerror(a_ret));
		return E_NOMEM;
	}

	/* 要求電文設定						*/
	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));
	a_ecn_hdp.ecn_hdr.ehd1 = ECN_EDH1_ECHONET_LITE;
	a_ecn_hdp.ecn_hdr.ehd2 = ECN_EDH2_FORMAT_1;
	a_ecn_hdp.ecn_hdr.tid = _ecn_tsk_new_tid();
	memcpy(&a_ecn_hdp.edata.seoj, &eobjinib_table[fa_seoj - 1].eojx1, sizeof(a_ecn_hdp.edata.seoj));
	a_ecn_hdp.edata.esv = fa_esv;
	a_ecn_hdp.edata.opc = 1;		/* 処理プロパティ数 */
	a_ecn_hdp.ecn_prp.epc = fa_epc;	/* プロパティコード */
	a_ecn_hdp.ecn_prp.pdc = fa_pdc;	/* 付随データサイズ */

	/* 要求電文用メモリにデータ追加 */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_id, &a_ecn_hdp, a_size);
	if (a_ret) {
		ECN_DBG_PUT_5("_ecn_fbs_add_data_ex(*, ecn_hdp{esv:0x%02X,epc:0x%02X}, %d) result = %d:%s",
			a_ecn_hdp.edata.esv, a_ecn_hdp.ecn_prp.epc, a_size,
			a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	if (0 < fa_pdc) {
		/* 付随データ追加 */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_id, p_edt, fa_pdc);
		if (a_ret) {
			ECN_DBG_PUT_5("_ecn_fbs_add_data_ex(*, ecn_hdp{esv:0x%02X,epc:0x%02X} edt, %d) result = %d:%s",
				a_ecn_hdp.edata.esv, a_ecn_hdp.ecn_prp.epc, fa_pdc,
				a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
	}
	if (fa_deoj == 0) {
		ECN_DBG_PUT("マルチキャスト");
		/* fa_deoj == 0 : マルチキャスト */
		((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx1 = EOJ_X1_PROFILE;
		((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx2 = EOJ_X2_NODE_PROFILE;
		((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx3 = 0x01;
		a_fbs_id.ptr->hdr.target.id = ENOD_MULTICAST_ID;
	}
	else if (fa_deoj <= tmax_eobjid) {
		ECN_DBG_PUT_1("fa_deoj = %d", fa_deoj);
		/* if (fa_deoj < 1 || tmax_eobjid < fa_deoj) …の異常系は関数冒頭で除外済みとする */
		a_eobj = &eobjinib_table[fa_deoj - 1];
		memcpy(&((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj, &a_eobj->eojx1,
			sizeof(((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj));

		/* テーブルから検索 */
		a_fbs_id.ptr->hdr.target.id = ENOD_NOT_MATCH_ID;
		a_enodid = (a_eobj->enodid == 0) ? fa_deoj : a_eobj->enodid;
		a_enod = &eobjinib_table[a_enodid - 1];
		if ((a_enod->eobjatr == EOBJ_LOCAL_NODE)
			|| ((a_enod->eobjatr == EOBJ_ASYNC_REMOTE_NODE) && (fa_esv == ESV_GET))) {
			a_fbs_id.ptr->hdr.target.id = ENOD_LOCAL_ID;
		}
		else{
			for (i = 1; i < tnum_enodid; i++) {
				const EOBJCB *temp = &eobjcb_table[i];
				if (a_enod != temp->profile)
					continue;

				a_fbs_id.ptr->hdr.target.id = (ECN_ENOD_ID)(i + ENOD_REMOTE_ID - 1);
				break;
			}
		}
		if (a_fbs_id.ptr->hdr.target.id == ENOD_NOT_MATCH_ID) {
			goto lb_except;
			ECN_DBG_PUT_3("deoj = %02X %02X %02x : enod not match",
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx1,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx2,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx3);
		}
#ifdef ECN_DBG_PUT_ENA
		else {
			ECN_DBG_PUT("deoj = %02X %02X %02x : %s",
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx1,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx2,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx3,
				_ecn_dbg_enod2str(a_fbs_id.ptr->hdr.target));
		}
#endif
	}
#ifdef ECHONET_CONTROLLER_EXTENTION
	else{
		T_ECN_EOJ eoj;
		ECN_ENOD_ID enodid;
		ECN_DBG_PUT_1("fa_deoj = %d", fa_deoj);
		/* オブジェクトIDからEOJとノードIDを取得 */
		if(ecn_agent_get_eoj_enodid(fa_deoj, &eoj, &enodid)){
			memcpy(&((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj, &eoj,
				sizeof(((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj));
			a_fbs_id.ptr->hdr.target.id = enodid;
		}
		else {
			goto lb_except;
			ECN_DBG_PUT_3("deoj = %02X %02X %02x : enod not match",
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx1,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx2,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx3);
		}
#ifdef ECN_DBG_PUT_ENA
		else {
			ECN_DBG_PUT_4("deoj = %02X %02X %02x : %s",
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx1,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx2,
				((T_EDATA *)a_fbs_id.ptr)->hdr.edata.deoj.eojx3,
				_ecn_dbg_enod2str(a_fbs_id.ptr->hdr.target));
		}
#endif
	}
#endif
	a_fbs_id.ptr->hdr.type = ECN_MSG_ECHONET;
	a_fbs_id.ptr->hdr.sender.id = ENOD_API_ID;
	if (fa_esv == ESV_INFC)
		a_fbs_id.ptr->hdr.reply.id = a_fbs_id.ptr->hdr.target.id;
	else if (fa_esv == ESV_INF_REQ)
		a_fbs_id.ptr->hdr.reply.id = ENOD_MULTICAST_ID;
	else
		a_fbs_id.ptr->hdr.reply.id = ENOD_API_ID;

	/* 正常終了 */
	*fp_fbs_id = a_fbs_id;
	return E_OK;

lb_except:
	/* データ作成失敗したら領域解放 */
	if (a_fbs_id.ptr)
		_ecn_fbs_del(a_fbs_id);

	return a_ret;
}

/*
 * 内部使用メッセージ
 */
static void _ecn_tsk_int_msg(intptr_t fa_exinf, ECN_FBS_ID fa_fbs_id)
{
	T_ECN_INTERNAL_MSG	a_im;
	ECN_FBS_SSIZE_T		a_len;
	int					a_ans;

	a_im.command = 0;
	a_len = 0;
	a_ans = _ecn_fbs_get_data(fa_fbs_id, &a_im.command, sizeof(a_im.command), &a_len);
	if (a_ans || !a_len) {
		return;		/* NG */
	}

	switch (a_im.command) {
	case ECN_INM_NOTIFY_INSTANCELIST:
		ECN_DBG_PUT("do _ecn_tsk_int_startup()");
		_ecn_tsk_int_startup(fa_exinf);
		break;
	default:
#ifdef ECHONET_CONTROLLER_EXTENTION
		if(ecn_agent_proc_int_msg(fa_fbs_id, a_im.command))
			break;
#endif
		ECN_DBG_PUT_1("recv: unefined internal-msg: %d", a_im.command);
		break;
	}
}

/*
 * タスク初期化
 */
static void _ecn_tsk_int_startup(intptr_t fa_exinf)
{
	ER	a_ret;

	if (m_eobjlist_need_init)
		_ecn_tsk_int_module_init(fa_exinf);

	/* インスタンスリスト通知の送信 */
	a_ret = _ecn_tsk_ntf_inl(fa_exinf);
	if (a_ret) {
		syslog(LOG_WARNING, "_ecn_tsk_ntf_inl() result = %d:%s", a_ret, itron_strerror(a_ret));
	}
}

/*
 * インスタンスリスト通知の送信
 * ECHONET-Lite_Ver.1.10_02.pdf p.43 図4-1-4
 */
ER _ecn_tsk_ntf_inl(intptr_t fa_exinf)
{
	const T_ECN_EOJ	a_seoj =	/* 0x01 : 一般ノード、0x02：送信専用ノード */
		{ EOJ_X1_PROFILE, EOJ_X2_NODE_PROFILE, 0x01 };
	const T_ECN_EOJ	a_deoj =
		{ EOJ_X1_PROFILE, EOJ_X2_NODE_PROFILE, 0x01 };
	T_ECN_EDT_HDR a_hdr;
	ER			a_ret	=	E_OK;
	ECN_FBS_ID	a_fbs	=	{ 0 };
	int			a_eoj_ct =	0;
	int			i;
	uint8_t		a_count;
	const EOBJCB	*enod = &eobjcb_table[0];	/* ローカルノード */
	const EOBJINIB *eobj;

	ECN_DBG_PUT("do _ecn_tsk_ntf_inl()");

	memset(&a_hdr, 0, sizeof(a_hdr));
	a_hdr.ecn_hdr.ehd1 = ECN_EDH1_ECHONET_LITE;
	a_hdr.ecn_hdr.ehd2 = ECN_EDH2_FORMAT_1;
	a_hdr.edata.seoj = a_seoj;
	a_hdr.edata.deoj = a_deoj;
	a_hdr.edata.esv = ESV_INF;
	a_hdr.edata.opc = 1;
	a_hdr.ecn_prp.epc = ECN_EPC_INL_BCS;

	for (i = 0; i < enod->eobjcnt; i++) {
		eobj = enod->eobjs[i];

		if (!a_eoj_ct) {
			/* メモリ確保・ヘッダ格納 */
			a_hdr.ecn_hdr.tid = (uint8_t)_ecn_tsk_new_tid(); /* シーケンス番号生成 */
			a_ret = _ecn_fbs_cre(sizeof(a_hdr), &a_fbs);
			if (a_ret != E_OK) /* 確保失敗 */
				goto lb_except;
			a_ret = _ecn_fbs_add_data(a_fbs, &a_hdr, sizeof(a_hdr));
			if (a_ret != E_OK) /* データ追加失敗 */
				goto lb_except;

			a_fbs.ptr->hdr.type = ECN_MSG_ECHONET;
			a_fbs.ptr->hdr.sender.id = ENOD_LOCAL_ID;
			a_fbs.ptr->hdr.target.id = ENOD_MULTICAST_ID;
			a_fbs.ptr->hdr.reply.id = ENOD_LOCAL_ID;
			((T_EDATA *)a_fbs.ptr)->hdr.ecn_prp.pdc = 1;	/* 件数 */
			/* 件数を格納 */
			a_count = (uint8_t)(enod->eobjcnt - i);
			if (a_count >= ECN_IST_LST_EOJ_MAX_CT)
				a_count = ECN_IST_LST_EOJ_MAX_CT;
			a_ret = _ecn_fbs_add_data_ex(a_fbs, &a_count, sizeof(a_count));
			if (a_ret != E_OK) /* データ追加失敗 */
				goto lb_except;
		}
		/* ECHONETオブジェクトID(3byte)を格納 */
		a_ret = _ecn_fbs_add_data_ex(a_fbs, &eobj->eojx1, sizeof(T_ECN_EOJ));
		if (a_ret != E_OK) /* データ追加失敗 */
			goto lb_except;

		/* 件数・edtサイズ加算 */
		((T_EDATA *)a_fbs.ptr)->hdr.ecn_prp.pdc += sizeof(T_ECN_EOJ);
		if (++a_eoj_ct < ECN_IST_LST_EOJ_MAX_CT)
			continue;

		/* 1アナウンスでの上限に達したら、一旦送信 */
		a_ret = _ecn_tsk_snd_dtq(a_fbs, false);
		if (a_ret != E_OK) /* データ送信失敗 */
			goto lb_except;
		a_eoj_ct = 0;
		a_fbs.ptr = 0;
	}
	if (a_eoj_ct) {
		/* 未送信データがあったら、送信 */
		a_ret = _ecn_tsk_snd_dtq(a_fbs, false);
		if (a_ret != E_OK) /* データ送信失敗 */
			goto lb_except;
		a_fbs.ptr = 0;
	}
	goto lb_finally;

lb_except:
	if (a_fbs.ptr)
		_ecn_fbs_del(a_fbs);

lb_finally:
	return a_ret;
}

const EOBJCB *_ecn_eno_fnd(ECN_ENOD_ID enodid)
{
	const EOBJCB *enod = NULL;
	int i;

	switch (enodid) {
	case ENOD_MULTICAST_ID:
	case ENOD_LOCAL_ID:
	case ENOD_API_ID:
		enod = &eobjcb_table[0];
		break;
	default:
		i = enodid - ENOD_REMOTE_ID + 1;
		if ((i >= 1) && (i < tnum_enodid))
			enod = &eobjcb_table[i];
#ifdef ECHONET_CONTROLLER_EXTENTION
		else{
			ecn_node_t *p_tmp = ecn_agent_find_node(enodid);
			if (p_tmp == NULL)
				return NULL;
			return &p_tmp->eobj;
		}
#endif
		break;
	}

	return enod;
}

/*
 * 3byteのeobjidで配列中を検索する
 */
const EOBJINIB *_ecn_eoj_fnd(const EOBJCB *fp_nod, const T_ECN_EOJ *fp_eoj)
{
	const EOBJINIB	*p_obj;
	int		i, count = fp_nod->eobjcnt;

#ifdef ECHONET_CONTROLLER_EXTENTION
	if (fp_nod->eobjs == NULL) {
		ecn_obj_t *p_tmp = ecn_agent_find_eobj(fp_nod, *fp_eoj);
		if(p_tmp == NULL)
			return NULL;
		return &p_tmp->inib;
	}
#endif

	for (i = 0; i < count; i++) {
		p_obj = fp_nod->eobjs[i];

		if (p_obj->eojx1 != fp_eoj->eojx1)
			continue;
		if (p_obj->eojx2 != fp_eoj->eojx2)
			continue;
		if (p_obj->eojx3 != fp_eoj->eojx3)
			continue;

		return p_obj;
	}

	return 0;
}

/*
 * 電文の構成要素数とサイズのチェックを行う
 */
static bool_t _ecn_tsk_check_format(T_EDATA *edata, int len)
{
	ER ret;
	T_ENUM_EPC enm;
	int opc;
	uint8_t epc;
	uint8_t pdc;

	len -= sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY);

	ret = ecn_itr_ini(&enm, edata);
	if (ret != E_OK) {
		return false;
	}
	opc = enm.count;
	for (;;) {
		ret = ecn_itr_nxt(&enm, &epc, &pdc, NULL);
		if (enm.is_eof)
			break;
		if (ret == E_BOVR){
			if (opc != 0)
				return false;
			opc = enm.count;
			len -= 1;
			continue;
		}
		if (ret != E_OK)
			break;

		opc--;
		len -= sizeof(T_ECN_PRP) + pdc;
	}

	return (opc == 0) && (len == 0);
}

static int _ecn_tsk_ecn_msg_main(ECN_FBS_ID fa_fbs_id, const EOBJINIB *p_obj, ATR eobjatr,
	const EOBJINIB *p_sobj, ATR sobjatr);

/*
 * ECHONET用メッセージ
 */
static void _ecn_tsk_ecn_msg(intptr_t fa_exinf, ECN_FBS_ID fa_fbs_id)
{
	const EOBJCB	*p_nod, *p_snod;
	const EOBJINIB	*p_obj, *p_sobj = NULL;
	ATR eobjatr, sobjatr = EPC_NONE;
	T_ECN_EDT_HDR	*p_esv;
	ER				a_ret;
	int				i, count;
	T_ECN_EOJ		*p_eoj;
	bool_t			a_prc;
	bool_t			a_fwd;

#ifdef ECN_DBG_PUT_ENA
	syslog(LOG_NOTICE, "dtq recv:");
	_ecn_dbg_bindmp(fa_fbs_id.ptr->bin, sizeof(fa_fbs_id.ptr->bin));
#endif

	p_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;

	if (p_esv->ecn_hdr.ehd1 != ECN_EDH1_ECHONET_LITE
		||	p_esv->ecn_hdr.ehd2 != ECN_EDH2_FORMAT_1) {
		ECN_DBG_PUT_2("_ecn_tsk_ecn_msg() format fault: 0x%02X,0x%02X", p_esv->ecn_hdr.ehd1, p_esv->ecn_hdr.ehd2);
		return;
	}

	if (p_esv->edata.deoj.eojx3 > 0x7F) {
		ECN_DBG_PUT_1("_ecn_tsk_ecn_msg() format fault: deoj %06X",
			p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
		return;
	}

	if ((p_esv->edata.seoj.eojx3 > 0x7F) || (p_esv->edata.seoj.eojx3 == 0x00)) {
		ECN_DBG_PUT_1("_ecn_tsk_ecn_msg() format fault: seoj %06X",
			p_esv->edata.seoj.eojx1 << 16 | p_esv->edata.seoj.eojx2 << 8 | p_esv->edata.seoj.eojx3);
		return;
	}

	if ((p_esv->edata.esv & 0xC0) != 0x40) {
		ECN_DBG_PUT_1("_ecn_tsk_ecn_msg() format fault: esv 0x%02X", p_esv->edata.esv);
		return;
	}

	if (p_esv->edata.opc == 0x00) {
		ECN_DBG_PUT_1("_ecn_tsk_ecn_msg() format fault: opc 0x%02X", p_esv->edata.opc);
		return;
	}

	/* 電文の構成要素数とサイズのチェックを行う */
	if (!_ecn_tsk_check_format((T_EDATA *)fa_fbs_id.ptr, fa_fbs_id.ptr->hdr.length)) {
		ECN_DBG_PUT("_ecn_tsk_ecn_msg() format fault");
		return;
	}

	/* 送信宛からノードを検索 */
	p_nod = _ecn_eno_fnd(fa_fbs_id.ptr->hdr.target.id);
	/* 送信元からノードを検索 */
	p_snod = _ecn_eno_fnd(fa_fbs_id.ptr->hdr.sender.id);
	if (p_snod != NULL) {
		sobjatr = p_snod->profile->eobjatr;
		/* ノードプロファイルの場合 */
		if ((p_esv->edata.seoj.eojx1 == EOJ_X1_PROFILE)
			&& (p_esv->edata.seoj.eojx2 == EOJ_X2_NODE_PROFILE)) {
			p_sobj = p_snod->profile;
		}
		/* 機器オブジェクトの場合 */
		else {
			p_sobj = _ecn_eoj_fnd(p_snod, &p_esv->edata.seoj);
		}
	}

#ifdef ECHONET_CONTROLLER_EXTENTION
	ecn_agent_proc_ecn_msg(&p_snod, &p_sobj, (T_EDATA *)fa_fbs_id.ptr);
#endif

	/* ノード内の機器オブジェクトを検索 */
	a_prc = false;
	a_fwd = false;
	if (p_nod != NULL) {
		eobjatr = p_nod->profile->eobjatr;
		p_eoj = &p_esv->edata.deoj;
		/* ノードプロファイルの場合 */
		if ((p_eoj->eojx1 == EOJ_X1_PROFILE)
			&& (p_eoj->eojx2 == EOJ_X2_NODE_PROFILE)) {
			if ((p_eoj->eojx3 == p_nod->profile->eojx3)
				|| (p_eoj->eojx3 == 0)) {
				/* 電文処理実行 */
				if (_ecn_tsk_ecn_msg_main(fa_fbs_id, p_nod->profile, eobjatr, p_sobj, sobjatr) == 1)
					a_fwd = true;
				a_prc = true;
			}
			/* 0x74 プロパティ値通知（応答要）の場合の場合は電文破棄 */
			else if (p_esv->edata.esv == ESV_INFC) {
				a_prc = true;
			}
		}
		/* 機器オブジェクトの場合 */
		else {
			count = p_nod->eobjcnt;
#ifdef ECHONET_CONTROLLER_EXTENTION
			p_obj = NULL;
#endif
			for (i = 0; i < count; i++) {
#ifdef ECHONET_CONTROLLER_EXTENTION
				if(p_nod->eobjs == NULL)
					p_obj = ecn_agent_next_eobj(p_nod, p_obj);
				else
					p_obj = p_nod->eobjs[i];
#else
				p_obj = p_nod->eobjs[i];
#endif
				if (p_obj->eojx1 != p_eoj->eojx1)
					continue;
				if (p_obj->eojx2 != p_eoj->eojx2)
					continue;
				/* インスタンスコードが０の場合、同じクラスの全てのインスタンス宛 */
				if ((p_obj->eojx3 != p_eoj->eojx3) && (p_eoj->eojx3 != 0))
					continue;

				/* 電文処理実行 */
				if (_ecn_tsk_ecn_msg_main(fa_fbs_id, p_obj, eobjatr, p_sobj, sobjatr) == 1)
					a_fwd = true;
				a_prc = true;
			}

			/* 機器オブジェクトが見つからず、0x74 プロパティ値通知（応答要）の場合は電文破棄 */
			if (!a_prc && (p_esv->edata.esv == ESV_INFC)) {
				a_prc = true;
			}
		}
	}
	/* 機器オブジェクトが見つからない場合でも */
	if (!a_prc) {
		/* 電文処理実行（応答受信用） */
		if (_ecn_tsk_ecn_msg_main(fa_fbs_id, NULL, EPC_NONE, p_sobj, sobjatr) == 1)
			a_fwd = true;
	}

	/* 応答の場合アプリケーションに転送する */
	if (a_fwd && (p_esv->ecn_hdr.tid == g_api_tid)) {
		g_release_esv = false;

		ECN_CAP_PUT_1("redirect ecn_svc_dataqueueid → ecn_api_dataqueueid (esv:0x%02X)",
			p_esv->edata.esv);
		fa_fbs_id.ptr->hdr.target.id = ENOD_API_ID;
		a_ret = snd_dtq(ecn_api_dataqueueid, (intptr_t)fa_fbs_id.ptr);
		if (a_ret != E_OK) {
			syslog(LOG_WARNING, "_ecn_tsk_ecn_msg() : snd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
			_ecn_fbs_del(fa_fbs_id);
		}
	}
#ifdef ECHONET_CONTROLLER_EXTENTION
	ecn_agent_proc_ecn_msg_end();
#endif
}

static int _ecn_tsk_ecn_msg_main(ECN_FBS_ID fa_fbs_id, const EOBJINIB *p_obj, ATR eobjatr,
	const EOBJINIB *p_sobj, ATR sobjatr)
{
	int result;
	T_ECN_EDT_HDR	*p_esv;
	ECN_FBS_ID		a_fbs_anno = { NULL };
	bool_t fromapp = sobjatr == EOBJ_LOCAL_NODE;

	p_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;

	switch (p_esv->edata.esv) {
	/* 0x60 プロパティ値書き込み要求（応答不要） */
	case ESV_SET_I:
		if (!p_obj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set(p_obj, eobjatr, fromapp, false, fa_fbs_id, ESV_NOP, ESV_SET_I_SNA, &a_fbs_anno);	/* 0; 0x50 */
		result = 0;
		break;

	/* 0x61 プロパティ値書き込み要求（応答要） */
	case ESV_SET_C:
		if (!p_obj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set(p_obj, eobjatr, fromapp, false, fa_fbs_id, ESV_SET_RES, ESV_SET_C_SNA, &a_fbs_anno); /* 0x71; 0x51 */
		result = 0;
		break;

	/* 0x62 プロパティ値読み出し要求 */
	case ESV_GET:
		if (!p_obj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_get(p_obj, eobjatr, false, EPC_RULE_GET, fa_fbs_id, ESV_GET_RES, ESV_GET_SNA); /* 0x72; 0x52 */
		result = 0;
		break;

	/* 0x63 プロパティ値通知要求 */
	case ESV_INF_REQ:
		if (!p_obj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
			result = -1;
			break;
		}
		fa_fbs_id.ptr->hdr.reply.id = ENOD_MULTICAST_ID;
		_ecn_tsk_eoj_get(p_obj, eobjatr, false, (EPC_RULE_GET|EPC_RULE_ANNO), fa_fbs_id, ESV_INF, ESV_INF_SNA); /* 0x73; 0x53 */
		result = 0;
		break;

	/* 0x6E プロパティ値書き込み・読み出し要求 */
	case ESV_SET_GET:
		if (!p_obj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.deoj.eojx1 << 16 | p_esv->edata.deoj.eojx2 << 8 | p_esv->edata.deoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set_get(p_obj, eobjatr, fromapp, fa_fbs_id, ESV_SET_GET_RES, ESV_SET_GET_SNA, &a_fbs_anno); /* 0x7E; 0x5E */
		result = 0;
		break;

	/* 0x74 プロパティ値通知（応答要） */
	case ESV_INFC:
		if (!p_sobj)
			/* 送信元が未知の他ノードであった場合、応答を返す */
			_ecn_tsk_eoj_res(fa_fbs_id, ESV_INFC_RES, ESV_INFC_RES); /* 0x7A; 0x7A */
		else if (fromapp)
			/* アプリケーションからの要求の場合、プロパティ値通知（応答要）を送信する */
			_ecn_tsk_eoj_get(p_sobj, sobjatr, true, (EPC_RULE_GET|EPC_RULE_ANNO), fa_fbs_id, ESV_INFC, ESV_NOP); /* 0x74; 0 */
		else
			/* 送信元が既知の他ノードであった場合、プロパティ値を更新し応答を返す */
			_ecn_tsk_eoj_set(p_sobj, sobjatr, fromapp, true, fa_fbs_id, ESV_INFC_RES, ESV_INFC_RES, &a_fbs_anno); /* 0x7A; 0x7A */
		result = 0;
		break;

	/* 0x60 プロパティ値書き込み要求（応答不要） */
	case ESV_SET_I_SNA:		/* 0x50 プロパティ値書き込み要求不可応答 */
		result = 1;
		break;

	/* 0x61 プロパティ値書き込み要求（応答要） */
	case ESV_SET_RES:		/* 0x71 プロパティ値書き込み応答 */
	case ESV_SET_C_SNA:		/* 0x51 プロパティ値書き込み要求不可応答 */
		result = 1;
		break;

	/* 0x62 プロパティ値読み出し要求 */
	case ESV_GET_RES:		/* 0x72 プロパティ値読み出し応答 */
	case ESV_GET_SNA:		/* 0x52 プロパティ値読み出し不可応答 */
		if (!p_sobj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.seoj.eojx1 << 16 | p_esv->edata.seoj.eojx2 << 8 | p_esv->edata.seoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set(p_sobj, sobjatr, fromapp, true, fa_fbs_id, ESV_NOP, ESV_NOP, &a_fbs_anno); /* 0; 0 */
		result = 1;
		break;

	/* 0x63 プロパティ値通知要求 */
	case ESV_INF:			/* 0x73 プロパティ値通知 */
	case ESV_INF_SNA:		/* 0x53 プロパティ値通知不可応答 */
		if (!p_sobj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.seoj.eojx1 << 16 | p_esv->edata.seoj.eojx2 << 8 | p_esv->edata.seoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set(p_sobj, sobjatr, fromapp, true, fa_fbs_id, ESV_NOP, ESV_NOP, &a_fbs_anno); /* 0; 0 */
		result = 1;
		break;

	/* 0x6E プロパティ値書き込み・読み出し要求 */
	case ESV_SET_GET_RES:	/* 0x7E プロパティ値書き込み・読み出し応答 */
	case ESV_SET_GET_SNA:	/* 0x5E プロパティ値書き込み・読み出し不可応答 */
		if (!p_sobj) {
			ECN_DBG_PUT_1("_ecn_tsk_ecn_msg_main() eoj %06X not found.",
				p_esv->edata.seoj.eojx1 << 16 | p_esv->edata.seoj.eojx2 << 8 | p_esv->edata.seoj.eojx3);
			result = -1;
			break;
		}
		_ecn_tsk_eoj_set_get_res(p_sobj, sobjatr, fromapp, fa_fbs_id, &a_fbs_anno); /* 0x7A; 0 */
		result = 1;
		break;

	/* 0x74 プロパティ値通知（応答要） */
	case ESV_INFC_RES:		/* 0x7A プロパティ値通知応答 */
		result = 1;
		break;

	default:
		syslog(LOG_WARNING, "_ecn_tsk_ecn_msg_main() esv 0x%02X undefined.", p_esv->edata.esv);
		result = -1;
		break;
	}

	/* プロパティ通知要求を送信 */
	if (a_fbs_anno.ptr != NULL) {
		ER a_ret = _ecn_tsk_snd_dtq(a_fbs_anno, true);
		if (a_ret != E_OK)
			_ecn_fbs_del(a_fbs_anno);
	}

	return result;
}

#ifdef ECN_DBG_PUT_ENA
static void f_put_fbs_eoj(const char *fp_fncnm, const char *fp_varnm,
	ECN_FBS_ID fa_fbs_id);
static void f_put_fbs_eoj(const char *fp_fncnm, const char *fp_varnm,
	ECN_FBS_ID fa_fbs_id)
{
	ECN_DBG_PUT("%s() %s eoj src:%06X dest:%06X",
		fp_fncnm, fp_varnm,
		((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.seoj.eojx1 << 16 | ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.seoj.eojx2 << 8 | ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.seoj.eojx3,
		((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.deoj.eojx1 << 16 | ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.deoj.eojx2 << 8 | ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.deoj.eojx3);
}
#else
#define f_put_fbs_eoj(f,v,r)
#endif /* #ifdef ECN_DBG_PUT_ENA */

static const EPRPINIB *_ecn_tsk_eoj_get_property(const EOBJINIB *fp_obj, uint8_t fa_epc);
static const EPRPINIB *_ecn_tsk_eoj_get_property(const EOBJINIB *fp_obj, uint8_t fa_epc)
{
	const EPRPINIB	*p = fp_obj->eprp;
	uint_t			i;
#ifdef ECHONET_CONTROLLER_EXTENTION
	if (fp_obj->eprpcnt == 0) {
		return ecn_agent_get_property(fp_obj, fa_epc, p);
	}
#endif
	for (i = 0; i < fp_obj->eprpcnt; i++, p++) {
		if (p->eprpcd != fa_epc)
			continue;

		return p;
	}
	ECN_DBG_PUT_4("_ecn_tsk_eoj_get_property(0x%02X-0x%02X-0x%02X, epc:0x%02X) not found.",
		fp_obj->eojx1, fp_obj->eojx2, fp_obj->eojx3,
		fa_epc);

	return NULL;
}

static int _ecn_tsk_eoj_set_edt(const EPRPINIB *fp_prp, void *fp_src, int fa_srcsz,
	bool_t *fa_anno);
static int _ecn_tsk_eoj_set_edt(const EPRPINIB *fp_prp, void *fp_src, int fa_srcsz,
	bool_t *fa_anno)
{
	if (!fp_prp->eprpset)
		return -1;

	ECN_DBG_PUT_1("_ecn_tsk_eoj_set_edt(epc:0x%02X) call eprpset()",
		fp_prp->eprpcd);

	return fp_prp->eprpset(fp_prp, fp_src, fa_srcsz, fa_anno);
}

static int _ecn_tsk_eoj_get_edt(const EPRPINIB *fp_prp, void *fp_dst, int fa_dstsz);
static int _ecn_tsk_eoj_get_edt(const EPRPINIB *fp_prp, void *fp_dst, int fa_dstsz)
{
	if (fa_dstsz < fp_prp->eprpsz)
		return -1;
	if (!fp_prp->eprpget)
		return -1;

	ECN_DBG_PUT_1("_ecn_tsk_eoj_get_edt(epc:0x%02X) call eprpget()",
		fp_prp->eprpcd);

	return fp_prp->eprpget(fp_prp, fp_dst, fp_prp->eprpsz);
}

static T_ECN_SUB_BLK *_ecn_tsk_get_prp_pce(ECN_FBS_ID fa_fbs_id, T_ECN_PRP *fp_prp,
	T_ECN_SUB_BLK *const fp_prev_blk);

/*
 * ECN_FBS_IDからT_ECN_PRP1件とその付随データ(edt)を抽出し、edtのポインタを返す
 * 引数
 * ECN_FBS_ID fa_fbs_id					読み取るFBS
 * T_ECN_PRP *fp_prp					epc+pdcの格納先
 * T_ECN_SUB_BLK * const fp_prev_blk	前回使ったメモリのポインタ(無ければ0)
 * 正常：ポインタ NG:0
 * ポインタは_ecn_fbs_dtq_rel()で解放する必要がある
 */
static T_ECN_SUB_BLK *_ecn_tsk_get_prp_pce(ECN_FBS_ID fa_fbs_id, T_ECN_PRP *fp_prp,
	T_ECN_SUB_BLK *const fp_prev_blk)
{
	T_ECN_SUB_BLK	*p_blk = 0;
	int		a_size;
	ER		a_ret;

	/* プロパティ用メモリの取得 */
	if (fp_prev_blk) {
		p_blk = fp_prev_blk; /* 前回使ったメモリがあるなら、再利用する */
	}
	else {
		p_blk = (T_ECN_SUB_BLK *)_ecn_fbs_dtq_get(sizeof(*p_blk));
		if (!p_blk) {
			ECN_DBG_PUT("_ecn_fbs_dtq_get() fault.");
			return 0;	/* メモリ不足 */
		}
	}
	memset(p_blk, 0, sizeof(*p_blk));

	/* T_ECN_PRP部分(epc,pdc)を読み取る */
	a_size = 0;
	a_ret = _ecn_fbs_get_data(fa_fbs_id, fp_prp, sizeof(*fp_prp), &a_size);
	if (a_ret || a_size < (int)sizeof(*fp_prp)) {
		ECN_DBG_PUT_2("_ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	if (0 < fp_prp->pdc) {
		/* 付随データ部分(edt)を読み取る */
		a_size = 0;
		a_ret = _ecn_fbs_get_data(fa_fbs_id, p_blk, fp_prp->pdc, &a_size);
		if (a_ret || a_size < (int)fp_prp->pdc) {
			ECN_DBG_PUT_2("_ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
	}

	return p_blk;

lb_except:
	/* プロパティ用メモリ解放 */
	if (p_blk && !fp_prev_blk)
		_ecn_fbs_dtq_rel(p_blk);

	return 0;	/* 0:NG */
}

static ER _ecn_tsk_eoj_set_main(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp, bool_t fa_update,
	ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, ECN_FBS_ID *fa_fbs_anno, int a_count,
	int *p_sw_ok);
/* プロパティ値書き込み実行 */
static ER _ecn_tsk_eoj_set_main(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp, bool_t fa_update,
	ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, ECN_FBS_ID *fa_fbs_anno, int a_count,
	int *p_sw_ok)
{
	T_ECN_SUB_BLK	*p_edt	=	0;
	T_ECN_PRP			a_prp;			/* epc+pdc */
	const EPRPINIB		*a_eprp;
	int			i, a_ans;
	ER			a_ret;
	uint8_t		a_size;
	bool_t		a_anno = false, a_update;

	((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc = 0;

	for (i = 0; i < a_count; i++) {
		/* ECN_FBS_IDからT_ECN_PRP1件とその付随データを抽出し、そのポインタを返す */
		p_edt = _ecn_tsk_get_prp_pce(fa_fbs_id, &a_prp, p_edt);
		if (!p_edt) {
			ECN_DBG_PUT("_ecn_tsk_eoj_set_main(): _ecn_tsk_get_prp_pce() fault.");
			goto lb_except;
		}

		/* プロパティの設定 set_prp(obj, reqp, resp) */
		a_size = a_prp.pdc;
		/* obj,epcに対応するset関数を呼ぶ */
		a_eprp = _ecn_tsk_eoj_get_property(fp_obj, a_prp.epc);
		if ((a_eprp != NULL)
			&& (fa_update || (((a_eprp->eprpatr & EPC_RULE_SET) != 0) || fa_fromapp))) {
			a_anno = (fa_eobjatr == EOBJ_LOCAL_NODE) && ((a_eprp->eprpatr & EPC_ANNOUNCE) != 0);
			a_update = a_anno;
			a_ans = _ecn_tsk_eoj_set_edt(a_eprp, p_edt->payload, a_size, &a_update);
			if (a_anno && (a_ans > 0))
				a_anno = a_update;
		}
		else {
			a_ans = -1;
		}
		if (a_ans == a_size) {
			ECN_DBG_PUT_4("_ecn_tsk_eoj_set_edt(0x%06X, 0x%02x, 0x%02X..., %u) ok.",
				fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
				a_prp.epc, p_edt->payload[0], a_size);
			a_prp.pdc = 0;
		} else {
			ECN_DBG_PUT_4("_ecn_tsk_eoj_set_edt(0x%06X, 0x%02x, 0x%02X..., %u) fault.",
				fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
				a_prp.epc, p_edt->payload[0], a_size);
			*p_sw_ok = 0;	/* プロパティ設定失敗 */
			/* 応答処理の場合EDTは設定しない */
			if (fa_update)
				a_prp.pdc = 0;
		}

		/* 応答電文用メモリにデータ追加(epc,pdcの2byte) */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_prp, sizeof(a_prp));
		if (a_ret) {
			ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}

		if (0 < a_prp.pdc) {
			/* 応答電文用メモリにデータ追加(edt n-byte) */
			a_ret = _ecn_fbs_add_data_ex(a_fbs_res, p_edt->payload, a_prp.pdc);
			if (a_ret) {
				ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
				goto lb_except;
			}
		}
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc ++;

		/* プロパティ通知ありの場合 */
		if (a_anno && (a_ans == a_size)) {
			if (fa_fbs_anno->ptr == NULL) {
				_ecn_tsk_mk_esv(fa_fbs_anno, (ID)1, (((intptr_t)fp_obj - (intptr_t)eobjinib_table) / sizeof(EPRPINIB)) + 1,
					a_prp.epc, 0, NULL, ESV_INF_REQ);
			}
			else{
				ecn_add_edt((T_EDATA *)fa_fbs_anno->ptr, a_prp.epc, 0, 0);
			}
		}
	}
	a_ret = E_OK; /* ok */
	goto lb_finally;

lb_except:
	a_ret = E_SYS;

lb_finally:
	/* プロパティ用メモリ解放 */
	if (p_edt)
		_ecn_fbs_dtq_rel(p_edt);

	return a_ret;
}

static ER _ecn_tsk_eoj_get_main(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_forward, ATR fa_access,
	ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, int a_count, int *p_sw_ok);
/* プロパティ値読み出し実行 */
static ER _ecn_tsk_eoj_get_main(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_forward, ATR fa_access,
	ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, int a_count, int *p_sw_ok)
{
	T_ECN_SUB_BLK	*p_edt		=	0;
	void				*p_prp_buf	=	0;	/* 作業領域 */
	const EPRPINIB		*a_eprp;
	ER					a_ret		=	E_SYS;
	T_ECN_PRP			a_prp;			/* epc+pdc */
	int		i, a_ans;

	((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc = 0;

	/* 作業領域確保 */
	p_prp_buf = _ecn_fbs_dtq_get(ECHONET_MEMPOOL_BLOCK_SIZE);
	if (!p_prp_buf) {
		ECN_DBG_PUT_2("_ecn_fbs_dtq_get() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;	/* メモリ不足 */
	}

	for (i = 0; i < a_count; i++) {
		/* ECN_FBS_IDからT_ECN_PRP1件とその付随データを抽出 */
		p_edt = _ecn_tsk_get_prp_pce(fa_fbs_id, &a_prp, p_edt);
		if (!p_edt) {
			ECN_DBG_PUT("_ecn_tsk_eoj_get_main(): _ecn_tsk_get_prp_pce() fault.");
			goto lb_except;
		}

		/* プロパティの取得 get_eprp(obj, reqp, resp, size) */
		memset(p_prp_buf, 0, ECHONET_MEMPOOL_BLOCK_SIZE);
		/* obj,epcに対応するget関数を呼ぶ */
		a_eprp = _ecn_tsk_eoj_get_property(fp_obj, a_prp.epc);
		if ((a_eprp != NULL) && (((a_eprp->eprpatr & fa_access) != 0) && ((fa_eobjatr == EOBJ_LOCAL_NODE) || fa_forward))) {
			a_ans = _ecn_tsk_eoj_get_edt(a_eprp, p_prp_buf, ECHONET_MEMPOOL_BLOCK_SIZE - 1);
		}
		else {
			a_ans = -1;
		}
		if (0 < a_ans && a_ans <= (int)UINT8_MAX) {
			a_prp.pdc = (uint8_t)a_ans;
		} else {
			*p_sw_ok = 0;	/* プロパティ取得失敗 */
			a_ans = 0;
			a_prp.pdc = 0;
		}

		/* 応答電文用メモリにデータ追加(epc,pdcの2byte) */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_prp, sizeof(a_prp));
		if (a_ret) {
			ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
		if (0 < a_ans) {
			/* 付随データ追加 */
			a_ret = _ecn_fbs_add_data_ex(a_fbs_res, p_prp_buf, a_ans);
			if (a_ret) {
				ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
				goto lb_except;
			}
		}
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc ++;
	}
	a_ret = E_OK; /* ok */
	goto lb_finally;

lb_except:
	a_ret = E_SYS;

lb_finally:
	/* 作業領域解放 */
	if (p_prp_buf)
		_ecn_fbs_dtq_rel(p_prp_buf);

	/* プロパティ用メモリ解放 */
	if (p_edt)
		_ecn_fbs_dtq_rel(p_edt);

	return a_ret;
}

static ER _ecn_tsk_eoj_res_main(ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, int a_count, int *p_sw_ok);
/* プロパティ値読み出し実行 */
static ER _ecn_tsk_eoj_res_main(ECN_FBS_ID fa_fbs_id, ECN_FBS_ID a_fbs_res, int a_count, int *p_sw_ok)
{
	T_ECN_SUB_BLK	*p_edt		=	0;
	void				*p_prp_buf	=	0;	/* 作業領域 */
	ER					a_ret		=	E_SYS;
	T_ECN_PRP			a_prp;			/* epc+pdc */
	int		i;

	((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc = 0;

	/* 作業領域確保 */
	p_prp_buf = _ecn_fbs_dtq_get(ECHONET_MEMPOOL_BLOCK_SIZE);
	if (!p_prp_buf) {
		ECN_DBG_PUT_2("_ecn_fbs_dtq_get() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;	/* メモリ不足 */
	}

	for (i = 0; i < a_count; i++) {
		/* ECN_FBS_IDからT_ECN_PRP1件とその付随データを抽出 */
		p_edt = _ecn_tsk_get_prp_pce(fa_fbs_id, &a_prp, p_edt);
		if (!p_edt) {
			ECN_DBG_PUT("_ecn_tsk_eoj_get_main(): _ecn_tsk_get_prp_pce() fault.");
			goto lb_except;
		}

		a_prp.pdc = 0;

		/* 応答電文用メモリにデータ追加(epc,pdcの2byte) */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_prp, sizeof(a_prp));
		if (a_ret) {
			ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc++;
	}
	a_ret = E_OK; /* ok */
	goto lb_finally;

lb_except:
	a_ret = E_SYS;

lb_finally:
	/* 作業領域解放 */
	if (p_prp_buf)
		_ecn_fbs_dtq_rel(p_prp_buf);

	/* プロパティ用メモリ解放 */
	if (p_edt)
		_ecn_fbs_dtq_rel(p_edt);

	return a_ret;
}

/* プロパティ値読み飛ばし実行 */
static ER _ecn_tsk_eoj_skip_main(const EOBJINIB *fp_obj, ECN_FBS_ID fa_fbs_id,
	int a_count);
static ER _ecn_tsk_eoj_skip_main(const EOBJINIB *fp_obj, ECN_FBS_ID fa_fbs_id,
	int a_count)
{
	T_ECN_PRP	a_prp;			/* epc+pdc */
	int			i, a_size;
	ER			a_ret;

	for (i = 0; i < a_count; i++) {
		/* T_ECN_PRP部分(epc,pdc)を読み取る */
		a_size = 0;
		a_ret = _ecn_fbs_get_data(fa_fbs_id, &a_prp, sizeof(a_prp), &a_size);
		if (a_ret || a_size < (int)sizeof(a_prp)) {
			ECN_DBG_PUT_2("_ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}

		/* pdc分読み飛ばし */
		a_ret = _ecn_fbs_seek_rpos(fa_fbs_id, a_prp.pdc);
		if (a_ret) {
			ECN_DBG_PUT_2("_ecn_fbs_seek_rpos() result = %d:%s", a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
	}
	a_ret = E_OK; /* ok */
	goto lb_finally;

lb_except:
	a_ret = E_SYS;

lb_finally:
	return a_ret;
}

static void _ecn_tsk_mk_rsp_hdr(const EOBJINIB *fp_obj, T_ECN_EDT_HDR *fp_rsp_esv,
	T_ECN_EDT_HDR const *fp_req_esv);
/* 応答電文設定 */
static void _ecn_tsk_mk_rsp_hdr(const EOBJINIB *fp_obj, T_ECN_EDT_HDR *fp_rsp_esv,
	T_ECN_EDT_HDR const *fp_req_esv)
{
	fp_rsp_esv->ecn_hdr.ehd1 = ECN_EDH1_ECHONET_LITE;
	fp_rsp_esv->ecn_hdr.ehd2 = ECN_EDH2_FORMAT_1;
	fp_rsp_esv->ecn_hdr.tid = fp_req_esv->ecn_hdr.tid;
	fp_rsp_esv->edata.seoj.eojx1 = fp_obj->eojx1;
	fp_rsp_esv->edata.seoj.eojx2 = fp_obj->eojx2;
	fp_rsp_esv->edata.seoj.eojx3 = fp_obj->eojx3;
	fp_rsp_esv->edata.deoj = fp_req_esv->edata.seoj;
}

static void _ecn_tsk_mk_rsp_hdr_res(T_ECN_EDT_HDR *fp_rsp_esv,
	T_ECN_EDT_HDR const *fp_req_esv);
static void _ecn_tsk_mk_rsp_hdr_res(T_ECN_EDT_HDR *fp_rsp_esv,
	T_ECN_EDT_HDR const *fp_req_esv)
{
	fp_rsp_esv->ecn_hdr.ehd1 = ECN_EDH1_ECHONET_LITE;
	fp_rsp_esv->ecn_hdr.ehd2 = ECN_EDH2_FORMAT_1;
	fp_rsp_esv->ecn_hdr.tid = fp_req_esv->ecn_hdr.tid;
	fp_rsp_esv->edata.seoj = fp_req_esv->edata.deoj;
	fp_rsp_esv->edata.deoj = fp_req_esv->edata.seoj;
}

/* 応答電文用fbs設定 */
static void _ecn_tsk_set_rsp_fbs(ECN_FBS_ID fa_rsp_fbs, T_ECN_FST_BLK const *fp_req_ptr);
/* 応答電文用fbs設定(sender/targetの設定) */
static void _ecn_tsk_set_rsp_fbs(ECN_FBS_ID fa_rsp_fbs, T_ECN_FST_BLK const *fp_req_ptr)
{
	fa_rsp_fbs.ptr->hdr.type = ECN_MSG_ECHONET;
	fa_rsp_fbs.ptr->hdr.sender.id = ENOD_LOCAL_ID;
	fa_rsp_fbs.ptr->hdr.target.id = fp_req_ptr->hdr.reply.id;
	fa_rsp_fbs.ptr->hdr.reply.id = ENOD_LOCAL_ID;
}

/* プロパティ値書き込み要求	*/
static void _ecn_tsk_eoj_set(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp, bool_t fa_update,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv,
	ECN_FBS_ID *fa_fbs_anno)
{
	T_ECN_EDT_HDR	const *p_req_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;
	T_ECN_EDT_HDR	a_ecn_hdp;
	ECN_FBS_ID		a_fbs_res	= { 0 };
	ER		a_ret;
	int		a_size;
	int		a_sw_ok = 1;
	T_ECN_EOJ	eoj;

	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));

	/* 応答最大サイズの取得 */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 応答電文用メモリの取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_res);
	if (a_ret != E_OK || !a_fbs_res.ptr) /* 確保失敗 */
		return;

	/* 応答電文設定 */
	if(p_req_esv->edata.deoj.eojx3 != 0)
		_ecn_tsk_mk_rsp_hdr_res(&a_ecn_hdp, p_req_esv);
	else
		_ecn_tsk_mk_rsp_hdr(fp_obj, &a_ecn_hdp, p_req_esv);

	/* 正常時の応答電文がプロパティ値通知応答の場合 */
	if (fa_res_esv == ESV_INFC_RES) {
		/* 送信元と宛先を入れ替え */
		eoj = a_ecn_hdp.edata.seoj;
		a_ecn_hdp.edata.seoj = a_ecn_hdp.edata.deoj;
		a_ecn_hdp.edata.deoj = eoj;
	}

	f_put_fbs_eoj("_ecn_tsk_eoj_set", "fa_fbs_id", fa_fbs_id); /* s/deoj デバッグ出力 */

	/* 応答電文用メモリにデータ追加 */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_ecn_hdp,
				sizeof(a_ecn_hdp.ecn_hdr) + sizeof(a_ecn_hdp.edata));
	if (a_ret)
		goto lb_except;

	/* payload先頭のT_ECN_HDR(ecn_hdr), T_ECN_EDATA_BODY(edata)を読み飛ばす */
	a_ret = _ecn_fbs_set_rpos(fa_fbs_id, offsetof(T_ECN_EDT_HDR, ecn_prp));
	if (a_ret)
		goto lb_except;

	/* 応答電文用fbs設定 */
	_ecn_tsk_set_rsp_fbs(a_fbs_res, fa_fbs_id.ptr);

	/* プロパティ値書き込み実行 */
	a_ret = _ecn_tsk_eoj_set_main(fp_obj, fa_eobjatr, fa_fromapp, fa_update, fa_fbs_id, a_fbs_res,
		fa_fbs_anno, p_req_esv->edata.opc, &a_sw_ok);
	if (a_ret)
		goto lb_except;

	/* 応答要の場合 */
	if (a_sw_ok) {
		if (!fa_res_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理成功 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_res_esv;
	} else {
		if (!fa_sna_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理失敗 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_sna_esv;
	}
	/* 応答送信 */
	a_ret = _ecn_tsk_snd_dtq(a_fbs_res, false);
	if (a_ret == E_OK)
		goto lb_finally;

lb_except:
	/* データ送信失敗したら領域解放 */
	if (a_fbs_res.ptr)
		_ecn_fbs_del(a_fbs_res);

lb_finally:
	return;
}

/* プロパティ値読み出し要求	*/
static void _ecn_tsk_eoj_get(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_forward, ATR fa_access,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv)
{
	T_ECN_EDT_HDR	const *p_req_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;
	T_ECN_EDT_HDR	a_ecn_hdp;
	ECN_FBS_ID		a_fbs_res	= { 0 };
	ER		a_ret;
	int		a_size;
	int		a_sw_ok = 1;

	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));

	/* 初期取得サイズ */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 応答電文用メモリの取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_res);
	if (a_ret != E_OK || !a_fbs_res.ptr) { /* 確保失敗 */
		ECN_DBG_PUT_2("_ecn_fbs_cre() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_finally;
	}

	/* 応答電文設定 */
	if(p_req_esv->edata.deoj.eojx3 != 0)
		_ecn_tsk_mk_rsp_hdr_res(&a_ecn_hdp, p_req_esv);
	else
		_ecn_tsk_mk_rsp_hdr(fp_obj, &a_ecn_hdp, p_req_esv);

	f_put_fbs_eoj("_ecn_tsk_eoj_get", "fa_fbs_id", fa_fbs_id); /* s/deoj デバッグ出力 */

	/* 応答電文用メモリにデータ追加 */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_ecn_hdp,
			sizeof(a_ecn_hdp.ecn_hdr) + sizeof(a_ecn_hdp.edata));
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	/* payload先頭のT_ECN_HDR(ecn_hdr), T_ECN_EDATA_BODY(edata)を読み飛ばす */
	a_ret = _ecn_fbs_set_rpos(fa_fbs_id, offsetof(T_ECN_EDT_HDR, ecn_prp));
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_fbs_set_rpos() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	/* 応答電文用fbs設定 */
	_ecn_tsk_set_rsp_fbs(a_fbs_res, fa_fbs_id.ptr);

	/* プロパティ値読み込み実行 */
	a_ret = _ecn_tsk_eoj_get_main(fp_obj, fa_eobjatr, fa_forward, fa_access, fa_fbs_id, a_fbs_res,
		((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.opc, &a_sw_ok);
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_tsk_eoj_get_main() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	if (a_sw_ok) {
		if (!fa_res_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理成功 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_res_esv;
	} else {
		if (!fa_sna_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理失敗 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_sna_esv;

		/* 不可応答の場合は個別送信する */
		if (a_fbs_res.ptr->hdr.target.id == ENOD_MULTICAST_ID) {
			a_fbs_res.ptr->hdr.target.id = fa_fbs_id.ptr->hdr.sender.id;
		}
	}
	/* 応答送信 */
	a_ret = _ecn_tsk_snd_dtq(a_fbs_res, false);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_tsk_snd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	goto lb_finally;

lb_except:
	/* データ送信失敗したら領域解放 */
	if (a_fbs_res.ptr)
		_ecn_fbs_del(a_fbs_res);

lb_finally:
	return;
}

/* プロパティ値読み出し要求	*/
static void _ecn_tsk_eoj_res(ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv,
	ECN_SRV_CODE fa_sna_esv)
{
	T_ECN_EDT_HDR	const *p_req_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;
	T_ECN_EDT_HDR	a_ecn_hdp;
	ECN_FBS_ID		a_fbs_res	= { 0 };
	ER		a_ret;
	int		a_size;
	int		a_sw_ok = 1;

	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));

	/* 初期取得サイズ */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 応答電文用メモリの取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_res);
	if (a_ret != E_OK || !a_fbs_res.ptr) { /* 確保失敗 */
		ECN_DBG_PUT_2("_ecn_fbs_cre() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_finally;
	}

	/* 応答電文設定						*/
	_ecn_tsk_mk_rsp_hdr_res(&a_ecn_hdp, p_req_esv);

	f_put_fbs_eoj("_ecn_tsk_eoj_res", "fa_fbs_id", fa_fbs_id); /* s/deoj デバッグ出力 */

	/* 応答電文用メモリにデータ追加 */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_ecn_hdp,
			sizeof(a_ecn_hdp.ecn_hdr) + sizeof(a_ecn_hdp.edata));
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	/* payload先頭のT_ECN_HDR(ecn_hdr), T_ECN_EDATA_BODY(edata)を読み飛ばす */
	a_ret = _ecn_fbs_set_rpos(fa_fbs_id, offsetof(T_ECN_EDT_HDR, ecn_prp));
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_fbs_set_rpos() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	/* 応答電文用fbs設定 */
	_ecn_tsk_set_rsp_fbs(a_fbs_res, fa_fbs_id.ptr);

	/* プロパティ値読み込み実行 */
	a_ret = _ecn_tsk_eoj_res_main(fa_fbs_id, a_fbs_res,
		((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.opc, &a_sw_ok);
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_tsk_eoj_res_main() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	if (a_sw_ok) {
		if (!fa_res_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理成功 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_res_esv;
	} else {
		if (!fa_sna_esv) {
			/* 応答不要の場合 */
			_ecn_fbs_del(a_fbs_res);
			goto lb_finally;
		}
		/* 設定処理失敗 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_sna_esv;
	}
	/* 応答送信 */
	a_ret = _ecn_tsk_snd_dtq(a_fbs_res, false);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_tsk_snd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	goto lb_finally;

lb_except:
	/* データ送信失敗したら領域解放 */
	if (a_fbs_res.ptr)
		_ecn_fbs_del(a_fbs_res);

lb_finally:
	return;
}

static void _ecn_tsk_eoj_set_get(const EOBJINIB *fp_obj, ATR fa_eobjatr, bool_t fa_fromapp,
	ECN_FBS_ID fa_fbs_id, ECN_SRV_CODE fa_res_esv, ECN_SRV_CODE fa_sna_esv,
	ECN_FBS_ID *fa_fbs_anno)
{
	T_ECN_EDT_HDR	const *p_req_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;
	T_ECN_EDT_HDR	a_ecn_hdp;
	ECN_FBS_ID		a_fbs_res	= { 0 };
	int				a_rdprp_wrpos;	/*	プロパティ読み込み件数書き込み時のヘッド位置	*/
	ER		a_ret;
	int		i, a_size, a_rdlen;
	int		a_sw_ok = 1;
	int		a_count = ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.opc;	/* 処理プロパティ数 */

	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));

	ECN_DBG_PUT_1("_ecn_tsk_eoj_set_get() eoj:%06X",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3);

	/* 初期取得サイズ */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 応答電文用メモリの取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_res);
	if (a_ret != E_OK || !a_fbs_res.ptr) /* 確保失敗 */
		goto lb_finally;

	/* 応答電文設定 */
	if(p_req_esv->edata.deoj.eojx3 != 0)
		_ecn_tsk_mk_rsp_hdr_res(&a_ecn_hdp, p_req_esv);
	else
		_ecn_tsk_mk_rsp_hdr(fp_obj, &a_ecn_hdp, p_req_esv);
	a_size -= sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY);

	f_put_fbs_eoj("_ecn_tsk_eoj_set_get", "fa_fbs_id", fa_fbs_id); /* s/deoj デバッグ出力 */

	/* 応答電文用メモリにデータ追加 */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &a_ecn_hdp,
				sizeof(a_ecn_hdp.ecn_hdr) + sizeof(a_ecn_hdp.edata));
	if (a_ret)
		goto lb_except;

	/* payload先頭のT_ECN_HDR(ecn_hdr), T_ECN_EDATA_BODY(edata)を読み飛ばす */
	a_ret = _ecn_fbs_set_rpos(fa_fbs_id, offsetof(T_ECN_EDT_HDR, ecn_prp));
	if (a_ret)
		goto lb_except;

	/* 応答電文用fbs設定 */
	_ecn_tsk_set_rsp_fbs(a_fbs_res, fa_fbs_id.ptr);

	/* プロパティ値書き込み実行 */
	a_ret = _ecn_tsk_eoj_set_main(fp_obj, fa_eobjatr, fa_fromapp, false, fa_fbs_id, a_fbs_res,
		fa_fbs_anno, a_count, &a_sw_ok);
	if (a_ret) {
		syslog(LOG_WARNING, "_ecn_tsk_eoj_set_get() eoj:%06X _ecn_tsk_eoj_set_main() fault. result = %d",
			fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
			a_ret);
		goto lb_except;
	}
	ECN_DBG_PUT_2("_ecn_tsk_eoj_set_get() eoj:%06X _ecn_tsk_eoj_set_main() ok sw_ok:%s",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
		(a_sw_ok ? "true" : "false"));

	/* 次の件数を読み取る(1byte)						*/
	/* → ECHONET-Lite_Ver.1.10_02.pdf p.40 [OPCGet]	*/
	a_rdlen = i = 0;
	a_ret = _ecn_fbs_get_data(fa_fbs_id, &i, 1, &a_rdlen);
	if (a_ret < 0 || a_rdlen < 1)
		goto lb_except;	/* NG */
	a_count = *(uint8_t *)&i;

	/* プロパティ読み込み件数書き込み時のヘッド情報を記録 */
	a_rdprp_wrpos = _ecn_fbs_get_datalen(a_fbs_res);

	/* 応答電文用メモリにデータ追加 (OPCGet 1byte) */
	a_ret = _ecn_fbs_add_data_ex(a_fbs_res, &i, 1);
	if (a_ret)
		goto lb_except;

	/* この時点での応答電文中プロパティ件数を記録 */
	i = ((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc;
	((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc = 0;

	/* プロパティ値読み込み実行 */
	a_ret = _ecn_tsk_eoj_get_main(fp_obj, fa_eobjatr, false, EPC_RULE_GET, fa_fbs_id, a_fbs_res,
		a_count, &a_sw_ok);
	if (a_ret) {
		syslog(LOG_WARNING, "%s _ecn_tsk_eoj_set_get() eoj:%06X _ecn_tsk_eoj_get_main() fault. result = %d",
			ECN_SRC_LOC,
			fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
			a_ret);
		goto lb_except;
	}
	ECN_DBG_PUT_2("_ecn_tsk_eoj_set_get() eoj:%06X _ecn_tsk_eoj_get_main() ok sw_ok:%s",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
		(a_sw_ok ? "true" : "false"));

	/* プロパティ読み込み件数書き込み時のヘッド情報で、読み出し件数を記録 */
	_ecn_fbs_poke(a_fbs_res, a_rdprp_wrpos, ((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc);

	/* 記録しておいたプロパティ件数（書き込み件数）を書き戻す */
	((T_EDATA *)a_fbs_res.ptr)->hdr.edata.opc = (uint8_t)i;

	/* 応答要の場合 */
	if (a_sw_ok) {
		/* 設定処理成功 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_res_esv;
	} else {
		/* 設定処理失敗 */
		((T_EDATA *)a_fbs_res.ptr)->hdr.edata.esv = fa_sna_esv;
	}
	/* 応答送信 */
	a_ret = _ecn_tsk_snd_dtq(a_fbs_res, false);
	if (a_ret == E_OK)
		goto lb_finally;

lb_except:
	/* データ送信失敗したら領域解放 */
	if (a_fbs_res.ptr)
		_ecn_fbs_del(a_fbs_res);

lb_finally:
	return;
}

static void _ecn_tsk_eoj_set_get_res(const EOBJINIB *fp_obj, ATR fa_eobjatr,
	bool_t fa_fromapp, ECN_FBS_ID fa_fbs_id, ECN_FBS_ID *fa_fbs_anno)
{
	T_ECN_EDT_HDR	const *p_req_esv = &((T_EDATA *)fa_fbs_id.ptr)->hdr;
	T_ECN_EDT_HDR	a_ecn_hdp;
	ECN_FBS_ID		a_fbs_dmy = { 0 };
	ER		a_ret;
	int		i, a_size, a_rdlen;
	int		a_sw_ok = 1;
	int		a_count = ((T_EDATA *)fa_fbs_id.ptr)->hdr.edata.opc;	/* 処理プロパティ数 */

	memset(&a_ecn_hdp, 0, sizeof(a_ecn_hdp));

	ECN_DBG_PUT_1("_ecn_tsk_eoj_set_get_res() eoj:%06X",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3);

	/* 初期取得サイズ */
	a_size = sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY) + sizeof(T_ECN_PRP);

	/* 応答電文用メモリ（ダミー）の取得 */
	a_ret = _ecn_fbs_cre(a_size, &a_fbs_dmy);
	if (a_ret != E_OK || !a_fbs_dmy.ptr) /* 確保失敗 */
		goto lb_finally;

	/* 応答電文設定						*/
	_ecn_tsk_mk_rsp_hdr_res(&a_ecn_hdp, p_req_esv);
	a_size -= sizeof(T_ECN_HDR) + sizeof(T_ECN_EDATA_BODY);

	/* payload先頭のT_ECN_HDR(ecn_hdr), T_ECN_EDATA_BODY(edata)を読み飛ばす */
	a_ret = _ecn_fbs_set_rpos(fa_fbs_id, offsetof(T_ECN_EDT_HDR, ecn_prp));
	if (a_ret)
		goto lb_except;

	/* プロパティ値書き込み応答読み飛ばし実行 */
	a_ret = _ecn_tsk_eoj_skip_main(fp_obj, fa_fbs_id, a_count);
	if (a_ret) {
		syslog(LOG_WARNING, "_ecn_tsk_eoj_set_get_res() eoj:%06X _ecn_tsk_eoj_set_main() fault. result = %d",
			fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
			a_ret);
		goto lb_except;
	}
	ECN_DBG_PUT_2("_ecn_tsk_eoj_set_get_res() eoj:%06X _ecn_tsk_eoj_set_main() ok sw_ok:%s",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
		(a_sw_ok ? "true" : "false"));

	/* 次の件数を読み取る(1byte)						*/
	/* → ECHONET-Lite_Ver.1.10_02.pdf p.40 [OPCGet]	*/
	a_rdlen = i = 0;
	a_ret = _ecn_fbs_get_data(fa_fbs_id, &i, 1, &a_rdlen);
	if (a_ret < 0 || a_rdlen < 1)
		goto lb_except;	/* NG */
	a_count = *(uint8_t *)&i;

	/* プロパティ値読み出し応答の書き込み実行 */
	a_ret = _ecn_tsk_eoj_set_main(fp_obj, fa_eobjatr, fa_fromapp, true, fa_fbs_id, a_fbs_dmy,
		fa_fbs_anno, a_count, &a_sw_ok);
	if (a_ret) {
		syslog(LOG_WARNING, "%s _ecn_tsk_eoj_set_get_res() eoj:%06X _ecn_tsk_eoj_get_main() fault. result = %d",
			ECN_SRC_LOC,
			fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
			a_ret);
		goto lb_except;
	}
	ECN_DBG_PUT_2("_ecn_tsk_eoj_set_get_res() eoj:%06X _ecn_tsk_eoj_get_main() ok sw_ok:%s",
		fp_obj->eojx1 << 16 | fp_obj->eojx2 << 8 | fp_obj->eojx3,
		(a_sw_ok ? "true" : "false"));

	goto lb_finally;

lb_except:
	/* データ送信失敗したら領域解放 */
	if (a_fbs_dmy.ptr)
		_ecn_fbs_del(a_fbs_dmy);

lb_finally:
	return;
}

/*
 * 応答電文待ちの割り込み電文作成
 */
ER _ecn_mk_brk_wai(ECN_FBS_ID *pk_fbs_id, const void *p_dat, size_t fa_size)
{
	ER				a_ret;
	ECN_FBS_ID		a_fbs_id = { 0 };	/* 要求電文用メモリ	*/

	if (!pk_fbs_id)
		return E_PAR;
	*pk_fbs_id = a_fbs_id;
	if (sizeof(a_fbs_id.ptr->payload) <= fa_size)
		return E_PAR; /* 付随データが大きすぎる */
	if (!p_dat && 0 < fa_size)
		return E_PAR; /* 付随データサイズが1以上なのにデータポインタがNULL */

	/* 要求電文用メモリの取得 */
	a_ret = _ecn_fbs_cre((0 < fa_size ? fa_size : 1), &a_fbs_id);
	if (a_ret != E_OK || !a_fbs_id.ptr) { /* 確保失敗 */
		syslog(LOG_WARNING, "_ecn_fbs_cre(%d) result = %d:%s", fa_size, a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	/* 要求電文設定 */
	a_fbs_id.ptr->hdr.type = ECN_MSG_USER_BREAK;
	a_fbs_id.ptr->hdr.sender.id = ENOD_API_ID;
	a_fbs_id.ptr->hdr.target.id = ENOD_API_ID;
	a_fbs_id.ptr->hdr.reply.id = ENOD_API_ID;

	if (0 < fa_size && p_dat) {
		/* 付随データ追加 */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_id, p_dat, fa_size);
		if (a_ret) {
			syslog(LOG_WARNING, "_ecn_fbs_add_data_ex(*, dat, %lu) result = %d:%s",
				fa_size,
				a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
	}

	*pk_fbs_id = a_fbs_id;

	/* 正常終了 */
	return a_ret;

lb_except:
	/* データ作成失敗したら領域解放 */
	if (a_fbs_id.ptr)
		_ecn_fbs_del(a_fbs_id);

	return a_ret;
}

/* 応答電文用fbs設定(sender/targetの設定) */
ER _ecn_tsk_cre_req_fbs(ID sender, uint8_t cmd, ECN_FBS_ID *pk_req)
{
	ER ret;
	ECN_FBS_ID req;

	ret = _ecn_fbs_cre(1, &req);
	if (ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_tsk_cre_req_fbs() : _ecn_fbs_cre() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	ret = _ecn_fbs_add_data(req, &cmd, sizeof(cmd));
	if (ret != E_OK) {
		_ecn_fbs_del(req);
		ECN_DBG_PUT_2("_ecn_tsk_cre_req_fbs() : _ecn_fbs_add_data() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	req.ptr->hdr.type = ECN_MSG_INTERNAL;
	req.ptr->hdr.sender.dtqid = sender;
	req.ptr->hdr.target.dtqid = ecn_svc_dataqueueid;
	req.ptr->hdr.reply.dtqid = sender;

	*pk_req = req;

	return E_OK;
}

/* 応答電文用fbs設定(sender/targetの設定) */
ER _ecn_tsk_cre_res_fbs(ECN_FBS_ID req, uint8_t cmd, ECN_FBS_ID *pk_res)
{
	ER ret;
	ECN_FBS_ID res;

	ret = _ecn_fbs_cre(1, &res);
	if (ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_tsk_cre_res_fbs() : _ecn_fbs_cre() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	ret = _ecn_fbs_add_data(res, &cmd, sizeof(cmd));
	if (ret != E_OK) {
		_ecn_fbs_del(res);
		ECN_DBG_PUT_2("_ecn_tsk_cre_res_fbs() : _ecn_fbs_add_data() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	res.ptr->hdr.type = ECN_MSG_INTERNAL;
	res.ptr->hdr.sender.dtqid = ecn_svc_dataqueueid;
	res.ptr->hdr.target.dtqid = req.ptr->hdr.reply.dtqid;
	res.ptr->hdr.reply.dtqid = ecn_svc_dataqueueid;

	*pk_res = res;

	return E_OK;
}
