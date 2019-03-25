/*
 *  TOPPERS ECHONET Lite Communication Middleware
 * 
 *  Copyright (C) 2014-2017 Cores Co., Ltd. Japan
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
 *  @(#) $Id: main.h 1663 2018-11-01 11:50:10Z coas-nagasima $
 */

#ifndef _MAIN_H_
#define _MAIN_H_

/*
 *		サンプルプログラム(1)のヘッダファイル
 */

/*
 *  ターゲット依存の定義
 */
#include <kernel.h>

#ifndef TOPPERS_CFG1_OUT
#include "tinet_cfg.h"
#endif

/*
 *  各タスクの優先度の定義
 */

#define MAIN_PRIORITY		5		/* メインタスクの優先度 */
#define MAIN_STACK_SIZE		2048	/* メインタスクのスタック領域のサイズ */
#define NUM_MAIN_DATAQUEUE	1		/* メインタスクで待ち受けているデータキューのサイズ */

#define HOIKUCAM_PRIORITY	5		/* HoikuCamタスクの優先度 */
#define HOIKUCAM_STACK_SIZE	10240	/* HoikuCamタスクのスタック領域のサイズ */

/*
 *  関数のプロトタイプ宣言
 */
#ifndef TOPPERS_MACRO_ONLY

/* メインタスク */
extern void main_task(intptr_t exinf);
/* HoikuCamタスク */
extern void hoikucam_task(intptr_t exinf);

#endif /* TOPPERS_MACRO_ONLY */

#endif	/* of #ifndef _MAIN_H_ */
