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
 *  @(#) $Id: stdio_stub.c 1876 2019-04-11 00:43:01Z coas-nagasima $
 */
#include "shellif.h"
#include <stdint.h>
#include <kernel.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include <sil.h>
#include "syssvc/serial.h"
#include "syssvc/syslog.h"
#include "target_syssvc.h"
#include "fdtable.h"
#include "kernel_cfg.h"
#include <string.h>
#include "util/ntstdio.h"
#include "hal/serial_api.h"

#ifdef _DEBUG
static const char THIS_FILE[] = __FILE__;
#endif

static int stdio_close(struct SHELL_FILE *fp);
static size_t stdio_read(struct SHELL_FILE *fp, unsigned char *data, size_t len);
static size_t stdio_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len);
static size_t stdin_read(struct SHELL_FILE *fp, unsigned char *data, size_t len);
static size_t stdout_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len);
static size_t stderr_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len);
static void stdio_delete(struct SHELL_FILE *fp);

static int sio_close(struct SHELL_FILE *fp);
static size_t sio_read(struct SHELL_FILE *fp, unsigned char *data, size_t len);
static size_t sio_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len);
static off_t sio_seek(struct SHELL_FILE *fp, off_t ofs, int org);
static int sio_ioctl(struct SHELL_FILE *fp, int req, void *arg);
static bool_t sio_readable(struct SHELL_FILE *fp);
static void sio_delete(struct SHELL_FILE *fp);

IO_TYPE IO_TYPE_STDIN = { stdio_close, stdin_read, stdio_write, sio_seek, sio_ioctl, sio_readable, stdio_delete };
IO_TYPE IO_TYPE_STDOUT = { stdio_close, stdio_read, stdout_write, sio_seek, sio_ioctl, sio_readable, stdio_delete };
IO_TYPE IO_TYPE_STDERR = { stdio_close, stdio_read, stderr_write, sio_seek, sio_ioctl, sio_readable, stdio_delete };
IO_TYPE IO_TYPE_SIO = { sio_close, sio_read, sio_write, sio_seek, sio_ioctl, sio_readable, sio_delete };

ntstdio_t ntstdio;
extern serial_t stdio_uart;

unsigned char ntstdio_xi(struct ntstdio_t *handle)
{
	return serial_getc((serial_t *)handle->exinf);
}

void ntstdio_xo(struct ntstdio_t *handle, unsigned char c)
{
	serial_putc((serial_t *)handle->exinf, c);
}

void sys_init(intptr_t exinf)
{
	ntstdio_init(&ntstdio, NTSTDIO_OPTION_LINE_ECHO | NTSTDIO_OPTION_CANON | NTSTDIO_OPTION_LF_CRLF | NTSTDIO_OPTION_LF_CR, ntstdio_xi, ntstdio_xo);
	ntstdio.exinf = (void *)&stdio_uart;
}

int stdio_close(struct SHELL_FILE *fp)
{
	return -EPERM;
}

size_t stdio_read(struct SHELL_FILE *fp, unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t stdio_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t stdin_read(struct SHELL_FILE *fp, unsigned char *data, size_t len)
{
	int i = 0;
	while (i < len) {
		int c = ntstdio_getc((struct ntstdio_t *)fp->exinf);
		data[i++] = c;
		if ((c == EOF) || (c == '\n'))
			break;
	}
	return i;
}

size_t stdout_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len)
{
	for (int i = 0; i < len; i++) {
		ntstdio_putc((struct ntstdio_t *)fp->exinf, data[i]);
	}
	return len;
}

size_t stderr_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len)
{
	for (int i = 0; i < len; i++) {
		ntstdio_putc((struct ntstdio_t *)fp->exinf, data[i]);
	}
	return len;
}

void stdio_delete(struct SHELL_FILE *fp)
{
}

int sio_close(struct SHELL_FILE *fp)
{
	return -EPERM;
}

size_t sio_read(struct SHELL_FILE *fp, unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t sio_write(struct SHELL_FILE *fp, const unsigned char *data, size_t len)
{
	return -EPERM;
}

off_t sio_seek(struct SHELL_FILE *fp, off_t ofs, int org)
{
	return -EPERM;
}

int sio_tcgetattr(int fd, struct termios *termios)
{
	struct SHELL_FILE *fp = fd_to_fp(fd);
	if ((fp == NULL) || (fp->type != &IO_TYPE_SIO))
		return -EBADF;

	ntstdio_t *ntstdio = (ntstdio_t *)fp->exinf;

	memset(termios, 0, sizeof(*termios));

	if (ntstdio->option & NTSTDIO_OPTION_LINE_ECHO) {
		termios->c_lflag |= ECHO;
	}
	else {
		termios->c_lflag &= ~ECHO;
	}
	if (ntstdio->option & NTSTDIO_OPTION_CANON) {
		termios->c_lflag |= ICANON;
	}
	else {
		termios->c_lflag &= ~ICANON;
	}
	if (ntstdio->option & NTSTDIO_OPTION_LF_CR) {
		termios->c_iflag |= INLCR;
	}
	else {
		termios->c_iflag &= ~INLCR;
	}
	if (ntstdio->option & NTSTDIO_OPTION_LF_CRLF) {
		termios->c_oflag |= ONLCR;
	}
	else {
		termios->c_oflag &= ~ONLCR;
	}

	return 0;
}

int sio_tcsetattr(int fd, int optional_actions, const struct termios *termios)
{
	struct SHELL_FILE *fp = fd_to_fp(fd);
	if ((fp == NULL) || (fp->type != &IO_TYPE_SIO))
		return -EBADF;

	ntstdio_t *ntstdio = (ntstdio_t *)fp->exinf;

	if (optional_actions == TCSANOW) {
		if (termios->c_lflag & ECHO) {
			ntstdio->option |= NTSTDIO_OPTION_LINE_ECHO;
		}
		else {
			ntstdio->option &= ~NTSTDIO_OPTION_LINE_ECHO;
		}
		if (termios->c_lflag & ICANON) {
			ntstdio->option |= NTSTDIO_OPTION_CANON;
		}
		else {
			ntstdio->option &= ~NTSTDIO_OPTION_CANON;
		}
		if (termios->c_iflag & INLCR) {
			ntstdio->option |= NTSTDIO_OPTION_LF_CR;
		}
		else {
			ntstdio->option &= ~NTSTDIO_OPTION_LF_CR;
		}
		if (termios->c_oflag & ONLCR) {
			ntstdio->option |= NTSTDIO_OPTION_LF_CRLF;
		}
		else {
			ntstdio->option &= ~NTSTDIO_OPTION_LF_CRLF;
		}
		return 0;
	}

	shell_abort();
	return 0;
}

int sio_ioctl(struct SHELL_FILE *fp, int request, void *arg)
{
	switch (request) {
	case TIOCGWINSZ:
		return 0;
	case TCGETS:
		return sio_tcgetattr(fp->fd, (struct termios *)arg);
	case TCSETS + TCSANOW:
	case TCSETS + TCSADRAIN:
	case TCSETS + TCSAFLUSH:
		return sio_tcsetattr(fp->fd, request - TCSETS, (const struct termios *)arg);
	}

	return -EINVAL;
}

bool_t sio_readable(struct SHELL_FILE *fp)
{
	return fp->readevt_w != fp->readevt_r;
}

void sio_delete(struct SHELL_FILE *fp)
{
	free((serial_t *)((struct ntstdio_t *)fp->exinf)->exinf);
	((struct ntstdio_t *)fp->exinf)->exinf = NULL;
	free((struct ntstdio_t *)fp->exinf);
	fp->exinf = NULL;
}
