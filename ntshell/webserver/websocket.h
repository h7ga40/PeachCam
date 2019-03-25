/*
 *  TOPPERS ECHONET Lite Communication Middleware
 * 
 *  Copyright (C) 2015 Cores Co., Ltd. Japan
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
 *  @(#) $Id: websocket.h 1195 2017-03-08 12:41:42Z coas-nagasima $
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stddef.h>
#include <stdint.h>
#include "t_stddef.h"

#define NUM_WEBSOCKET_MBXID	2

#ifndef TOPPERS_MACRO_ONLY
#include "websocket_fbs.h"

extern ID ws_api_mailboxid;

typedef struct ws_state
{
	int state;
	int fin;
	enum opecode_t
	{
		continuation_frame = 0,
		text_frame = 1,
		binary_frame = 2,
		connection_close = 8,
		ping = 9,
		pong = 10
	} opecode;
	int payload_len;
	int masked;
	char masking_key[4];
	int data_pos;
} ws_state_t;

struct websocket
{
	ID wbsid;
	ws_state_t wstate;
	ws_state_t rstate;
	WS_FBS_ID cur_in_msg;
};

void websocket_init(struct websocket *ws, ID wbsid);
void websocket_destroy(struct websocket *ws);
struct websocket *websocket_getws(ID wbsid);

int websocket_input(struct websocket *ws, const void *data, int len);
bool_t websocket_newdata(struct websocket *ws);
int websocket_output(struct websocket *ws, void *data, int len);

#endif /* TOPPERS_MACRO_ONLY */

#endif  /* WEBSOCKET_H */
