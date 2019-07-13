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
 *  @(#) $Id: echonet_udp6_task.h 1971 2019-07-10 04:36:59Z coas-nagasima $
 */

/*
 *		ECHONET Lite タスク
 */

#ifndef TOPPERS_ECHONET_UDP_TASK_H
#define TOPPERS_ECHONET_UDP_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel.h>
#include "echonet.h"
#include "echonet_fbs.h"

/*
 *  ECHONET Lite 一斉同報アドレス
 */
#define ECHONET_MULTICAST_ADDR 	\
	{ UINT_C(0xff), UINT_C(0x02), UINT_C(0x00), UINT_C(0x00), \
	  UINT_C(0x00), UINT_C(0x00), UINT_C(0x00), UINT_C(0x00), \
	  UINT_C(0x00), UINT_C(0x00), UINT_C(0x00), UINT_C(0x00), \
	  UINT_C(0x00), UINT_C(0x00), UINT_C(0x00), UINT_C(0x01) }

/*
 *  ECHONET Lite タスク関連の定数のデフォルト値の定義
 */
#ifndef ECHONET_UDP_TASK_PRIORITY
#define ECHONET_UDP_TASK_PRIORITY	3		/* 初期優先度 */
#endif /* ECHONET_UDP_TASK_PRIORITY */

#ifndef ECHONET_UDP_TASK_STACK_SIZE
#define ECHONET_UDP_TASK_STACK_SIZE	1024	/* スタック領域のサイズ */
#endif /* ECHONET_UDP_TASK_STACK_SIZE */

#ifndef NUM_ECHONET_UDP_DATAQUEUE
#define NUM_ECHONET_UDP_DATAQUEUE	10
#endif /* NUM_ECHONET_UDP_DATAQUEUE */

/*
 *  ECHONET Lite タスクの本体
 */
void echonet_udp_task(intptr_t exinf);

/*  ノンブロッキングコールのコールバック関数  */
ER callback_nblk_udp(ID cepid, FN fncd, void *p_parblk);

/*
 *  リモートECHONETノードの適合確認
 */
bool_t is_match(const EOBJCB *eobjcb, T_EDATA *edata, const void *_ipaddr, uint16_t portno);

/* IPアドレスを文字列に変換 */
char *ipaddr2str(char *buf, int bubsz, uint8_t *ipaddr);

#ifdef __cplusplus
}
#endif

#endif /* TOPPERS_ECHONET_UDP_TASK_H */
