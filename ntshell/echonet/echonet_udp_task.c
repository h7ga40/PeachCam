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
 *		ECHONET Lite UDP通信処理タスク
 */
#ifdef SUPPORT_INET4

#include <kernel.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include <sil.h>

#include "syssvc/serial.h"
#include "syssvc/syslog.h"
#include "echonet.h"

#include <tinet_config.h>
#include <tinet_defs.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/net.h>
#include <netinet/in_itron.h>
#include <netinet/udp_var.h>
#include <netinet/ip_igmp.h>

#include "echonet_task.h"
#include "echonet_lcl_task.h"
#include "echonet_dbg.h"

#ifndef ECHONET_UDP_TASK_GET_TIMER
#define ECHONET_UDP_TASK_GET_TIMER	TMO_FEVR
#endif /* ECHONET_UDP_TASK_GET_TIMER */

#ifndef ECHONET_UDP_TASK_PROGRESS
#define ECHONET_UDP_TASK_PROGRESS(timer)
#endif /* ECHONET_UDP_TASK_PROGRESS */

#ifndef ECHONET_UDP_TASK_TIMEOUT
#define ECHONET_UDP_TASK_TIMEOUT
#endif /* ECHONET_UDP_TASK_TIMEOUT */

#ifndef ECHONET_UDP_SEND_TO_MBX
#define ECHONET_UDP_SEND_TO_MBX
#endif /* ECHONET_UDP_SEND_TO_MBX */

#ifndef ECHONET_NODE_MATCH
#define ECHONET_NODE_MATCH(enodcb, edata, ipaddr, portno) is_match(enodcb, edata, ipaddr, portno)
#endif /* ECHONET_NODE_MATCH */

static ECN_ENOD_ID udp_get_id(T_EDATA *edata, const T_IN4_ADDR ipaddr, uint16_t portno);
static int udp_get_ip(T_IPV4EP *fp_ipep, ECN_ENOD_ID fa_enodid);
void _ecn_int_msg(ECN_FBS_ID fbs_id, ECN_FBS_SSIZE_T a_snd_len);
void _ecn_esv_msg(ECN_FBS_ID fbs_id);

/*
 * 受信したUDPデータをMAILBOXに送る
 */
ER _ecn_udp2dtq(const uint8_t *buffer, size_t fa_len, const T_IPV4EP *dst);
ER _ecn_udp2dtq(const uint8_t *buffer, size_t fa_len, const T_IPV4EP *dst)
{
	ECN_FBS_ID	a_fbs_id = { 0 };
	ER			a_ret = E_OK;
	ECN_ENOD_ID	a_enod_id;
	union
	{
		const uint8_t			*buffer;
		const T_ECN_EDT_HDR		*t_esv;
	} a_rcv_pkt;

	a_rcv_pkt.buffer = buffer;
	if (	a_rcv_pkt.t_esv->ecn_hdr.ehd1 != ECN_EDH1_ECHONET_LITE	/* ECHONET Lite規格	*/
	||	a_rcv_pkt.t_esv->ecn_hdr.ehd2 != ECN_EDH2_FORMAT_1			/* 電文形式1		*/) {
		ECN_DBG_PUT_2("[UDP ECHO SRV] illegal type (0x%02X,0x%02X)", a_rcv_pkt.t_esv->ecn_hdr.ehd1, a_rcv_pkt.t_esv->ecn_hdr.ehd2);
		return E_PAR;
	}

#ifdef ECN_DBG_PUT_ENA
	_ecn_dbg_bindmp(buffer, fa_len);
#endif

	a_ret = _ecn_fbs_cre(fa_len, &a_fbs_id);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("[UDP ECHO SRV] _ecn_fbs_cre() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}
	a_ret = _ecn_fbs_add_data_ex(a_fbs_id, buffer, fa_len);
	if (a_ret) {
		ECN_DBG_PUT_2("[UDP ECHO SRV] _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	a_fbs_id.ptr->hdr.type = ECN_MSG_ECHONET;
	a_fbs_id.ptr->hdr.target.id = ENOD_LOCAL_ID;
	a_fbs_id.ptr->hdr.sender.id = ENOD_NOT_MATCH_ID;

	/* IPアドレスからリモートECHONETノードへ変換 */
	a_enod_id = udp_get_id((T_EDATA *)a_fbs_id.ptr, dst->ipaddr, dst->portno);
	if (a_enod_id < 0 || tnum_enodadr <= a_enod_id) {
		ECN_DBG_PUT_1("[UDP ECHO SRV] udp src(%s) echonet-node not found.",
			ip2str(NULL, &dst->ipaddr));
	} else {
		/* 送信元ECHONETノードを記録 */
		a_fbs_id.ptr->hdr.sender.id = a_enod_id;
	}
	a_fbs_id.ptr->hdr.reply.id = a_fbs_id.ptr->hdr.sender.id;

	/* echonet_taskに送る */
	a_ret = snd_dtq(ecn_svc_dataqueueid, (intptr_t)a_fbs_id.ptr);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("[UDP ECHO SRV] snd_dtq(ecn_svc_dataqueueid) result = %d:%s", a_ret, itron_strerror(a_ret));
		goto lb_except;
	}
	return a_ret;
lb_except:
	_ecn_fbs_del(a_fbs_id);
	return a_ret;
}

/*
 *  ECHONET UDP通信処理タスク
 */
void echonet_udp_task(intptr_t exinf)
{
	T_IN4_ADDR	a_src;
	ER			a_ret, a_ret2;
	ID			tskid;
	ECN_FBS_SSIZE_T	a_snd_len;
	union
	{
		T_MSG		*p_msg;
		ECN_FBS_ID	fbs_id;
	} a_mdt;
	SYSTIM a_prev, a_now;
	int a_timer;
	T_IP_MREQ a_mreq;

	get_tid(&tskid);
	memcpy(&a_src, in4_get_ifaddr(0), sizeof(T_IN4_ADDR));
	ECN_DBG_PUT_4("[UDP TSK:%d,%d] started, IP Address: %s ether_max_len: %u",
		tskid, exinf,
		ip2str(NULL, &a_src),
		ETHER_MAX_LEN);
	memcpy(enodadrb_table[ENOD_LOCAL_ID].ipaddr, &a_src, sizeof(T_IN4_ADDR));

	a_mreq.imr_interface = IPV4_ADDRANY;
	a_mreq.imr_multiaddr = MAKE_IPV4_ADDR(224, 0, 23, 0);

	a_ret = udp_set_opt(ecn_udp_cepid, IP_ADD_MEMBERSHIP, (char *)&a_mreq, sizeof(a_mreq));
	if (a_ret != E_OK) {
		printf("setsockopt %d", a_ret);
		return;
	}

	syscall(act_tsk(ecn_svc_taskid));

	a_ret = get_tim(&a_now);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("get_tim() result = %d:%s", a_ret, itron_strerror(a_ret));
		return;
	}

	/* メッセージループ */
	for (;;) {
		a_prev = a_now;

		a_timer = ECHONET_UDP_TASK_GET_TIMER;

		a_ret = trcv_dtq(ecn_udp_dataqueueid, (intptr_t *)&a_mdt.p_msg, a_timer);
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
		ECHONET_UDP_TASK_PROGRESS(a_timer);

		if (a_ret == E_OK) {
			/* 送信データ長を取得 */
			a_snd_len = _ecn_fbs_get_datalen(a_mdt.fbs_id);

			ECN_DBG_PUT_1("[UDP TSK] trcv_dtq() dtq recv (%d byte)", _ecn_fbs_get_datalen(a_mdt.fbs_id));

			if (0 < a_snd_len) {
				switch(a_mdt.fbs_id.ptr->hdr.type){
				case ECN_MSG_INTERNAL:
					_ecn_int_msg(a_mdt.fbs_id, a_snd_len);
					break;
				case ECN_MSG_ECHONET:
					_ecn_esv_msg(a_mdt.fbs_id);
					break;
				}
			}

			_ecn_fbs_del(a_mdt.fbs_id);
		}

		ECHONET_UDP_TASK_TIMEOUT;
	}
}

/* 応答電文用fbs設定(sender/targetの設定) */
static ER _ecn_udp_cre_req_fbs(ID sender, uint8_t cmd, ECN_FBS_ID *pk_req)
{
	ER ret;
	ECN_FBS_ID req;

	ret = _ecn_fbs_cre(1, &req);
	if (ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_udp_cre_req_fbs() : _ecn_fbs_cre() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	ret = _ecn_fbs_add_data(req, &cmd, sizeof(cmd));
	if (ret != E_OK) {
		_ecn_fbs_del(req);
		ECN_DBG_PUT_2("_ecn_udp_cre_req_fbs() : _ecn_fbs_add_data() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	req.ptr->hdr.type = ECN_MSG_INTERNAL;
	req.ptr->hdr.sender.dtqid = sender;
	req.ptr->hdr.target.dtqid = ecn_udp_dataqueueid;
	req.ptr->hdr.reply.dtqid = sender;

	*pk_req = req;

	return E_OK;
}

/* 応答電文用fbs設定(sender/targetの設定) */
static ER _ecn_udp_cre_res_fbs(ECN_FBS_ID req, uint8_t cmd, ECN_FBS_ID *pk_res)
{
	ER ret;
	ECN_FBS_ID res;

	ret = _ecn_fbs_cre(1, &res);
	if (ret != E_OK) {
		ECN_DBG_PUT_2("_ecn_udp_cre_res_fbs() : _ecn_fbs_cre() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	ret = _ecn_fbs_add_data(res, &cmd, sizeof(cmd));
	if (ret != E_OK) {
		_ecn_fbs_del(res);
		ECN_DBG_PUT_2("_ecn_udp_cre_res_fbs() : _ecn_fbs_add_data() result = %d:%s", ret, itron_strerror(ret));
		return ret;
	}

	res.ptr->hdr.type = ECN_MSG_INTERNAL;
	res.ptr->hdr.sender.dtqid = ecn_udp_dataqueueid;
	res.ptr->hdr.target.dtqid = req.ptr->hdr.reply.dtqid;
	res.ptr->hdr.reply.dtqid = ecn_udp_dataqueueid;

	*pk_res = res;

	return E_OK;
}

/*
 *  内部メッセージ受信処理
 */
void _ecn_int_msg(ECN_FBS_ID fbs_id, ECN_FBS_SSIZE_T a_snd_len)
{
	ER result = E_OK, a_ret;
	ecn_udp_msg_get_ipaddr_req_t msg;
	ecn_udp_msg_get_ipaddr_error_t err;
	ECN_FBS_SSIZE_T len;
	ECN_FBS_ID buf;
	uint8_t cmd;

	a_ret = _ecn_fbs_get_data(fbs_id, &cmd, 1, &len);
	if (a_ret != E_OK) {
		ECN_DBG_PUT_2("[UDP TSK] _ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
		return;
	}

	switch(cmd){
	// IPアドレスを返信
	case ECN_UDP_MSG_GET_IPADDR_REQ:
		if (a_snd_len < sizeof(msg)) {
			result = E_PAR;
			break;
		}

		a_snd_len = 0;
		a_ret = _ecn_fbs_get_data(fbs_id, &msg, sizeof(msg), &a_snd_len);
		if (a_ret != E_OK) {
			ECN_DBG_PUT_2("[UDP TSK] _ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
		}

		if ((msg.enodid < 0) && (msg.enodid >= tnum_enodadr)) {
			result = E_PAR;
			break;
		}

		a_ret = _ecn_udp_cre_res_fbs(fbs_id, ECN_UDP_MSG_GET_IPADDR_RES, &buf);
		if (a_ret != E_OK) {
			return;
		}

		a_ret = _ecn_fbs_add_data_ex(buf, &msg.requestid, offsetof(ecn_udp_msg_get_ipaddr_res_t, enodadrb));
		if (a_ret != E_OK) {
			_ecn_fbs_del(buf);
			ECN_DBG_PUT_2("_ecn_int_msg() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
			return;
		}

		a_ret = _ecn_fbs_add_data_ex(buf, &enodadrb_table[msg.enodid], sizeof(enodadrb_table[msg.enodid]));
		if (a_ret != E_OK) {
			_ecn_fbs_del(buf);
			ECN_DBG_PUT_2("_ecn_int_msg() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
			return;
		}

		a_ret = psnd_dtq(buf.ptr->hdr.target.dtqid, (intptr_t)buf.ptr);
		if (a_ret != E_OK) {
			_ecn_fbs_del(buf);
			ECN_DBG_PUT_2("_ecn_int_msg() : psnd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
			return;
		}
		return;
	}

	a_ret = _ecn_udp_cre_res_fbs(fbs_id, ECN_UDP_MSG_GET_IPADDR_ERROR, &buf);
	if (a_ret != E_OK) {
		return;
	}

	err.requestid = msg.requestid;
	err.error = result;
	a_ret = _ecn_fbs_add_data_ex(buf, &err, sizeof(err));
	if (a_ret != E_OK) {
		_ecn_fbs_del(buf);
		ECN_DBG_PUT_2("_ecn_int_msg() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		return;
	}

	a_ret = psnd_dtq(buf.ptr->hdr.target.dtqid, (intptr_t)buf.ptr);
	if (a_ret != E_OK) {
		_ecn_fbs_del(buf);
		ECN_DBG_PUT_2("_ecn_int_msg() : psnd_dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
		return;
	}
}

/*
 *  ECHONET 電文受信処理
 */
void _ecn_esv_msg(ECN_FBS_ID fbs_id)
{
	/* UDP出力領域（暫定） */
	static uint_t buffer[(ETHER_MAX_LEN) / sizeof(uint_t) + 1];
	T_IPV4EP a_dst;
	ER a_ret;
	ECN_FBS_SSIZE_T a_snd_len;

	a_dst.ipaddr = 0;
	a_dst.portno = 0;
	/* 送信先IPアドレス */
	a_ret = udp_get_ip(&a_dst, fbs_id.ptr->hdr.target.id);
	if (!a_ret) {
		ECN_DBG_PUT_4("[UDP TSK] echonet-node 0x%02X-0x%02X-0x%02X → udp dest(%s)",
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx1,
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx2,
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx3,
			ip2str(NULL, &a_dst.ipaddr));

		/* fbsから出力領域にデータを抽出 */
		a_snd_len = 0;
		a_ret = _ecn_fbs_get_data(fbs_id, buffer, sizeof(buffer), &a_snd_len);
		if (a_ret != E_OK) {
			ECN_DBG_PUT_2("[UDP TSK] _ecn_fbs_get_data() result = %d:%s", a_ret, itron_strerror(a_ret));
		}
		else if (a_dst.ipaddr == MAKE_IPV4_ADDR(127,0,0,1)) {
			/* 送信先が127.0.0.1 → dtqに転送 */
			ECN_DBG_PUT_1("redirect ecn_udp_dataqueueid → ecn_svc_dataqueueid (esv:0x%02X)",
				((T_EDATA *)fbs_id.ptr)->hdr.edata.esv);

			a_ret = _ecn_udp2dtq((const uint8_t *)buffer, a_snd_len, &a_dst);
			if (a_ret != E_OK) {
				syslog(LOG_WARNING, "_ecn_esv_msg() : _ecn_udp2dtq() result = %d:%s", a_ret, itron_strerror(a_ret));
			}
		}
		else {
			ECN_DBG_PUT_2("[UDP TSK] udp_snd_dat() to:%s %ubyte(s)",
				ip2str(NULL, &a_dst.ipaddr), a_snd_len);
#ifdef ECN_DBG_PUT_ENA
			_ecn_dbg_bindmp((const uint8_t *)buffer, a_snd_len);
#endif
			/* UDP送信 */
			a_ret = udp_snd_dat(ecn_udp_cepid, &a_dst, buffer, a_snd_len, TMO_FEVR);
			if (a_ret < 0) {
				ECN_DBG_PUT_1("[UDP TSK] send, error: %s", itron_strerror(a_ret));
			}
		}

		/* データが長すぎて1パケットに収まらなかった場合 */
		if (_ecn_fbs_exist_data(fbs_id)) {
			ECN_DBG_PUT_1("[UDP TSK] send, data so long: %dbyte(s)", _ecn_fbs_get_datalen(fbs_id));
		}
	} else {
		ECN_DBG_PUT_3("[UDP TSK] echonet-node 0x%02X-0x%02X-0x%02X not found.",
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx1,
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx2,
			((T_EDATA *)fbs_id.ptr)->hdr.edata.deoj.eojx3);
	}
}

/*
 *  ノンブロッキングコールのコールバック関数
 */
ER callback_nblk_udp(ID cepid, FN fncd, void *p_parblk)
{
	ER	error = E_OK;

	static uint8_t buffer[(ETHER_MAX_LEN) / sizeof(uint_t) + 1];

	T_IPV4EP dst;
	int		a_len;

	switch (fncd) {
	case TEV_UDP_RCV_DAT:
	case TFN_UDP_RCV_DAT:
		/* ECN_CAP_PUT("[UDP ECHO SRV] callback_nblk_udp() recv: %u", *(int *)p_parblk); */
		memset(buffer, 0, sizeof(buffer));
		if ((a_len = udp_rcv_dat(cepid, &dst, buffer, sizeof(buffer) - 1, 20 * 1000)) < 0) {
			syslog(LOG_WARNING, "[UDP ECHO SRV] recv, error: %s", itron_strerror(a_len));
			return a_len;
		}
#ifdef _MSC_VER
		/* (開発用) ECHONET用パケット以外は読み捨てる */
		if (buffer[0] != 0x10 || buffer[1] != 0x81) {
			return E_OK;
		}
#endif
		ECN_CAP_PUT_4("[UDP ECHO SRV] recv, addr: %s:%d, len: %d, msg: '%s'",
			ip2str(NULL, &dst.ipaddr), dst.portno, a_len, buffer);
		if ((int)sizeof(buffer) <= a_len)
			a_len = (int)sizeof(buffer) - 1;
		if (0 < a_len) {
			ECHONET_UDP_SEND_TO_MBX;

			_ecn_udp2dtq(buffer, a_len, &dst);
		}

		return E_OK;

	case TFN_UDP_CRE_CEP:
	case TFN_UDP_SND_DAT:
		break;
	default:
		ECN_CAP_PUT_2("[UDP ECHO SRV] fncd:0x%04X(%s)", -fncd,
			(fncd == TEV_UDP_RCV_DAT ? "TEV_UDP_RCV_DAT" :
			(fncd == TFN_UDP_CRE_CEP ? "TFN_UDP_CRE_CEP" :
			(fncd == TFN_UDP_RCV_DAT ? "TFN_UDP_RCV_DAT" :
			(fncd == TFN_UDP_SND_DAT ? "TFN_UDP_SND_DAT" : "undef")))));

		error = E_PAR;
		break;
	}
	return error;
}

/*
 *  リモートECHONETノードの適合確認
 */
bool_t is_match(const EOBJCB *enodcb, T_EDATA *edata, const void *_ipaddr, uint16_t portno)
{
	/*T_IN4_ADDR *ipaddr = (T_IN4_ADDR *)_ipaddr;*/
	ER			ret;
	T_ENUM_EPC	enm;
	uint8_t		epc;
	uint8_t		pdc;
	uint8_t		p_edt[256];
	int			i, j, k;
	int			count;
	const EOBJINIB	*p_eobj;
	bool_t		match;

	if (!edata)
		return false;

	/* ノードスタート時インスタンスリスト通知以外は除外 */
	if (	edata->hdr.edata.esv != ESV_INF
		||	edata->hdr.edata.deoj.eojx1 != EOJ_X1_PROFILE
		||	edata->hdr.edata.deoj.eojx2 != EOJ_X2_NODE_PROFILE
		||	edata->hdr.edata.deoj.eojx3 != 0x01
		||	edata->hdr.edata.seoj.eojx1 != EOJ_X1_PROFILE
		||	edata->hdr.edata.seoj.eojx2 != EOJ_X2_NODE_PROFILE
		||	(	edata->hdr.edata.seoj.eojx3 != 0x01
			&&	edata->hdr.edata.seoj.eojx3 != 0x02)) {
		return false;
	}

	ret = ecn_itr_ini(&enm, edata);
	if (ret) {
		syslog(LOG_WARNING, "is_match(): ecn_itr_ini() result = %d:%s", ret, itron_strerror(ret));
		return false;
	}
	while ((ret = ecn_itr_nxt(&enm, &epc, &pdc, p_edt)) == E_OK) {
		if (enm.is_eof) {
			break;
		}
		ECN_DBG_PUT_2("is_match(): ecn_itr_nxt() result: epc=0x%02X, pdc=%d", epc, pdc);
		/* インスタンスリスト通知または自ノードインスタンスリストＳ以外は除外 */
		if ((epc != 0xD5) && (epc != 0xD6)) {
			continue;
		}

		/* ２バイト目以降にeojが列挙されている */
		count = (pdc - 1) / sizeof(T_ECN_EOJ);

		/* ノード内の機器オブジェクトを検索 */
		for (k = 0; k < enodcb->eobjcnt; k++) {
			p_eobj = enodcb->eobjs[k];

			/* インスタンスリストを確認 */
			match = false;
			for (i = 0, j = 1; i < count; i++, j += sizeof(T_ECN_EOJ)) {
				if (p_eobj->eojx1 != p_edt[j])
					continue;
				if (p_eobj->eojx2 != p_edt[j + 1])
					continue;
				if (p_eobj->eojx3 != p_edt[j + 2])
					continue;

				match = true;
				break;
			}

			if (!match)
				return false;
		}

		/* すべて揃っていたら適合（インスタンスリストの方が多くてもいいこととする） */
		return true;
	}

	return false;
}

/*
 *  IPアドレスからリモートECHONETノードへ変換
 */
ECN_ENOD_ID udp_get_id(T_EDATA *edata, const T_IN4_ADDR ipaddr, uint16_t portno)
{
	T_ENOD_ADDR	*ea;
	int			i;

	if (ipaddr == MAKE_IPV4_ADDR(127,0,0,1))
		return ENOD_LOCAL_ID;
	if (ipaddr == MAKE_IPV4_ADDR(224,0,23,0))
		return ENOD_MULTICAST_ID;

	/* IPアドレスの同じものを検索 */
	for (i = 0, ea = enodadrb_table; i < tnum_enodadr; i++, ea++) {
		if (!ea->inuse)
			continue;
		if (*((T_IN4_ADDR *)ea->ipaddr) != ipaddr)
			continue;

		ECN_CAP_PUT_2("udp_get_id(): ip-found remote(%d) = %s",
			i - ENOD_REMOTE_ID, ip2str(NULL, &ipaddr));
		return (ECN_ENOD_ID)i;
	}

	/* 対応するリモートノードを検索 */
	for (i = ENOD_REMOTE_ID, ea = &enodadrb_table[ENOD_REMOTE_ID]; i < tnum_enodadr; i++, ea++) {
		if (!ea->inuse)
			continue;
		if ((i - ENOD_REMOTE_ID + 1) >= tnum_enodid)
			break;
		if (*((T_IN4_ADDR *)ea->ipaddr) != 0)
			continue;
		if (!ECHONET_NODE_MATCH(&eobjcb_table[i - ENOD_REMOTE_ID + 1], edata, &ipaddr, portno))
			continue;

		/* 対応するリモートノードがあればIPアドレスを設定 */
		*((T_IN4_ADDR *)ea->ipaddr) = ipaddr;

		ECN_CAP_PUT_2("udp_get_id(): enod-found remote(%d) = %s",
			i - ENOD_REMOTE_ID, ip2str(NULL, &ipaddr));
		return (ECN_ENOD_ID)i;
	}

	/* 空き領域を探して自動登録 */
	for (i = ENOD_REMOTE_ID, ea = &enodadrb_table[ENOD_REMOTE_ID]; i < tnum_enodadr; i++, ea++) {
		if (ea->inuse)
			continue;

		ea->inuse = true;
		*((T_IN4_ADDR *)ea->ipaddr) = ipaddr;

		ECN_CAP_PUT_2("udp_get_id(): empty-found remote(%d) = %s",
			i - ENOD_REMOTE_ID, ip2str(NULL, &ipaddr));
		return (ECN_ENOD_ID)i;
	}

	return ENOD_NOT_MATCH_ID;
}

/*
 *  リモートECHONETノードからIPアドレスへ変換
 */
int udp_get_ip(T_IPV4EP *fp_ipep, ECN_ENOD_ID fa_enodid)
{
	T_ENOD_ADDR *ea;

	if (!fp_ipep)
		return -1;	/* NG */

	fp_ipep->portno = 3610;

	if (fa_enodid == ENOD_MULTICAST_ID) {
		/* targetがENOD_MULTICAST_IDの場合、マルチキャストを行う */
		fp_ipep->ipaddr = MAKE_IPV4_ADDR(224,0,23,0);
		return 0;	/* ok */
	}

	if (fa_enodid < ENOD_REMOTE_ID) {
		/* targetが未定義・LOCAL・APIの場合、ローカル配送を行う */
		fp_ipep->ipaddr = MAKE_IPV4_ADDR(127,0,0,1);
		return 0;	/* ok */
	}

	if (fa_enodid >= tnum_enodadr)
		return -1;	/* NG */

	ea = &enodadrb_table[fa_enodid];
	if (!ea->inuse)
		return -1;	/* NG */

	if (*((T_IN4_ADDR *)ea->ipaddr) == 0)
		return -1;	/* NG */

	fp_ipep->ipaddr = *((T_IN4_ADDR *)ea->ipaddr);
	return 0;	/* ok */
}

ER ecn_udp_get_ipaddr(ID sender, int requestid, ECN_ENOD_ID enodid, ECN_FBS_ID *pk_req)
{
	ER a_ret;
	ECN_FBS_ID req;

	a_ret = _ecn_udp_cre_req_fbs(sender, ECN_UDP_MSG_GET_IPADDR_REQ, &req);
	if (a_ret != E_OK) {
		return a_ret;
	}

	a_ret = _ecn_fbs_add_data_ex(req, &requestid, sizeof(((ecn_udp_msg_get_ipaddr_res_t *)0)->requestid));
	if (a_ret != E_OK) {
		_ecn_fbs_del(req);
		ECN_DBG_PUT_2("ecn_udp_get_ipaddr() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	a_ret = _ecn_fbs_add_data_ex(req, &enodid, sizeof(((ecn_udp_msg_get_ipaddr_res_t *)0)->enodid));
	if (a_ret != E_OK) {
		_ecn_fbs_del(req);
		ECN_DBG_PUT_2("ecn_udp_get_ipaddr() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	a_ret = _ecn_fbs_add_data_ex(req, &enodadrb_table[enodid], sizeof(((ecn_udp_msg_get_ipaddr_res_t *)0)->enodadrb));
	if (a_ret != E_OK) {
		_ecn_fbs_del(req);
		ECN_DBG_PUT_2("ecn_udp_get_ipaddr() : _ecn_fbs_add_data_ex() result = %d:%s", a_ret, itron_strerror(a_ret));
		return a_ret;
	}

	*pk_req = req;

	return E_OK;
}

char *ipaddr2str(char *buf, int bubsz, uint8_t *ipaddr)
{
	return ip2str(buf, (T_IN4_ADDR *)ipaddr);
}

#endif
