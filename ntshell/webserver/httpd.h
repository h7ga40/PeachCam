/*
 *  TOPPERS ECHONET Lite Communication Middleware
 *
 *  Copyright (C) 2017 Cores Co., Ltd. Japan
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

#ifndef __HTTPD_H__
#define __HTTPD_H__

/*
 *  ターゲット依存の定義
 */
#include <kernel.h>
#include "httpd-fs.h"
#include "http_parser.h"
#include "websocket.h"

/*
 *  各タスクの優先度の定義
 */

#define HTTPD_PRIORITY	5		/* HTTPサーバータスクの優先度 */

#define HTTPD_STACK_SIZE		1024	/* HTTPサーバータスクのスタック領域のサイズ */

 /*  TCP 送受信ウィンドバッファサイズ  */

#define TCP_SWBUF_SIZE	512
#define TCP_RWBUF_SIZE	512

/*
 *  関数のプロトタイプ宣言
 */
#ifndef TOPPERS_MACRO_ONLY

 /*  TCP 送受信ウィンドバッファ  */

extern uint8_t tcp_swbuf1[];
extern uint8_t tcp_rwbuf1[];
extern uint8_t tcp_swbuf2[];
extern uint8_t tcp_rwbuf2[];

/* HTTPサーバータスク */
extern void httpd_task(intptr_t exinf);

#define MAX_ELEMENT_SIZE 256

struct message {
	enum http_method method;
	unsigned short http_major;
	unsigned short http_minor;
	char filename[64];
	char request_url[MAX_ELEMENT_SIZE];
	char response_status[64];
	int num_headers;
	char host[MAX_ELEMENT_SIZE];
	char referer[MAX_ELEMENT_SIZE];
	char upgrade[32];
	char connection[32];
	char sec_websocket_key[64];
	char origin[MAX_ELEMENT_SIZE];
	char sec_websocket_protocol[64];
	char sec_websocket_version[4];
	char response_key[86];
	size_t body_size;
	int should_keep_alive;
	int headers_complete_cb_called;
	int message_begin_cb_called;
	int message_complete_cb_called;
	int body_is_final;
};

#ifndef _MSC_VER
size_t strnlen(const char *s, size_t maxlen);
#endif
size_t strlncat(char *dst, size_t len, const char *src, size_t n);
extern http_parser_settings websvr_settings;

typedef enum httpd_state_t {
	STATE_DISCONNECTED,		/* 切断中 */
	STATE_CONNECTED,		/* 接続中 */
	STATE_WEBSOCKET,		/* WebSocket通信中 */
	STATE_CLOSING,			/* 切断処理中 */
	STATE_RESET,			/* mruby起動のためのリセット */
} httpd_state_t;

typedef enum httpd_in_state_t {
	IN_STATE_START,
	IN_STATE_REQUEST,
	IN_STATE_UPLOAD,
	IN_STATE_UPLOAD_WAIT,
	IN_STATE_RESPONSE,
	IN_STATE_WEBSOCKET,
	IN_STATE_END,
} httpd_in_state_t;

typedef enum httpd_out_state_t {
	OUT_STATE_WAIT_REQUEST,
	OUT_STATE_OPEN_GET_FILE,
	OUT_STATE_WAIT_POST_BODY,
	OUT_STATE_BODY_RECEIVED,
	OUT_STATE_SEND_HEADER,
	OUT_STATE_SEND_FILE,
	OUT_STATE_SEND_DATA,
	OUT_STATE_SEND_END,
} httpd_out_state_t;

struct httpd_state {
	ID tskid;
	ID cepid;
	uint8_t dst[20];
	char temp[16];
	char addr[sizeof("0123:4567:89ab:cdef:0123:4567:89ab:cdef")];
	httpd_state_t state;
	int reset;
	struct {
		httpd_in_state_t state;
		bool_t wait;
		char *data;
		SYSTIM timer;
	} in;
	struct {
		httpd_out_state_t state;
		bool_t wait;
		const char *statushdr;
	} out;
	struct {
		int parse_pos;
		int parse_len;
		struct http_parser parser;
		struct http_parser_url handle;
		struct message message;
	};
	union {
		struct {
			char *filename;
			char *query;
			struct httpd_fs_file file;
		};
		struct {
			char *_dummy_filename;
			const char *response_body;
			int response_pos;
			int response_len;
		};
		struct {
			struct websocket websocket;
			int close_req;
		};
	};
};
#define get_context(p) (struct httpd_state *)((intptr_t)p - (intptr_t)&((struct httpd_state *)0)->parser)

/* 
 * ノンブロッキングコールのコールバック関数
 */
extern ER callback_nblk_tcp(ID cepid, FN fncd, void *p_parblk);

#endif /* TOPPERS_MACRO_ONLY */

#endif /* __HTTPD_H__ */
