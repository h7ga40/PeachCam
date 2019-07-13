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
 *  @(#) $Id: echonet_lcl_task.h 1971 2019-07-10 04:36:59Z coas-nagasima $
 */

/*
 *		ECHONET Lite 下位通信レイヤー処理タスク
 */

#ifndef TOPPERS_ECHONET_LCL_TASK_H
#define TOPPERS_ECHONET_LCL_TASK_H

#ifdef SUPPORT_INET4
#include "echonet_udp_task.h"
#endif

#ifdef SUPPORT_INET6
#include "echonet_udp6_task.h"
#endif

#ifdef SUPPORT_UIP
#include "echonet_uip_task.h"
#endif

typedef struct ecn_udp_msg_get_ipaddr_req
{
	unsigned int requestid;
	ECN_ENOD_ID enodid;
} ecn_udp_msg_get_ipaddr_req_t;

typedef struct ecn_udp_msg_get_ipaddr_res
{
	unsigned int requestid;
	ECN_ENOD_ID enodid;
	ENODADRB enodadrb;
} ecn_udp_msg_get_ipaddr_res_t;

typedef struct ecn_udp_msg_get_ipaddr_error
{
	unsigned int requestid;
	ER error;
} ecn_udp_msg_get_ipaddr_error_t;

ER ecn_udp_get_ipaddr(ID sender, int requestid, ECN_ENOD_ID enodid, ECN_FBS_ID *pk_req);

#endif /* TOPPERS_ECHONET_LCL_TASK_H */
