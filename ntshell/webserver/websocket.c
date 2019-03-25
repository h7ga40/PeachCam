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
 *  @(#) $Id: websocket.c 1195 2017-03-08 12:41:42Z coas-nagasima $
 */

#include <string.h>
#include "websocket.h"
#include "kernel.h"
#include "kernel_cfg.h"
#include "syssvc/syslog.h"

static WS_FBS_ID cur_out_msg;

void websocket_init(struct websocket *ws, ID wbsid)
{
	memset(ws, 0, sizeof(*ws));
	ws->wbsid = wbsid;
}

void websocket_destroy(struct websocket *ws)
{
	if ((cur_out_msg.ptr != NULL) && (((ID *)cur_out_msg.ptr->_gap)[0] == ws->wbsid))
	{
		_ws_fbs_del(cur_out_msg);
		memset(&cur_out_msg, 0, sizeof(cur_out_msg));
	}

	if (ws->cur_in_msg.ptr != NULL) {
		_ws_fbs_del(ws->cur_in_msg);
		memset(&ws->cur_in_msg, 0, sizeof(ws->cur_in_msg));
	}

	ws->wbsid = 0;
}

int websocket_input(struct websocket *ws, const void *data, int len)
{
	ws_state_t *s = &ws->rstate;
	const uint8_t *pos = (const uint8_t *)data, *end = &((const uint8_t *)data)[len];

	for(; pos < end; pos++){
		switch (s->state)
		{
		// FIN RSV1-3 OPECODE
		case 0:
			s->fin = (*pos & 0x80) != 0;
			s->opecode = (enum opecode_t)(*pos & 0x0F);
			s->state = 1;
			break;
		// MASK Payload len
		case 1:
			// MASK=0はNG
			if((*pos & 0x80) == 0)
				return -1/*MASK_ERROR*/;

			switch(*pos & 0x7F){
			case 127:
				s->payload_len = 0;
				s->state = 2/*7+64bit*/;
				break;
			case 126:
				s->payload_len = 0;
				s->state = 8/*7+16bit*/;
				break;
			default:
				s->payload_len = *pos & 0x7F;
				s->state = 10/*7bit*/;
				break;
			}
			break;
		// 64bit Extended payload length
		case 2: case 3: case 4: case 5: case 6: case 7:
		// 16bit Extended payload length
		case 8: case 9:
			s->payload_len = (s->payload_len << 8) + *pos;
			s->state++;
			break;
		// Masking-key
		case 10: case 11: case 12:
			s->masking_key[s->state - 10] = *pos;
			s->state++;
			break;
		case 13:
			s->masking_key[3] = *pos;

			s->data_pos = 0;
			if (s->payload_len <= 0) {
				s->state = 0;
			}
			else {
				_ws_fbs_cre(1, &ws->cur_in_msg);
				s->state++;
			}
			break;
		// Payload buffer
		default:
			if(ws->cur_in_msg.ptr != NULL)
				_ws_fbs_poke(ws->cur_in_msg, s->data_pos, *pos ^ s->masking_key[s->data_pos % 4]);

			s->data_pos++;
			if (s->data_pos >= s->payload_len) {
				ER ret;
				s->state = 0;

				if (ws->cur_in_msg.ptr != NULL) {
					((ID *)ws->cur_in_msg.ptr->_gap)[0] = ws->wbsid;
					ret = psnd_dtq(ws_api_mailboxid, (intptr_t)ws->cur_in_msg.ptr);
					if (ret != E_OK) {
						syslog(LOG_WARNING, "websocket_input() : psnd_dtq(%d) result = %d", ws_api_mailboxid, ret);
						_ws_fbs_del(ws->cur_in_msg);
					}
				}
				memset(&ws->cur_in_msg, 0, sizeof(ws->cur_in_msg));
			}
			break;
		}
	}

	return 0;
}

bool_t websocket_newdata(struct websocket *ws)
{
	ws_state_t *s = &ws->wstate;
	struct websocket *mws;
	ER ret;
	WS_FBS_ID data;
	ID wbsid;

	if(s->state != 0)
		return true;

	if (cur_out_msg.ptr != NULL)
		return (((ID *)cur_out_msg.ptr->_gap)[0] == ws->wbsid);

	for (;;) {
		ret = prcv_dtq(WEBSOCKET_MBXID, (intptr_t *)&data.ptr);
		if (ret == E_TMOUT)
			return false;

		if (ret != E_OK) {
			TOPPERS_assert_abort();
			return false;
		}

		wbsid = ((ID *)data.ptr->_gap)[0];
		mws = websocket_getws(wbsid);
		if ((mws != NULL) && (mws->wbsid != 0))
			break;

		_ws_fbs_del(cur_out_msg);
	}

	cur_out_msg.ptr = data.ptr;

	return (wbsid == ws->wbsid);
}

int websocket_output(struct websocket *ws, void *data, int len)
{
	ws_state_t *s = &ws->wstate;
	uint8_t *pos = (uint8_t *)data, *end = &((uint8_t *)data)[len];

	for(; pos < end; pos++){
		switch (s->state)
		{
		// FIN RSV1-3 OPECODE
		case 0:
			if ((cur_out_msg.ptr == NULL) || (((ID *)cur_out_msg.ptr->_gap)[0] != ws->wbsid))
				return (intptr_t)pos - (intptr_t)data;

			s->payload_len = _ws_fbs_get_datalen(cur_out_msg);
			if(s->payload_len == 0){
				goto next;
			}

			s->fin = 1;
			s->opecode = text_frame;
			s->masked = 0;//
			s->masking_key[0] = 0;//0x01;
			s->masking_key[1] = 0;//0x23;
			s->masking_key[2] = 0;//0x45;
			s->masking_key[3] = 0;//0x67;

			*pos = ((s->fin != 0) ? 0x80 : 0)
				| ((char)s->opecode & 0x0F);
			s->state = 1;
			break;
		// MASK Payload len
		case 1:
			if(s->payload_len < 126){
				*pos = ((s->masked != 0) ? 0x80 : 0) | s->payload_len;
				if (s->masked != 0) {
					s->state = 10/*7bit*/;
				}
				else{
					goto payload_check;
				}
			}
			else if(s->payload_len < 0x10000){
				*pos = ((s->masked != 0) ? 0x80 : 0) | 126;
				s->state = 8/*7+16bit*/;
			}
			else{
				*pos = ((s->masked != 0) ? 0x80 : 0) | 127;
				s->state = 2/*7+64bit*/;
			}
			break;
		// 64bit Extended payload length
		case 2: case 3: case 4: case 5: case 6: case 7:
		// 16bit Extended payload length
		case 8: case 9:
			*pos = (s->payload_len >> (8 * (9 - s->state)));
			s->state++;
			if((s->state == 10) && (s->masked == 0)){
				goto payload_check;
			}
			break;
		// Masking-key
		case 10: case 11: case 12:
			*pos = s->masking_key[s->state - 10];
			s->state++;
			break;
		case 13:
			*pos = s->masking_key[3];
			s->state++;
		payload_check:
			s->data_pos = 0;
			if (s->data_pos < s->payload_len)
				s->state = 14;
			else
				goto next;
			break;
		// Payload buffer
		default:
			*pos = _ws_fbs_peek(cur_out_msg, s->data_pos) ^ s->masking_key[s->data_pos % 4];

			s->data_pos++;
			if(s->data_pos >= s->payload_len){
				goto next;
			}
			break;
		next:
			_ws_fbs_del(cur_out_msg);
			memset(&cur_out_msg, 0, sizeof(cur_out_msg));
			s->state = 0;
			websocket_newdata(ws);
			break;
		}
	}

	return (intptr_t)pos - (intptr_t)data;
}
