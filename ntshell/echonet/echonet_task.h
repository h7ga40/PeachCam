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
 *  @(#) $Id: echonet_task.h 1971 2019-07-10 04:36:59Z coas-nagasima $
 */

/*
 *		ECHONET Lite タスク
 */

#ifndef TOPPERS_ECHONET_TASK_H
#define TOPPERS_ECHONET_TASK_H

/*#include ".h"*/

#include "echonet_rename.h"
#include "echonet.h"
#include "echonet_fbs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ECHONET Lite タスク関連の定数のデフォルト値の定義
 */ 
#ifndef ECHONET_TASK_PRIORITY
#define ECHONET_TASK_PRIORITY	3		/* 初期優先度 */
#endif /* ECHONET_TASK_PRIORITY */

#ifndef ECHONET_TASK_STACK_SIZE
#define ECHONET_TASK_STACK_SIZE	1024	/* スタック領域のサイズ */
#endif /* ECHONET_TASK_STACK_SIZE */

extern const EOBJINIB _echonet_eobjinib_table[];
extern EOBJCB _echonet_eobjcb_table[];
extern ENODADRB _echonet_enodadrb_table[];

/*
 *  ECHONET Lite タスクの本体
 */
void echonet_task(intptr_t exinf);

/* ECHONET Lite インスタンスリストの1アナウンス当たり最大ノード数 */
#define ECN_IST_LST_EOJ_MAX_CT (84)

/* ECHONET Liteオブジェクトコード x1,x2,x3(T_ECN_EOJ)を24bit整数に変換 */
#define _ECN_EOJ2VALUE(s) (((s).eojx1 << 16) | ((s).eojx2 << 8) | (s).eojx3)

enum ecn_epc_code
{
	ECN_EPC_INL_BCS		=	0xD5	/* インスタンスリスト通知プロパティ */
};

typedef enum
{
	ECN_MSG_UNDEF		=	0,
	ECN_MSG_INTERNAL	=	1,
	ECN_MSG_ECHONET		=	2,
	ECN_MSG_USER_BREAK	=	3
} ECN_MSG_TYPE;

typedef enum
{
	ECN_INM_NOTIFY_INSTANCELIST = 1,
	ECN_INM_GET_DEVICE_LIST_REQ,
	ECN_INM_GET_DEVICE_LIST_RES,
	ECN_INM_GET_DEVICE_INFO_REQ,
	ECN_INM_GET_DEVICE_INFO_RES,
	ECN_UDP_MSG_GET_IPADDR_REQ,
	ECN_UDP_MSG_GET_IPADDR_RES,
	ECN_UDP_MSG_GET_IPADDR_ERROR
} ECN_INT_MSG_COMMAND;

typedef struct
{
	uint8_t	command;
	uint8_t	data[1];
} T_ECN_INTERNAL_MSG;

/* 
 * インスタンスリスト通知の送信
 */
ER _ecn_tsk_ntf_inl();
/*
 * 要求電文作成
 */
ER _ecn_tsk_mk_esv(ECN_FBS_ID *fp_fbs_id, ID fa_seoj, ID fa_deoj,
	uint8_t fa_epc, uint8_t fa_pdc, const void *p_edt, ECN_SRV_CODE fa_esv);

ER _ecn_tsk_snd_dtq(ECN_FBS_ID fa_rsp_fbs, bool_t from_app);

/*
 * 応答電文待ちの割り込み電文作成
 */
ER _ecn_mk_brk_wai(ECN_FBS_ID *pk_fbs_id, const void *p_dat, size_t fa_size);

const EOBJCB *_ecn_eno_fnd(ECN_ENOD_ID enodid);

const EOBJINIB *_ecn_eoj_fnd(const EOBJCB *fp_nod, const T_ECN_EOJ *fp_eoj);

ER _ecn_tsk_cre_req_fbs(ID sender, uint8_t cmd, ECN_FBS_ID *pk_req);

ER _ecn_tsk_cre_res_fbs(ECN_FBS_ID req, uint8_t cmd, ECN_FBS_ID *pk_res);

#ifdef __cplusplus
}
#endif

#endif /* TOPPERS_ECHONET_TASK_H */
