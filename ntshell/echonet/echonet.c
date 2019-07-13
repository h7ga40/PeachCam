/*
 *  TOPPERS ECHONET Lite Communication Middleware
 * 
 *  Copyright (C) 2014-2019 Cores Co., Ltd. Japan
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
 *  @(#) $Id: echonet.c 1971 2019-07-10 04:36:59Z coas-nagasima $
 */

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <t_stdlib.h>

#include "echonet.h"
#include "echonet_fbs.h"
#include "echonet_task.h"
#include "echonet_dbg.h"
#include "echonet_agent.h"

/*
 * ECHONET Liteサービス処理開始
 */
ER ecn_sta_svc()
{
	return act_tsk(ecn_udp_taskid);
}

/*
 * インスタンスリスト通知の送信
 */
ER ecn_ntf_inl()
{
	return _ecn_tsk_ntf_inl();
}

/*
 * ECHONETオブジェクト参照
 * 引数
 * ID		fa_eobjid	ECHONETオブジェクトID
 * T_REOBJ	*fp_eobj	ECHONETオブジェクトの設定内容コピー先
 *
 * eobjidで指定したECHONETオブジェクトの設定内容を参照する。
 * 参照した設定内容はpk_eobjに指定したメモリに返される。
 */
ER ecn_ref_eobj(ID fa_eobjid, T_REOBJ *fp_eobj)
{
	const EOBJINIB	*p_obj;

	if (!fp_eobj)
		return E_PAR;	/* パラメータエラー */

	if ((fa_eobjid <= 0) || (fa_eobjid > tmax_eobjid))
		return E_OBJ;	/* オブジェクト未登録 */

	p_obj = &eobjinib_table[fa_eobjid - 1];

	*fp_eobj = *p_obj;

	return E_OK;
}

/*
 * ECHONETプロパティ参照
 * 引数
 * ID			fa_eobjid	ECHONETオブジェクトID
 * uint8_t		fa_epc		プロパティコード
 * T_RPRP		*fp_eprp	ECHONETオブジェクトのプロパティ設定内容コピー先
 *
 * eobjidで指定したECHONETオブジェクトの、epcで指定したプロパティの設定内容を参照する。
 * 参照した設定内容はpk_eprpに指定したメモリに返される。
 */
ER ecn_ref_eprp(ID fa_eobjid, uint8_t fa_epc, T_RPRP *fp_eprp)
{
	const EOBJINIB	*p_obj;
	const EPRPINIB	*p;
	uint_t			i;

	if (!fp_eprp)
		return E_PAR;	/* パラメータエラー */

	if ((fa_eobjid <= 0) || (fa_eobjid > tmax_eobjid))
		return E_OBJ;	/* オブジェクト未登録 */

	p_obj = &eobjinib_table[fa_eobjid - 1];

	/* eojの持つプロパティ初期化定義配列から検索 */
	p = p_obj->eprp;
	for (i = 0; i < p_obj->eprpcnt; i++, p++) {
		if (p->eprpcd != fa_epc)
			continue;
		*fp_eprp = *p;
		return E_OK;
	}

	return E_PAR;	/* パラメータエラー */
}

/*
 * プロパティ値書き込み要求（応答不要）電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * uint8_t			fa_epc		プロパティコード
 * uint8_t			fa_pdc		プロパティ値データサイズ
 * const void		*p_edt		プロパティ値データ
 *
 *	プロパティ値書き込み要求（応答不要）電文を作成する。
 *	電文はdeojで指定したECHONETオブジェクトを宛先とし、
 *	電文の初めのプロパティ値として、epcとpdc、p_edtで指定された
 *	プロパティコード、データサイズ、データを電文に設定する。
 *	作成した電文の先頭アドレスはポインタ経由で返される。
 */
ER ecn_esv_seti(T_EDATA **ppk_esv, ID fa_deoj, uint8_t fa_epc, uint8_t fa_pdc,
	const void *p_edt)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, (ID)1, fa_deoj, fa_epc, fa_pdc, p_edt, ESV_SET_I);
}

/*
 * プロパティ値書き込み要求（応答要）電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * uint8_t			fa_epc		プロパティコード
 * uint8_t			fa_pdc		プロパティ値データサイズ
 * const void		*p_edt		プロパティ値データ
 *
 *	プロパティ値書き込み要求（応答要）電文を作成する。
 *	電文はdeojで指定したECHONETオブジェクトを宛先とし、
 *	電文の初めのプロパティ値として、epcとpdc、p_edtで指定された
 *	プロパティコード、データサイズ、データを電文に設定する。
 *	作成した電文の先頭アドレスはポインタ経由で返される。
 */
ER ecn_esv_setc(T_EDATA **ppk_esv, ID fa_deoj, uint8_t fa_epc, uint8_t fa_pdc,
	const void *p_edt)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, (ID)1, fa_deoj, fa_epc, fa_pdc, p_edt, ESV_SET_C);
}

/*
 * プロパティ値読み出し要求電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * uint8_t			fa_epc		プロパティコード
 *
 * プロパティ値読み出し要求電文を作成する。
 * 電文はdeojで指定したECHONETオブジェクトを宛先とし、
 * 電文の初めのプロパティ値として、epcで指定された
 * プロパティコードを電文に設定する。
 * 作成した電文の先頭アドレスはポインタ経由で返される。
 */
ER ecn_esv_get(T_EDATA **ppk_esv, ID fa_deoj, uint8_t fa_epc)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, (ID)1, fa_deoj, fa_epc, 0, NULL, ESV_GET);
}

/*
 * プロパティ値通知要求電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * uint8_t			fa_epc		プロパティコード
 *
 * プロパティ値通知要求電文を作成する。
 * 電文はdeojで指定したECHONETオブジェクトを宛先とし、
 * 電文の初めのプロパティ値として、epcで指定された
 * プロパティコードを電文に設定する。
 * 作成した電文の先頭アドレスはポインタ経由で返される。
 */
ER ecn_esv_inf_req(T_EDATA **ppk_esv, ID fa_deoj, uint8_t fa_epc)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, (ID)1, fa_deoj, fa_epc, 0, NULL, ESV_INF_REQ);
}

/*
 * プロパティ値書き込み・読み出し要求電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * uint8_t			fa_epc		プロパティコード
 * uint8_t			fa_pdc		プロパティ値データサイズ
 * const void		*p_edt		プロパティ値データ
 *
 * プロパティ値書き込み・読み出し要求電文を作成する。
 * 電文はdeojで指定したECHONETオブジェクトを宛先とし、
 * 電文の初めのプロパティ値として、epcとpdc、p_edtで指定された
 * プロパティコード、データサイズ、データを電文に設定する。
 * 作成した電文の先頭アドレスはポインタ経由で返される。
 */
ER ecn_esv_set_get(T_EDATA **ppk_esv, ID fa_deoj, uint8_t fa_epc, uint8_t fa_pdc,
	const void *p_edt)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, (ID)1, fa_deoj, fa_epc, fa_pdc, p_edt, ESV_SET_GET);
}

/*
 * プロパティ値書き込み・読み出し要求電文作成：折り返し指定
 * プロパティ値書き込み・読み出し要求電文作成中の、書き込みプロパティ配列を終えて
 * 読み出しプロパティ配列に移る時に用いる。
 */
ER ecn_trn_set_get(T_EDATA *ppk_esv, int *p_trn_pos)
{
	ER	a_ret;

	if (!ppk_esv)
		return E_PAR;
	if (!p_trn_pos)
		return E_PAR;

	/* プロパティ読み込み件数書き込み時のヘッド情報を記録 */
	*p_trn_pos = _ecn_fbs_get_datalen(*(ECN_FBS_ID *)&ppk_esv);

	/* 応答電文用メモリにデータ追加・この時点での応答電文中プロパティ件数を記録 */
	a_ret = _ecn_fbs_add_data_ex(*(ECN_FBS_ID *)&ppk_esv, &ppk_esv->hdr.edata.opc, 1);
	if (a_ret) {
		ECN_DBG_PUT_2("ecn_trn_set_get(): _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	/* 応答電文中プロパティ件数を 0 に戻す（今後、終了指定するまでは読み出しプロパティ件数になる） */
	ppk_esv->hdr.edata.opc = 0;

	return E_OK;
}

/*
 * プロパティ値書き込み・読み出し要求電文作成：終了指定
 * プロパティ値書き込み・読み出し要求電文作成中の、読み出しプロパティ配列を終える時に用いる。
 */
ER ecn_end_set_get(T_EDATA *ppk_esv, int fa_trn_pos)
{
	ER		a_ret;
	int		a_wr_opc;
	uint8_t	a_rd_opc;

	if (!ppk_esv)
		return E_PAR;

	/* この時点での応答電文中プロパティ件数(読み出しプロパティ件数)を保持 */
	a_rd_opc = ppk_esv->hdr.edata.opc;

	/* 応答電文中プロパティ件数(書き込みプロパティ件数)を保存位置から読み取り */
	a_wr_opc = _ecn_fbs_peek(*(ECN_FBS_ID *)&ppk_esv, fa_trn_pos);
	if (a_wr_opc < 0) {
		ECN_DBG_PUT_2("ecn_end_set_get(): _ecn_fbs_peek() result = %d:%s", a_wr_opc, itron_strerror(a_wr_opc));
		return a_wr_opc;
	}

	/* 応答電文中プロパティ件数(書き込みプロパティ件数)を復元 */
	ppk_esv->hdr.edata.opc = (uint8_t)a_wr_opc;

	/* 応答電文中プロパティ件数(読み出しプロパティ件数)を保存位置に書き込み */
	a_ret = _ecn_fbs_poke(*(ECN_FBS_ID *)&ppk_esv, fa_trn_pos, a_rd_opc);
	if (a_ret) {
		ECN_DBG_PUT_2("ecn_end_set_get(): _ecn_fbs_poke() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	return E_OK;
}

/*
 * プロパティ値通知（応答要）電文作成
 * 引数
 * T_EDATA			**ppk_esv	取得した電文の格納先
 * ID				fa_deoj		宛先のECHONETオブジェクトID
 * ID				fa_seoj		送信元のECHONETオブジェクトID
 * uint8_t			fa_sepc		送信元のプロパティコード
 */
ER ecn_esv_infc(T_EDATA **ppk_esv, ID fa_deoj, ID fa_seoj, uint8_t fa_sepc)
{
	return _ecn_tsk_mk_esv((ECN_FBS_ID *)ppk_esv, fa_seoj, fa_deoj, fa_sepc, 0, NULL, ESV_INFC);
}

/*
 * 要求電文へのプロパティ指定追加 (プロパティデータが付随しない場合に用いる)
 */
ER ecn_add_epc(T_EDATA *pk_esv, uint8_t epc)
{
	return ecn_add_edt(pk_esv, epc, 0, 0);
}

/*
 * 要求電文へのプロパティデータ追加 (プロパティおよび付随データを追加する)
 */
ER ecn_add_edt(T_EDATA *pk_esv, uint8_t fa_epc, uint8_t fa_pdc, const void *p_edt)
{
	ECN_FBS_ID	a_fbs_id;
	ER			a_ret;
	T_ECN_PRP	a_ecn_prp;

	if (!pk_esv)
		return E_PAR; /* 取得したFBS_IDの格納先がNULL */
	if (ECHONET_MEMPOOL_BLOCK_SIZE <= fa_pdc)
		return E_PAR; /* プロパティ値サイズが大きすぎる */
	if (!p_edt && 0 < fa_pdc)
		return E_PAR; /* プロパティ値サイズが1以上なのにデータポインタがNULL */

	a_fbs_id.ptr = (T_ECN_FST_BLK *)pk_esv;
	if (((T_EDATA *)a_fbs_id.ptr)->hdr.edata.opc == 0xFF)
		return E_PAR; /* プロパティが多すぎる */

	/* 要求電文用メモリにデータ追加 */
	memset(&a_ecn_prp, 0, sizeof(a_ecn_prp));
	a_ecn_prp.epc = fa_epc;
	a_ecn_prp.pdc = fa_pdc;
	a_ret = _ecn_fbs_add_data_ex(a_fbs_id, &a_ecn_prp, sizeof(a_ecn_prp));
	if (a_ret) {
		ECN_DBG_PUT_4("ecn_add_edt(): _ecn_fbs_add_data_ex(*, ecn_prp{epc:0x%02X}, %u) result = %d:%s",
			a_ecn_prp.epc, sizeof(a_ecn_prp),
			a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	if (0 < fa_pdc) {
		/* 付随データ追加 */
		a_ret = _ecn_fbs_add_data_ex(a_fbs_id, p_edt, fa_pdc);
		if (a_ret) {
			ECN_DBG_PUT_4("ecn_add_edt(): _ecn_fbs_add_data_ex(*, ecn_prp{epc:0x%02X} edt, %u) result = %d:%s",
				a_ecn_prp.epc, fa_pdc,
				a_ret, itron_strerror(a_ret));
			goto lb_except;
		}
	}
	/* プロパティ数インクリメント */
	((T_EDATA *)a_fbs_id.ptr)->hdr.edata.opc++;

lb_except:
	return a_ret;
}

/*
 * 要求電文の送信
 * esvで指定された要求電文を送信する。
 * 電文に指定された宛先からIPアドレスを特定し、UDPパケットとして送信する。
 */
ER ecn_snd_esv(T_EDATA *pk_esv)
{
	ECN_FBS_ID a_rsp_fbs;
	a_rsp_fbs.ptr = (T_ECN_FST_BLK *)pk_esv;
	return _ecn_tsk_snd_dtq(a_rsp_fbs, true);
}

/*
 * 応答電文の受信永遠待ち
 * 応答電文を受信するのを待つ。
 * 受信した応答電文はppk_esvで指定したポインターに先頭アドレスが返される。
 * 要求電文の宛先のECHONETオブジェクトがネットワークに存在しない場合、
 * 応答電文は返送されないので、永遠に待つことになるのでタイムアウト付きの
 * ecn_trcv_esvを使用することを推奨する。
 */
ER ecn_rcv_esv(T_EDATA **ppk_esv)
{
	return ecn_trcv_esv(ppk_esv, TMO_FEVR);
}

/*
 * 応答電文の受信待ちタイムアウトあり
 * 応答電文を受信するのを待つ。
 * 受信した応答電文はppk_esvで指定したポインターに先頭アドレスが返される。
 * tmoで指定されたタイムアウト時間が経過しても応答電文を受信しない場合、
 * 待ちが解除され、戻り値がE_TMOで返される。
 * tmoにTMO_FEVRを指定するとecn_rcv_esvと同じ振る舞いをする。
 * tmoに0を指定するとecn_prcv_esvと同じ振る舞いをする。
 */
ER ecn_trcv_esv(T_EDATA **ppk_esv, int fa_tmout)
{
	T_MSG	*p_msg = 0;
	ER		a_ret;

	if (!ppk_esv)
		return E_PAR;

	a_ret = trcv_dtq(ecn_api_dataqueueid, (intptr_t *)&p_msg, fa_tmout);
	if (a_ret != E_OK) {
		*ppk_esv = NULL;
		return a_ret;
	}

	*ppk_esv = (T_EDATA *)p_msg;
	return (((T_ECN_FST_BLK *)p_msg)->hdr.type == ECN_MSG_ECHONET) ? E_OK : E_BRK;
}

/*
 * 応答電文の受信ポーリング
 * 応答電文を受信するのを待つ。
 * 受信した応答電文はppk_esvで指定したポインターに先頭アドレスが返される。
 * 応答電文の受信の有無にかかわらず、待たずに関数を抜ける。
 */
ER ecn_prcv_esv(T_EDATA **ppk_esv)
{
	return ecn_trcv_esv(ppk_esv, TMO_POL);
}

/*
 * 応答電文の破棄
 */
ER ecn_rel_esv(T_EDATA *pk_esv)
{
	ECN_FBS_ID	a_fbs;
	ER			a_ret;

	a_fbs.ptr = (T_ECN_FST_BLK *)pk_esv;
	a_ret = _ecn_fbs_del(a_fbs);
	return a_ret;
}

/*
 * 応答電文の送信元ノードを取得する
 */
ID ecn_get_enod(T_EDATA *pk_esv)
{
	const EOBJCB	*p_snod;
	const EOBJINIB	*p_sobj = NULL;

	p_snod = _ecn_eno_fnd(((T_ECN_FST_BLK *)pk_esv)->hdr.sender.id);
	if (p_snod != NULL) {
		p_sobj = p_snod->profile;
	}

	if (p_sobj == NULL)
		return EOBJ_NULL;
#ifdef ECHONET_CONTROLLER_EXTENTION
	if (p_sobj->eprpcnt == 0)
		return ecn_agent_get_eobj(p_sobj);
#endif
	return 1 + (((intptr_t)p_sobj - (intptr_t)eobjinib_table) / sizeof(EOBJINIB));
}

/*
 * 応答電文の送信元機器オブジェクトを取得する
 */
ID ecn_get_eobj(T_EDATA *pk_esv)
{
	const EOBJCB	*p_snod;
	const EOBJINIB	*p_sobj = NULL;
	T_ECN_EDT_HDR	*p_esv;

	p_esv = &pk_esv->hdr;
	p_snod = _ecn_eno_fnd(((T_ECN_FST_BLK *)pk_esv)->hdr.sender.id);
	if (p_snod != NULL) {
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

	if (p_sobj == NULL)
		return EOBJ_NULL;
#ifdef ECHONET_CONTROLLER_EXTENTION
	if (p_sobj->eprpcnt == 0)
		return ecn_agent_get_eobj(p_sobj);
#endif
	return 1 + (((intptr_t)p_sobj - (intptr_t)eobjinib_table) / sizeof(EOBJINIB));
}

/*
 * 応答電文解析イテレーター初期化
 */
ER ecn_itr_ini(T_ENUM_EPC *pk_itr, T_EDATA *pk_esv)
{
	if (!pk_itr)
		return E_PAR;
	if (!pk_esv)
		return E_PAR;

	memset(pk_itr, 0, sizeof(*pk_itr));
	pk_itr->pk_esv = pk_esv;
	pk_itr->count = pk_esv->hdr.edata.opc;
								/* 今のブロックでのプロパティ総数 */
	pk_itr->got_ct = 0;			/* 今のブロックで、読み取った数 */
	pk_itr->next_blk_ct = 0;	/* 後続ブロック数 */
	pk_itr->is_eof = 0;			/* 終端に達した時、非0 */
	pk_itr->cur = offsetof(T_ECN_EDT_HDR, ecn_prp);

	switch (pk_esv->hdr.edata.esv) {
	case ESV_SET_GET:		/* 0x6E プロパティ値書き込み・読み出し要求		*/
	case ESV_SET_GET_RES:	/* 0x7E プロパティ値書き込み・読み出し応答		*/
	case ESV_SET_GET_SNA:	/* 0x5E	プロパティ値書き込み・読み出し不可応答	*/
		pk_itr->next_blk_ct ++;
	default:
		break;
	}

	return E_OK;
}

/*
 * 応答電文解析イテレーターインクリメント
 */
ER ecn_itr_nxt(T_ENUM_EPC *pk_itr, uint8_t *p_epc, uint8_t *p_pdc, void *p_edt)
{
	ECN_FBS_ID		a_fbs_id;
	int				a_rd_bak;
	ER				a_ret;
	ECN_FBS_SSIZE_T	a_rd_len;
	T_ECN_PRP		a_ecn_prp;

	if (!pk_itr)
		return E_PAR;
	if (!p_epc)
		return E_PAR;
	if (!p_pdc)
		return E_PAR;
	if (!pk_itr->pk_esv)
		return E_PAR;

	if (pk_itr->is_eof)
		return E_BOVR; /* データ終了 */
	if (pk_itr->count <= pk_itr->got_ct && pk_itr->next_blk_ct < 1) {
		pk_itr->is_eof = 1;			/* 終端に達した時、非0 */
		return E_BOVR; /* データ終了 */
	}

	a_fbs_id.ptr = (T_ECN_FST_BLK *)pk_itr->pk_esv;

	/* 元のカーソル位置を保存し、イテレータのカーソル位置にする */
	a_rd_bak = a_fbs_id.ptr->hdr.rd;
	a_fbs_id.ptr->hdr.rd = pk_itr->cur;

#ifdef ECN_ENA_ITR_NXT_CARP
	ECN_CAP_PUT("ecn_itr_nxt() rd.cur=b%dp%d", pk_itr->cur.blk, pk_itr->cur.pos ECN_CAP_PUT(;
#endif
	if (pk_itr->count <= pk_itr->got_ct && 0 < pk_itr->next_blk_ct) {
		/* 次ブロックに移動 */
		pk_itr->next_blk_ct --;
		pk_itr->count = 0;		/* 今のブロックでのプロパティ総数 */
		pk_itr->got_ct = 0;		/* 今のブロックで、読み取った数 */

		/* 次ブロックのプロパティ数を読み取る */
		a_rd_len = 0;
		a_ret = _ecn_fbs_get_data(a_fbs_id, &a_ecn_prp.pdc, 1, &a_rd_len);
		if (a_ret != E_OK)
			goto lb_except;
		if (0 < a_rd_len) {
			pk_itr->count = a_ecn_prp.pdc;	/* 今のブロックでのプロパティ総数 */
		}
		a_ret = E_BOVR;	/* データ終了 */
		goto lb_finally;
	}

	/* プロパティコードとデータサイズを読み取る */
	memset(&a_ecn_prp, 0, sizeof(a_ecn_prp));
	a_rd_len = 0;
	a_ret = _ecn_fbs_get_data(a_fbs_id, &a_ecn_prp, sizeof(a_ecn_prp), &a_rd_len);
	if (a_ret != E_OK)
		goto lb_except;
	if (a_rd_len < sizeof(a_ecn_prp)) {
		ECN_DBG_PUT_1("ecn_itr_nxt() ecn_prp read fault. rd.cur=%d", pk_itr->cur);
		pk_itr->is_eof = 1;			/* 終端に達した時、非0 */
		a_ret = E_BOVR;	/* データ終了 */
		goto lb_finally;
	}
	*p_epc = a_ecn_prp.epc;
	*p_pdc = a_ecn_prp.pdc;

	if (0 < a_ecn_prp.pdc) {
		if (p_edt == NULL) {
			a_ret = _ecn_fbs_seek_rpos(a_fbs_id, a_ecn_prp.pdc);
			if (a_ret != E_OK)
				goto lb_except;
		}
		else {
			/* 付随データを読み取る */
			a_rd_len = 0;
			a_ret = _ecn_fbs_get_data(a_fbs_id, p_edt, a_ecn_prp.pdc, &a_rd_len);
			if (a_ret != E_OK)
				goto lb_except;
			if (a_rd_len < (ECN_FBS_SSIZE_T)a_ecn_prp.pdc) {
				ECN_DBG_PUT_3("ecn_itr_nxt() edt read fault. rd.cur=%d,epc=0x%02X,pdc=%u",
					pk_itr->cur, a_ecn_prp.epc , a_ecn_prp.pdc);
				pk_itr->is_eof = 1;			/* 終端に達した時、非0 */
				a_ret = E_BOVR;	/* データ終了 */
				goto lb_finally;
			}
		}
	}
	pk_itr->got_ct++;

#ifdef ECN_ENA_ITR_NXT_CARP
	ECN_CAP_PUT("ecn_itr_nxt() read: ct=%d/%d", pk_itr->got_ct, pk_itr->count ECN_CAP_PUT(;
#endif

lb_finally:
	/* イテレータのカーソル位置を更新 */
	pk_itr->cur = a_fbs_id.ptr->hdr.rd;

lb_except:
	/* 元のカーソル位置に戻す */
	a_fbs_id.ptr->hdr.rd = a_rd_bak;
	return a_ret;
}

/*
 * 応答電文待ちの割り込み送信
 */
ER ecn_brk_wai(const void *p_dat, int fa_datsz)
{
	ER				a_ret;
	ECN_FBS_ID		a_fbs_id = { 0 };	/* 要求電文用メモリ	*/

	/* 応答電文待ちの割り込み電文作成 */
	a_ret = _ecn_mk_brk_wai(&a_fbs_id, p_dat, fa_datsz);
	if (a_ret)
		return a_ret;

	/* 割り込み送信 */
	a_ret = _ecn_tsk_snd_dtq(a_fbs_id, true);
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_tsk_snd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}

	/* 正常終了 */
	return a_ret;

lb_except:
	/* データ作成失敗したら領域解放 */
	if (a_fbs_id.ptr)
		_ecn_fbs_del(a_fbs_id);

	return a_ret;
}

/*
 * 割り込みデータの取得
 */
ER ecn_get_brk_dat(T_EDATA *pk_esv, void *p_buf, int fa_bufsz, int *p_datsz)
{
	ECN_FBS_ID	a_fbs;
	ER			a_ret;
	ECN_FBS_SSIZE_T		a_len;

	if (p_datsz)
		*p_datsz = 0;
	if (!pk_esv)
		return E_PAR;

	a_fbs.ptr = (T_ECN_FST_BLK *)pk_esv;
	if (a_fbs.ptr->hdr.type != ECN_MSG_USER_BREAK) {
		ECN_DBG_PUT_1("ecn_get_brk_dat: fbs-type:%d != ECN_MSG_USER_BREAK",
			a_fbs.ptr->hdr.type);
		return E_PAR;
	}

	/* 読み取り位置を先頭に戻す */
	a_ret = _ecn_fbs_set_rpos(a_fbs, 0);
	if (a_ret) {
		ECN_DBG_PUT_2("_ecn_fbs_set_rpos(*, 0) result = %d:%s",
			a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	/* 付随データを読み込む */
	if (p_buf && 0 < fa_bufsz) {
		a_len = 0;
		a_ret = _ecn_fbs_get_data(a_fbs, p_buf, fa_bufsz, &a_len);
		if (a_ret || !a_len) {
			ECN_DBG_PUT_2("_ecn_fbs_get_data(*, p_buf, fa_bufsz) result = %d:%s",
				a_ret, itron_strerror(a_ret));
			return E_PAR;
		}
		*p_datsz = a_len;
	}

	return E_OK;
}

#ifndef ECN_USER_DATA_PROP_SET
/*
 * データ設定関数
 */
int ecn_data_prop_set(const EPRPINIB *item, const void *src, int size, bool_t *anno)
{
	if (size > item->eprpsz)
		size = item->eprpsz;

	if (*anno)
		*anno = memcmp((uint8_t *)item->exinf, src, size) != 0;

	memcpy((uint8_t *)item->exinf, src, size);

	return size;
}
#endif

#ifndef ECN_USER_DATA_PROP_GET
/*
 * データ取得関数
 */
int ecn_data_prop_get(const EPRPINIB *item, void *dst, int size)
{
	if (size > item->eprpsz)
		size = item->eprpsz;

	memcpy(dst, (uint8_t *)item->exinf, size);

	return size;
}
#endif
