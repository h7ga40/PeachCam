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
 *  @(#) $Id: httpd.c 1357 2018-01-06 14:57:47Z coas-nagasima $
 */

#include "httpd.h"
#include <string.h>
#include <stdlib.h>
#include "kernel_cfg.h"
#include "tinet_cfg.h"
#include "syssvc/syslog.h"
#include "http-strings.h"
#include <tinet_defs.h>
#include <tinet_config.h>
#include "netinet/in.h"
#include "netinet/in_var.h"
#include "netinet/in_itron.h"
#include "httpd.h"
#include "httpd-fs.h"
#include "http-strings.h"
#include "base64.h"
#include "sha1.h"
#include <stdio.h>
#include "core/ntlibc.h"

#define TRUE 1
#define FALSE 0

SYSTIM httpd_time;
struct httpd_state *uploding;
extern char command[256];

extern int execute_command(int wait);

/*  TCP 送受信ウィンドバッファ  */
uint8_t tcp_swbuf1[TCP_SWBUF_SIZE];
uint8_t tcp_rwbuf1[TCP_RWBUF_SIZE];
uint8_t tcp_swbuf2[TCP_SWBUF_SIZE];
uint8_t tcp_rwbuf2[TCP_RWBUF_SIZE];

#define ISO_nl      0x0a
#define ISO_space   0x20
#define ISO_bang    0x21
#define ISO_percent 0x25
#define ISO_period  0x2e
#define ISO_slash   0x2f
#define ISO_colon   0x3a

struct httpd_state httpd_state[2] = {
	{ HTTPD1_TASK, TCP_CEPID1 },
	{ HTTPD2_TASK, TCP_CEPID2 },
};

#ifndef _MSC_VER
#define strnlen httpd_strnlen
/* strnlen() is a POSIX.2008 addition. Can't rely on it being available so
 * define it ourselves.
 */
size_t
strnlen(const char *s, size_t maxlen)
{
	const char *p;

	p = memchr(s, '\0', maxlen);
	if (p == NULL)
		return maxlen;

	return p - s;
}

void strcpy_s(char *dst, int size, const char *src)
{
	int slen = strlen(src);
	if (slen >= size)
		slen = size - 1;
	memcpy(dst, src, slen);
	dst[slen] = '\0';
}

void strcat_s(char *dst, int size, const char *src)
{
	int dlen = strlen(dst);
	int slen = strlen(src);
	if (dlen + slen >= size)
		slen = size - 1 - dlen;
	memcpy(&dst[dlen], src, slen);
	dst[dlen + slen] = '\0';
}
#endif

int httpd_strnicmp(const char *s1, const char *s2, size_t n)
{
	int i;
	char c1, c2;

	for (i = 0; i < n; i++, s1++, s2++) {
		c1 = *s1;
		c2 = *s2;
		if (c1 == '\0' && c2 == '\0')
			return 0;

		if (c1 >= 'a' && c1 <= 'z')
			c1 += 'A' - 'a';

		if (c2 >= 'a' && c2 <= 'z')
			c2 += 'A' - 'a';

		if (c1 < c2)
			return -1;

		if (c1 > c2)
			return 1;
	}

	return 0;
}

size_t
strlncat(char *dst, size_t len, const char *src, size_t n)
{
	size_t slen;
	size_t dlen;
	size_t rlen;
	size_t ncpy = 0;

	slen = strnlen(src, n);
	dlen = strnlen(dst, len);

	if (dlen < len) {
		rlen = len - dlen;
		ncpy = slen < rlen ? slen : (rlen - 1);
		memcpy(dst + dlen, src, ncpy);
		dst[dlen + ncpy] = '\0';
	}

	//assert(len > slen + dlen);
	//return slen + dlen;
	return ncpy;
}

int websvr_message_begin(http_parser *p)
{
	struct httpd_state *s = get_context(p);
	memset(&s->message, 0, sizeof(s->message));
	s->message.message_begin_cb_called = TRUE;
	return 0;
}

int websvr_request_url(http_parser *p, const char *buf, size_t len)
{
	struct httpd_state *s = get_context(p);

	strlncat(s->message.request_url, sizeof(s->message.request_url), buf, len);

	char *ptr = strrchr(s->message.request_url, '?');
	if (ptr != NULL) {
		ptr[0] = '\0';
		s->query = &ptr[1];
	}
	else
		s->query = NULL;

	/* ""か"/"なら"index.html"に変更 */
	if ((s->message.request_url[0] == '\0') || ((s->message.request_url[0] == '/') && (s->message.request_url[1] == '\0'))) {
		s->filename = &s->message.filename[sizeof(s->message.filename) - 2];
		strlcpy(s->filename, "0:", sizeof(s->message.request_url) + 2);
		strlcat(s->filename, http_index_html, sizeof(s->message.request_url) + 2);
		s->file.redirect = 1;
	}
	/* "/~/"ならSDカードから読み込み */
	else if ((s->message.request_url[0] == '/') && (s->message.request_url[1] == '~') && (s->message.request_url[2] == '/')) {
		s->filename = &s->message.filename[sizeof(s->message.filename) - 2 - sizeof(http_www) - 1 + 2];
		memcpy(s->filename, "1:", 2);
		memcpy(s->filename, http_www, sizeof(http_www) - 1);
	}
	else {
		s->filename = &s->message.filename[sizeof(s->message.filename) - 2];
		memcpy(s->filename, "0:", 2);
	}
	return 0;
}

int websvr_response_status(http_parser *p, const char *buf, size_t len)
{
	struct httpd_state *s = get_context(p);

	strlncat(s->message.response_status, sizeof(s->message.response_status), buf, len);

	return 0;
}

int websvr_header_field(http_parser *p, const char *buf, size_t len)
{
	struct httpd_state *s = get_context(p);
	struct message *m = &s->message;

	if (ntlibc_strncmp("Referer", buf, len) == 0) {
		m->num_headers = 1;
	}
	else if (ntlibc_strncmp("Host", buf, len) == 0) {
		m->num_headers = 2;
	}
	else if (ntlibc_strncmp("Upgrade", buf, len) == 0) {
		m->num_headers = 3;
	}
	else if (ntlibc_strncmp("Connection", buf, len) == 0) {
		m->num_headers = 4;
	}
	else if (ntlibc_strncmp("Sec-WebSocket-Key", buf, len) == 0) {
		m->num_headers = 5;
	}
	else if (ntlibc_strncmp("Origin", buf, len) == 0) {
		m->num_headers = 6;
	}
	else if (ntlibc_strncmp("Sec-WebSocket-Protocol", buf, len) == 0) {
		m->num_headers = 7;
	}
	else if (ntlibc_strncmp("Sec-WebSocket-Version", buf, len) == 0) {
		m->num_headers = 8;
	}
	else {
		m->num_headers = 0;
	}

	return 0;
}

int websvr_header_value(http_parser *p, const char *buf, size_t len)
{
	struct httpd_state *s = get_context(p);
	struct message *m = &s->message;

	switch (m->num_headers) {
	case 1:
		strlncat(m->referer, sizeof(m->referer), buf, len);
		break;
	case 2:
		strlncat(m->host, sizeof(m->host), buf, len);
		break;
	case 3:
		strlncat(m->upgrade, sizeof(m->upgrade), buf, len);
		break;
	case 4:
		strlncat(m->connection, sizeof(m->connection), buf, len);
		break;
	case 5:
		strlncat(m->sec_websocket_key, sizeof(m->sec_websocket_key), buf, len);
		break;
	case 6:
		strlncat(m->origin, sizeof(m->origin), buf, len);
		break;
	case 7:
		strlncat(m->sec_websocket_protocol, sizeof(m->sec_websocket_protocol), buf, len);
		break;
	case 8:
		strlncat(m->sec_websocket_version, sizeof(m->sec_websocket_version), buf, len);
		break;
	}

	return 0;
}

int websvr_headers_complete(http_parser *p)
{
	struct httpd_state *s = get_context(p);

	s->message.method = p->method;
	s->message.http_major = p->http_major;
	s->message.http_minor = p->http_minor;
	s->message.headers_complete_cb_called = TRUE;
	s->message.should_keep_alive = http_should_keep_alive(p);

	if ((s->message.method == HTTP_GET)
		&& httpd_strnicmp(http_websocket, s->message.upgrade, sizeof(s->message.upgrade)) == 0) {
		s->in.state = IN_STATE_WEBSOCKET;
		s->state = STATE_WEBSOCKET;
		s->close_req = 0;
		websocket_init(&s->websocket, s->cepid);
		return 0;
	}
	else if (s->message.method == HTTP_GET) {
		s->in.state = IN_STATE_RESPONSE;
		s->out.state = OUT_STATE_OPEN_GET_FILE;
		return 1;
	}
	else if (s->message.method == HTTP_POST) {
		if ((s->parser.content_length > 16 * 1024)
			|| httpd_strnicmp(s->message.request_url, http_upload, sizeof(http_upload) - 1) != 0) {
			goto error;
		}

		if (uploding == NULL) {
			uploding = s;
			// アップロード先はSDカード
			s->filename[0] = '1';
			printf("create:    %s.%d %s\n", s->addr, ((T_IPV4EP *)s->dst)->portno, s->filename);
			if (!httpd_fs_create(s->filename, &s->file)) {
				goto error;
			}

			s->in.state = IN_STATE_UPLOAD;
		}
		else if (strcmp(s->filename, uploding->filename) == 0) {
			printf("collision: %s.%d %s\n", s->addr, ((T_IPV4EP *)s->dst)->portno, s->filename);
			goto error;
		}
		else {
			s->in.state = IN_STATE_UPLOAD_WAIT;
			s->in.wait = true;
		}

		s->out.state = OUT_STATE_WAIT_POST_BODY;
		s->out.wait = true;
		return 0;
	}
	else {
		s->state = STATE_CLOSING;
		return 1;
	}
error:
	s->filename = NULL;
	s->response_body = http_content_403;
	s->response_pos = 0;
	s->response_len = sizeof(http_content_403) - 1;

	s->out.statushdr = http_header_403;
	s->out.state = OUT_STATE_SEND_HEADER;
	return 1;
}

int websvr_body(http_parser *p, const char *buf, size_t len)
{
	struct httpd_state *s = get_context(p);

	if (s->message.body_is_final) {
		syslog(LOG_ERROR, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");
		s->state = STATE_CLOSING;
		return 0;
	}

	httpd_fs_write(&s->file, buf, len);

	s->message.body_size += len;
	s->message.body_is_final = http_body_is_final(p);

	if (s->message.body_is_final) {
		printf("close:     %s.%d %s\n", s->addr, ((T_IPV4EP *)s->dst)->portno, s->filename);
		httpd_fs_close(&s->file);
		memset(&s->file, 0, sizeof(s->file));

		strlcpy(command, "mruby -b ", sizeof(command));
		strlcat(command, s->filename, sizeof(command));
		s->reset = 1;

		s->filename = NULL;
		s->response_body = http_content_200;
		s->response_pos = 0;
		s->response_len = sizeof(http_content_200) - 1;

		uploding = NULL;
		s->in.state = IN_STATE_END;
		s->out.state = OUT_STATE_BODY_RECEIVED;
	}

	return 0;
}

int websvr_message_complete(http_parser *p)
{
	struct httpd_state *s = get_context(p);
	if (s->message.should_keep_alive != http_should_keep_alive(p)) {
		syslog(LOG_ERROR, "\n\n *** Error http_should_keep_alive() should have same "
			"value in both on_message_complete and on_headers_complete "
			"but it doesn't! ***\n\n");
		assert(0);
	}

	if (s->message.body_size &&
		http_body_is_final(p) &&
		!s->message.body_is_final) {
		syslog(LOG_ERROR, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");
		assert(0);
	}

	s->message.message_complete_cb_called = TRUE;
	return 0;
}

http_parser_settings websvr_settings =
{
	websvr_message_begin,
	websvr_request_url,
	websvr_response_status,
	websvr_header_field,
	websvr_header_value,
	websvr_headers_complete,
	websvr_body,
	websvr_message_complete,
};

/*
 *  ネットワーク層の選択
 */

#ifdef SUPPORT_INET6

#define TCP_ACP_CEP(c,r,d,t)	tcp6_acp_cep(c,r,d,t)
#define IP2STR(s,a)		ipv62str(s,a)

#else	/* of #ifdef SUPPORT_INET6 */

#ifdef SUPPORT_INET4

#define TCP_ACP_CEP(c,r,d,t)	tcp_acp_cep(c,r,d,t)
#define IP2STR(s,a)		ip2str(s,a)

#endif	/* of #ifdef SUPPORT_INET4 */

#endif	/* of #ifdef SUPPORT_INET6 */

struct httpd_state *get_httpd(ID cepid)
{
	for (int i = 0; i < 2; i++) {
		if (httpd_state[i].cepid != cepid)
			continue;

		return &httpd_state[i];
	}
	return NULL;
}

struct websocket *websocket_getws(ID wbsid)
{
	for (int i = 0; i < 2; i++) {
		if (httpd_state[i].websocket.wbsid != wbsid)
			continue;

		return &httpd_state[i].websocket;
	}
	return NULL;
}

void send_file(struct httpd_state *s)
{
	char *buf;
	int len, slen;

	while (s->file.len > 0) {
		slen = tcp_get_buf(s->cepid, (void **)&buf, TMO_FEVR);
		if (slen < 0) {
			syslog(LOG_ERROR, "send_file#tcp_get_buf(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, slen);
			s->state = STATE_CLOSING;
			break;
		}
		if (slen == 0)
			return;

		len = s->file.len;
		if (len > slen)
			len = slen;

		len = httpd_fs_read(&s->file, buf, len);
		if (len <= 0) {
			syslog(LOG_ERROR, "send_file#httpd_fs_read(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, len);
			break;
		}

		s->file.len -= len;
		s->file.pos += len;

		if ((slen = tcp_snd_buf(s->cepid, len)) != E_OK) {
			syslog(LOG_ERROR, "send_file#tcp_snd_buf(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, slen);
			s->state = STATE_CLOSING;
			break;
		}
	}

	printf("close:     %s.%d %s\n", s->addr, ((T_IPV4EP *)s->dst)->portno, s->filename);
	httpd_fs_close(&s->file);
	s->file.len = 0;
	s->file.pos = 0;

	s->out.state = OUT_STATE_SEND_END;
}

void send_data(struct httpd_state *s)
{
	char *buf;
	int len, slen;

	while (s->response_len > 0) {
		slen = tcp_get_buf(s->cepid, (void **)&buf, TMO_FEVR);
		if (slen < 0) {
			syslog(LOG_ERROR, "send_data#tcp_get_buf(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, slen);
			s->state = STATE_CLOSING;
			break;
		}
		if (slen == 0)
			return;

		len = s->response_len;
		if (len > slen)
			len = slen;

		memcpy(buf, &s->response_body[s->response_pos], len);

		s->response_len -= len;
		s->response_pos += len;

		if ((slen = tcp_snd_buf(s->cepid, len)) != E_OK) {
			syslog(LOG_ERROR, "send_data#tcp_snd_buf(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, slen);
			s->state = STATE_CLOSING;
			break;
		}
	}

	s->response_body = NULL;
	s->response_len = 0;
	s->response_pos = 0;

	s->out.state = OUT_STATE_SEND_END;
}

void send_headers(struct httpd_state *s, const char *statushdr)
{
	int len;
	char *ptr;

	len = strlen(statushdr);
	tcp_snd_dat(s->cepid, (void *)statushdr, len, TMO_FEVR);

	if ((s->filename[0] == '0') && (s->file.len > 0)) {
		len = sizeof(http_content_encoding_gzip) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_encoding_gzip, len, TMO_FEVR);
	}

	if (s->file.redirect) {
		len = sizeof(http_location) - 1;
		tcp_snd_dat(s->cepid, (void *)http_location, len, TMO_FEVR);
		if (s->filename[0] == '1') {
			len = 2;
			tcp_snd_dat(s->cepid, "/~", len, TMO_FEVR);
		}
		len = strlen(s->filename);
		tcp_snd_dat(s->cepid, s->filename, len, TMO_FEVR);
		if (s->query != NULL) {
			tcp_snd_dat(s->cepid, "?", 1, TMO_FEVR);
			len = strlen(s->query);
			tcp_snd_dat(s->cepid, s->query, len, TMO_FEVR);
		}
		len = 2;
		tcp_snd_dat(s->cepid, "\r", len, TMO_FEVR);
	}

	ptr = strrchr(s->filename, ISO_period);
	if (ptr == NULL) {
		len = sizeof(http_content_type_binary) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_binary, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_html, ptr, sizeof(http_html) - 1) == 0 ||
		ntlibc_strncmp(http_htm, ptr, sizeof(http_htm) - 1) == 0) {
		len = sizeof(http_content_type_html) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_html, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_css, ptr, sizeof(http_css) - 1) == 0) {
		len = sizeof(http_content_type_css) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_css, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_js, ptr, sizeof(http_js) - 1) == 0) {
		len = sizeof(http_content_type_js) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_js, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_json, ptr, sizeof(http_json) - 1) == 0) {
		len = sizeof(http_content_type_json) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_json, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_png, ptr, sizeof(http_png) - 1) == 0) {
		len = sizeof(http_content_type_png) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_png, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_gif, ptr, sizeof(http_gif) - 1) == 0) {
		len = sizeof(http_content_type_gif) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_gif, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_jpg, ptr, sizeof(http_jpg) - 1) == 0) {
		len = sizeof(http_content_type_jpg) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_jpg, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_svg, ptr, sizeof(http_svg) - 1) == 0) {
		len = sizeof(http_content_type_svg) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_svg, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_text, ptr, sizeof(http_text) - 1) == 0) {
		len = sizeof(http_content_type_text) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_text, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_eot, ptr, sizeof(http_eot) - 1) == 0) {
		len = sizeof(http_content_type_eot) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_eot, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_ttf, ptr, sizeof(http_ttf) - 1) == 0) {
		len = sizeof(http_content_type_ttf) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_ttf, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_woff, ptr, sizeof(http_woff) - 1) == 0) {
		len = sizeof(http_content_type_woff) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_woff, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_woff2, ptr, sizeof(http_woff2) - 1) == 0) {
		len = sizeof(http_content_type_woff2) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_woff2, len, TMO_FEVR);
	}
	else if (ntlibc_strncmp(http_ico, ptr, sizeof(http_ico) - 1) == 0) {
		len = sizeof(http_content_type_ico) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_ico, len, TMO_FEVR);
	}
	else {
		len = sizeof(http_content_type_plain) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_type_plain, len, TMO_FEVR);
	}

	if (s->file.len > 0) {
		len = sizeof(http_content_length) - 1;
		tcp_snd_dat(s->cepid, (void *)http_content_length, len, TMO_FEVR);
		snprintf(s->temp, sizeof(s->temp), "%d\r\n", s->file.len);
		tcp_snd_dat(s->cepid, (void *)s->temp, strlen(s->temp), TMO_FEVR);
	}

	if (s->message.should_keep_alive && s->reset == 0) {
		len = sizeof(http_connection_keep_alive) - 1;
		tcp_snd_dat(s->cepid, (void *)http_connection_keep_alive, len, TMO_FEVR);
	}
	else {
		len = sizeof(http_connection_close) - 1;
		tcp_snd_dat(s->cepid, (void *)http_connection_close, len, TMO_FEVR);
	}

	tcp_snd_dat(s->cepid, (void *)http_crnl, 2, TMO_FEVR);

	if (s->filename != NULL) {
		s->out.state = OUT_STATE_SEND_FILE;
	}
	else {
		s->out.state = OUT_STATE_SEND_DATA;
	}
}

void handle_output(struct httpd_state *s)
{
	s->out.wait = false;

	switch (s->out.state) {
	case OUT_STATE_WAIT_REQUEST:
		s->out.wait = true;
		break;
	case OUT_STATE_OPEN_GET_FILE:
		printf("open:      %s.%d %s\n", s->addr, ((T_IPV4EP *)s->dst)->portno, s->filename);
		if (!httpd_fs_open(s->filename, sizeof(s->message.request_url), &s->file)) {
			s->filename = NULL;
			s->response_body = http_content_404;
			s->response_pos = 0;
			s->response_len = sizeof(http_content_403) - 1;
			s->out.statushdr = http_header_404;
		}
		else {
			s->out.statushdr = s->file.redirect ? http_header_301 : http_header_200;
		}
		s->out.state = OUT_STATE_SEND_HEADER;
		break;
	case OUT_STATE_WAIT_POST_BODY:
		s->out.wait = true;
		break;
	case OUT_STATE_BODY_RECEIVED:
		s->out.statushdr = http_header_200;
		s->out.state = OUT_STATE_SEND_HEADER;
		break;
	case OUT_STATE_SEND_HEADER:
		send_headers(s, s->out.statushdr);
		break;
	case OUT_STATE_SEND_FILE:
		send_file(s);
		break;
	case OUT_STATE_SEND_DATA:
		send_data(s);
		break;
	case OUT_STATE_SEND_END:
		s->out.wait = true;
		if (s->message.should_keep_alive && s->reset == 0) {
			s->out.state = OUT_STATE_WAIT_REQUEST;
		}
		else {
			s->state = STATE_CLOSING;
		}
		break;
	}
}

void send_ws_headers(struct httpd_state *s, const char *statushdr)
{
	int len;

	len = strlen(statushdr);
	tcp_snd_dat(s->cepid, (void *)statushdr, len, TMO_FEVR);

	len = sizeof(http_upgrade) - 1;
	tcp_snd_dat(s->cepid, (void *)http_upgrade, len, TMO_FEVR);
	len = strlen(s->message.upgrade);
	tcp_snd_dat(s->cepid, s->message.upgrade, len, TMO_FEVR);
	len = sizeof(http_crnl) - 1;
	tcp_snd_dat(s->cepid, (void *)http_crnl, len, TMO_FEVR);

	len = sizeof(http_connection) - 1;
	tcp_snd_dat(s->cepid, (void *)http_connection, len, TMO_FEVR);
	len = strlen(s->message.connection);
	tcp_snd_dat(s->cepid, s->message.connection, len, TMO_FEVR);
	len = sizeof(http_crnl) - 1;
	tcp_snd_dat(s->cepid, (void *)http_crnl, len, TMO_FEVR);

	len = sizeof(http_sec_websocket_accept) - 1;
	tcp_snd_dat(s->cepid, (void *)http_sec_websocket_accept, len, TMO_FEVR);
	len = strlen(s->message.response_key);
	tcp_snd_dat(s->cepid, s->message.response_key, len, TMO_FEVR);
	len = sizeof(http_crnl) - 1;
	tcp_snd_dat(s->cepid, (void *)http_crnl, len, TMO_FEVR);

	len = sizeof(http_sec_websocket_protocol) - 1;
	tcp_snd_dat(s->cepid, (void *)http_sec_websocket_protocol, len, TMO_FEVR);
	len = strlen(s->message.sec_websocket_protocol);
	tcp_snd_dat(s->cepid, s->message.sec_websocket_protocol, len, TMO_FEVR);
	len = sizeof(http_crnl) - 1;
	tcp_snd_dat(s->cepid, (void *)http_crnl, len, TMO_FEVR);

	len = sizeof(http_crnl) - 1;
	tcp_snd_dat(s->cepid, (void *)http_crnl, len, TMO_FEVR);
}

void send_ws_data(struct httpd_state *s)
{
	char *buf;
	int slen;

	slen = tcp_get_buf(s->cepid, (void **)&buf, TMO_FEVR);
	if (slen < 0) {
		syslog(LOG_ERROR, "send_ws_data#tcp_get_buf(%s.%d) => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, slen);
		return;
	}

	websocket_output(&s->websocket, buf, slen);
}

void handle_ws_output(struct httpd_state *s)
{
	char shaHash[20];
	SHA_CTX sha1;
	int len;

	strlncat(s->message.response_key, sizeof(s->message.response_key),
		s->message.sec_websocket_key, sizeof(s->message.sec_websocket_key));
	len = strlncat(s->message.response_key, sizeof(s->message.response_key),
		http_websocket_guid, sizeof(http_websocket_guid));
	memset(shaHash, 0, sizeof(shaHash));
	SHA1_Init(&sha1);
	SHA1_Update(&sha1, (sha1_byte *)s->message.response_key, len);
	SHA1_Final((sha1_byte *)shaHash, &sha1);
	base64_encode((unsigned char *)s->message.response_key,
		sizeof(s->message.response_key), (unsigned char *)shaHash, sizeof(shaHash));

	send_ws_headers(s, http_header_101);

	s->message.response_key[0] = '\0';

	do {
		while (!websocket_newdata(&s->websocket))
			slp_tsk();

		send_ws_data(s);
	} while ((s->state == STATE_CONNECTED) && (!s->close_req));
	s->state = STATE_DISCONNECTED;
	websocket_destroy(&s->websocket);
	s->close_req = 0;

	s->state = STATE_CLOSING;
}

void handle_input(struct httpd_state *s)
{
	size_t done;
	int len;

	s->in.wait = false;

	switch (s->in.state) {
	case IN_STATE_START:
		http_parser_init(&s->parser, HTTP_REQUEST);
		s->in.state = IN_STATE_REQUEST;
		break;
	case IN_STATE_REQUEST:
	case IN_STATE_RESPONSE:
	case IN_STATE_UPLOAD:
		if ((len = tcp_rcv_buf(s->cepid, (void **)&s->in.data, TMO_POL)) <= 0) {
			if ((len == E_TMOUT) || (len == 0)) {
				// 3秒は待つ
				//if (httpd_time - s->in.timer < 30000000) {
					s->in.wait = true;
					break;
				//}
			}
			syslog(LOG_ERROR, "handle_input#tcp_rcv_buf#%d(%s.%d) => %d", s->in.state, s->addr, ((T_IPV4EP *)s->dst)->portno, len);
			uploding = NULL;
			s->state = STATE_CLOSING;
			return;
		}
		done = http_parser_execute(&s->parser, &websvr_settings, s->in.data, len);
		tcp_rel_buf(s->cepid, done);
		if (s->parser.http_errno != HPE_OK) {
			syslog(LOG_ERROR, "http_parser error %s.%d => %d", s->addr, ((T_IPV4EP *)s->dst)->portno, s->parser.http_errno);
			uploding = NULL;
			s->state = STATE_CLOSING;
			return;
		}

		s->parse_pos = done;
		s->parse_len = len - done;
		break;
	case IN_STATE_UPLOAD_WAIT:
		if (uploding != NULL) {
			s->in.wait = true;
		}
		else {
			uploding = s;
			s->in.state = IN_STATE_UPLOAD;
		}
		break;
	case IN_STATE_WEBSOCKET:
		if (s->parse_len <= 0) {
			if ((len = tcp_rcv_buf(s->cepid, (void **)&s->in.data, TMO_POL)) <= 0) {
				if ((len == E_TMOUT) || (len == 0)) {
					s->in.wait = true;
					break;
				}
				syslog(LOG_ERROR, "handle_input#tcp_rcv_buf#%d(%s.%d) => %d", s->in.state, s->addr, ((T_IPV4EP *)s->dst)->portno, len);
				s->state = STATE_CLOSING;
				break;
			}

			s->parse_pos = 0;
			s->parse_len = len;
		}
		else
			len = s->parse_len;
		done = websocket_input(&s->websocket, (void *)s->in.data, s->parse_len);
		tcp_rel_buf(s->cepid, done);
		if ((done != 0) || (s->websocket.rstate.opecode == connection_close)) {
			s->close_req = 1;
			s->state = STATE_CLOSING;
			break;
		}
		s->parse_pos = done;
		s->parse_len -= done;
		break;
	case IN_STATE_END:
		s->in.wait = true;
		break;
	default:
		s->state = STATE_CLOSING;
		break;
	}
}

/*
 *  ノンブロッキングコールのコールバック関数
 */
ER
callback_nblk_tcp(ID cepid, FN fncd, void *p_parblk)
{
	struct httpd_state *s = get_httpd(cepid);

	if (s == NULL)
		printf("callback_nblk_tcp(%d, %d)\n", fncd, cepid);
	else
		printf("callback_nblk_tcp(%d, %s.%d)\n", fncd, s->addr, ((T_IPV4EP *)s->dst)->portno);

	return E_PAR;
}

/*
 * HTTPサーバータスク
 */
void httpd_task(intptr_t exinf)
{
	ER ret, ret2;
	struct httpd_state *s = &httpd_state[exinf];

	for (;;) {
		ret2 = get_tim(&httpd_time);
		if (ret2 != E_OK) {
			syslog(LOG_ERROR, "get_tim");
			return;
		}

		switch (s->state) {
		case STATE_DISCONNECTED:
			memset(&s->dst, 0, sizeof(s->dst));
			if ((ret = TCP_ACP_CEP(s->cepid, TCP_REPID, (T_IPV4EP *)s->dst, TMO_FEVR)) != E_OK) {
				syslog(LOG_ERROR, "tcp_acp_cep(%d) => %d", s->cepid, ret);
				tslp_tsk(100 * 1000);	// TODO
				s->state = STATE_CLOSING;
				break;
			}
			IP2STR(s->addr, &((T_IPV4EP *)s->dst)->ipaddr);
			printf("connected: %s.%d\n", s->addr, ((T_IPV4EP *)s->dst)->portno);
			memset(&s->in, 0, sizeof(s->in));
			memset(&s->out, 0, sizeof(s->out));
			s->in.timer = httpd_time;
			s->state = STATE_CONNECTED;
			break;
		case STATE_CONNECTED:
			handle_input(s);
			handle_output(s);
			break;
		case STATE_WEBSOCKET:
			handle_input(s);
			handle_ws_output(s);
			break;
		case STATE_CLOSING:
			printf("close:     %s.%d\n", s->addr, ((T_IPV4EP *)s->dst)->portno);
			tcp_sht_cep(s->cepid);
			tcp_cls_cep(s->cepid, TMO_FEVR);

			if (s->reset) {
				s->reset = 0;
				s->state = STATE_RESET;
			}
			else {
				s->state = STATE_DISCONNECTED;
			}
			break;
		case STATE_RESET:
			execute_command(0);
			s->state = STATE_DISCONNECTED;
			break;
		}

		if (s->in.wait && s->out.wait) {
			tslp_tsk(100 * 1000);
		}
	}
}
