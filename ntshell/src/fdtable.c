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
 *  @(#) $Id: fdtable.c 1781 2019-02-01 00:02:42Z coas-nagasima $
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
#ifndef NTSHELL_NO_SOCKET
#include <tinet_defs.h>
#include <tinet_config.h>
#include <net/net.h>
#include <net/net_endian.h>
#include <netinet/in.h>
#include <netinet/in_itron.h>
#include <tinet_nic_defs.h>
#include <tinet_cfg.h>
#include <netinet/in_var.h>
#include <net/ethernet.h>
#include <net/if6_var.h>
#include <net/net.h>
#include <net/if_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#include <net/net_buf.h>
#endif
#include "ff.h"
#include "socket_stub.h"
#include "kernel_cfg.h"
#include <string.h>

#define IO_TYPE_FREE	0
#define IO_TYPE_SIO		1
#define IO_TYPE_FILE	2
#define IO_TYPE_DIR		3
#define IO_TYPE_TCP		4
#define IO_TYPE_UDP	5

static struct _IO_FILE fd_table[8 * sizeof(FLGPTN)] = {
	{ 0, IO_TYPE_SIO, 0, stdio_close, stdin_read, stdio_write, sio_seek, sio_ioctl },
	{ 1, IO_TYPE_SIO, 0, stdio_close, stdio_read, stdout_write, sio_seek, sio_ioctl },
	{ 2, IO_TYPE_SIO, 0, stdio_close, stdio_read, stderr_write, sio_seek, sio_ioctl },
};
#define fd_table_count (sizeof(fd_table) / sizeof(fd_table[0]))

static int new_fd(int type, int id)
{
	for (int fd = 3; fd < fd_table_count; fd++) {
		struct _IO_FILE *fp = &fd_table[fd];
		if (fp->type != IO_TYPE_FREE)
			continue;

		memset(fp, 0, sizeof(struct _IO_FILE));
		fp->fd = fd;
		fp->type = type;
		fp->handle = id;
		return fd;
	}

	return -ENOMEM;
}

static struct _IO_FILE *id_to_fd(int type, int id)
{
	for (int fd = 3; fd < fd_table_count; fd++) {
		struct _IO_FILE *fp = &fd_table[fd];
		if ((fp->type == type) && (fp->handle == id))
			return fp;
	}

	return NULL;
}

static int delete_fd(int type, int id)
{
	struct _IO_FILE *fp = id_to_fd(type, id);
	if (fp == NULL)
		return -EBADF;

	return delete_fp(fp);
}

int delete_fp(struct _IO_FILE *fp)
{
	free(fp->pfile);
	fp->pfile = NULL;
	free(fp->pdir);
	fp->pdir = NULL;
	free(fp->psock);
	fp->psock = NULL;
	memset(fp, 0, sizeof(struct _IO_FILE));

	return 0;
}

struct _IO_FILE *fd_to_fp(int fd)
{
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;
	return &fd_table[fd];
}

struct _IO_FILE *new_sio_fd(int sioid)
{
	int fd = new_fd(IO_TYPE_SIO, sioid);
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;

	struct _IO_FILE *fp = &fd_table[fd];
	fp->close = sio_close;
	fp->read = sio_read;
	fp->write = sio_write;
	fp->seek = sio_seek;
	fp->ioctl = sio_ioctl;
	fp->writable = 1;

	return fp;
}

int delete_sio_fd(int sioid)
{
	return delete_fd(IO_TYPE_SIO, sioid);
}

struct _IO_FILE *sioid_to_fd(int sioid)
{
	return id_to_fd(IO_TYPE_SIO, sioid);
}

struct _IO_FILE *new_file_fd(int fileid)
{
	int fd = new_fd(IO_TYPE_FILE, fileid);
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;

	struct _IO_FILE *fp = &fd_table[fd];
	fp->close = file_close;
	fp->read = file_read;
	fp->write = file_write;
	fp->seek = file_seek;
	fp->ioctl = file_ioctl;
	fp->writable = 1;
	fp->pfile = malloc(sizeof(FIL));
	memset(fp->pfile, 0, sizeof(FIL));

	return fp;
}

int delete_file_fd(int fileid)
{
	return delete_fd(IO_TYPE_FILE, fileid);
}

struct _IO_FILE *fileid_to_fd(int fileid)
{
	return id_to_fd(IO_TYPE_FILE, fileid);
}

struct _IO_FILE *new_dir_fd(int fileid)
{
	int fd = new_fd(IO_TYPE_DIR, fileid);
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;

	struct _IO_FILE *fp = &fd_table[fd];
	fp->close = dir_close;
	fp->read = dir_read;
	fp->write = dir_write;
	fp->seek = dir_seek;
	fp->ioctl = dir_ioctl;
	fp->writable = 0;
	fp->pdir = malloc(sizeof(struct _IO_DIR));
	memset(fp->pdir, 0, sizeof(struct _IO_DIR));

	return fp;
}

int delete_dir_fd(int dirid)
{
	return delete_fd(IO_TYPE_DIR, dirid);
}

struct _IO_FILE *dirid_to_fd(int dirid)
{
	return id_to_fd(IO_TYPE_DIR, dirid);
}

#ifndef NTSHELL_NO_SOCKET
struct _IO_FILE *new_tcp_fd(int tcpid)
{
	int fd = new_fd(IO_TYPE_TCP, tcpid);
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;

	struct _IO_FILE *fp = &fd_table[fd];
	fp->close = tcp_fd_close;
	fp->read = tcp_fd_read;
	fp->write = tcp_fd_write;
	fp->seek = tcp_fd_seek;
	fp->ioctl = tcp_fd_ioctl;
	fp->writable = 0;
	fp->psock = malloc(sizeof(socket_t));
	memset(fp->psock, 0, sizeof(socket_t));

	return fp;
}

int delete_tcp_fd(int tcpid)
{
	return delete_fd(IO_TYPE_TCP, tcpid);
}

struct _IO_FILE *tcpid_to_fd(int tcpid)
{
	return id_to_fd(IO_TYPE_TCP, tcpid);
}

struct _IO_FILE *new_udp_fd(int udpid)
{
	int fd = new_fd(IO_TYPE_UDP, udpid);
	if ((fd < 0) || (fd >= fd_table_count))
		return NULL;

	struct _IO_FILE *fp = &fd_table[fd];
	fp->close = udp_fd_close;
	fp->read = udp_fd_read;
	fp->write = udp_fd_write;
	fp->seek = udp_fd_seek;
	fp->ioctl = udp_fd_ioctl;
	fp->writable = 1;
	fp->psock = malloc(sizeof(socket_t));
	memset(fp->psock, 0, sizeof(socket_t));

	return fp;
}

int delete_udp_fd(int udpid)
{
	return delete_fd(IO_TYPE_UDP, udpid);
}

struct _IO_FILE *udpid_to_fd(int udpid)
{
	return id_to_fd(IO_TYPE_UDP, udpid);
}

#endif

void memor(void *dst, void *src, size_t len)
{
	uint8_t *d = (uint8_t *)dst;
	uint8_t *s = (uint8_t *)src;
	uint8_t *e = &s[len];

	while (s < e) {
		*d++ |= *s++;
	}
}

struct fd_events {
	int count;
	fd_set readfds;
	fd_set writefds;
	fd_set errorfds;
};

ER shell_get_evts(struct fd_events *evts, TMO tmout);

#define TMO_MAX INT_MAX

int shell_select(int n, fd_set *__restrict rfds, fd_set *__restrict wfds, fd_set *__restrict efds, struct timeval *__restrict tv)
{
	ER ret;
	TMO tmout = TMO_FEVR;
	struct fd_events evts;

	if (tv != NULL) {
		if (tv->tv_sec < (TMO_MAX / 1000000))
			tmout = tv->tv_sec * 1000000 + tv->tv_usec;
		else
			tmout = TMO_MAX;
	}

	if (rfds != NULL)
		memcpy(&evts.readfds, rfds, sizeof(fd_set));
	else
		memset(&evts.readfds, 0, sizeof(fd_set));
	if (wfds != NULL)
		memcpy(&evts.writefds, wfds, sizeof(fd_set));
	else
		memset(&evts.writefds, 0, sizeof(fd_set));
	if (efds != NULL)
		memcpy(&evts.errorfds, efds, sizeof(fd_set));
	else
		memset(&evts.errorfds, 0, sizeof(fd_set));
	evts.count = 0;

	ret = shell_get_evts(&evts, tmout);
	if (rfds != NULL)
		memset(rfds, 0, sizeof(fd_set));
	if (wfds != NULL)
		memset(wfds, 0, sizeof(fd_set));
	if (efds != NULL)
		memset(efds, 0, sizeof(fd_set));
	if (ret == E_OK) {
		if (rfds != NULL)
			memor(rfds, &evts.readfds, sizeof(fd_set));
		if (wfds != NULL)
			memor(wfds, &evts.writefds, sizeof(fd_set));
		if (efds != NULL)
			memor(efds, &evts.errorfds, sizeof(fd_set));
		return evts.count;
	}
	if (ret == E_TMOUT) {
		return 0;
	}

	return -EBADF;
}

int shell_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	ER ret;
	TMO tmout;
	struct fd_events evts;

	if(timeout < 0)
		tmout = TMO_FEVR;
	else if (timeout < (TMO_MAX / 1000))
		tmout = timeout * 1000;
	else
		tmout = TMO_MAX;

	memset(&evts, 0, sizeof(evts));

	for (int i = 0; i < nfds; i++) {
		struct pollfd *pfd = &fds[i];
		int fd = pfd->fd;
		if ((fd < 0) || (fd >= fd_table_count))
			continue;

		if (pfd->events & POLLIN)
			FD_SET(fd, &evts.readfds);
		if (pfd->events & POLLOUT)
			FD_SET(fd, &evts.writefds);
		if (pfd->events & POLLERR)
			FD_SET(fd, &evts.errorfds);
		pfd->revents = 0;
	}

	ret = shell_get_evts(&evts, tmout);
	if (ret == E_OK) {
		int result = 0;
		for (int i = 0; i < nfds; i++) {
			struct pollfd *pfd = &fds[i];
			int fd = pfd->fd;
			if ((fd < 0) || (fd >= fd_table_count))
				continue;

			if (FD_ISSET(fd, &evts.readfds))
				pfd->revents |= POLLIN;
			if (FD_ISSET(fd, &evts.writefds))
				pfd->revents |= POLLOUT;
			if (FD_ISSET(fd, &evts.errorfds))
				pfd->revents |= POLLERR;
			if (pfd->revents != 0)
				result++;
		}
		return result;
	}
	if (ret == E_TMOUT) {
		return 0;
	}

	return -EBADF;
}

/* TODO:コールバック化したい */
void stdio_update_evts()
{
	int fd = STDIN_FILENO;
	struct _IO_FILE *fp = &fd_table[fd];
	T_SERIAL_RPOR rpor;
	FLGPTN flgptn = 0;

	ER ret = serial_ref_por(SIO_PORTID, &rpor);
	if (ret != E_OK)
		return;

	if (rpor.reacnt != 0) {
		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		FD_SET(fd, (fd_set *)&flgptn);
	}
	if (rpor.wricnt != 0) {
		if (fp->writeevt_w == fp->writeevt_r) fp->writeevt_w++;

		FD_SET(fd, (fd_set *)&flgptn);
	}

	if (flgptn != 0) {
		set_flg(FLG_SELECT_WAIT, flgptn);
	}
}

/* TODO:コールバック化したい */
void stdio_flgptn(FLGPTN *flgptn)
{
	int fd = STDIN_FILENO;
	struct _IO_FILE *fp = &fd_table[fd];
	T_SERIAL_RPOR rpor;
	*flgptn = 0;

	ER ret = serial_ref_por(SIO_PORTID, &rpor);
	if (ret != E_OK)
		return;

	if (rpor.reacnt != 0) {
		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		FD_SET(fd, (fd_set *)flgptn);
	}
	if (rpor.wricnt != 0) {
		if (fp->writeevt_w == fp->writeevt_r) fp->writeevt_w++;

		FD_SET(fd, (fd_set *)flgptn);
	}
}

#ifndef NTSHELL_NO_SOCKET

ER socket_tcp_callback(ID cepid, FN fncd, void *p_parblk)
{
	struct _IO_FILE *fp = tcpid_to_fd(cepid);
	FLGPTN flgptn = 0;
	ER ret;
	int len;

	if (fp == NULL)
		return E_PAR;

	int fd = fp->fd;
	FD_SET(fd, (fd_set *)&flgptn);

	switch (fncd) {
	case TFN_TCP_RCV_BUF:
		len = *(int *)p_parblk;
		if (len <= 0)
			return E_OK;

		ret = wai_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "wai_sem => %d", ret);
		}
		fp->psock->len += len;
		ret = sig_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "sig_sem => %d", ret);
		}

		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_TCP_RCV_DAT:
		len = *(int *)p_parblk;
		if (len <= 0)
			return E_OK;

		ret = wai_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "wai_sem => %d", ret);
		}
		fp->psock->len += len;
		ret = sig_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "sig_sem => %d", ret);
		}

		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_TCP_SND_DAT:
		if (fp->writeevt_w == fp->writeevt_r) fp->writeevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_TCP_CAN_CEP:
		if (fp->errorevt_w == fp->errorevt_r) fp->errorevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_TCP_DEL_REP:
		delete_tcp_rep(cepid);
		return E_OK;

	case TFN_TCP_DEL_CEP:
		delete_tcp_fd(cepid);
		return E_OK;

	default:
		return E_OK;
	}
}

ER socket_udp_callback(ID cepid, FN fncd, void *p_parblk)
{
	struct _IO_FILE *fp = udpid_to_fd(cepid);
	FLGPTN flgptn = 0;
	int len;

	if (fp == NULL)
		return E_PAR;

	int fd = fp->fd;
	FD_SET(fd, (fd_set *)&flgptn);

	switch (fncd) {
	case TEV_UDP_RCV_DAT:
	{
		T_UDP_RCV_DAT_PARA *udppara = (T_UDP_RCV_DAT_PARA *)p_parblk;
		len = udppara->len;
		if (len <= 0)
			return E_OK;

		ER ret = wai_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "wai_sem => %d", ret);
		}
		fp->psock->len = len;
		if (fp->psock->input != NULL) {
			ret = rel_net_buf(fp->psock->input);
			if (ret < 0) {
				syslog(LOG_ERROR, "rel_net_buf => %d", ret);
			}
		}
		fp->psock->input = udppara->input;
		fp->psock->buf = GET_UDP_SDU(udppara->input, udppara->off);
		memset(&fp->psock->raddr4, 0, sizeof(fp->psock->raddr4));
		fp->psock->raddr4.sin_family = AF_INET;
		fp->psock->raddr4.sin_port = htons(udppara->rep4.portno);
		fp->psock->raddr4.sin_addr.s_addr = htonl(udppara->rep4.ipaddr);
		udppara->input->flags |= NB_FLG_NOREL_IFOUT;
		ret = sig_sem(SEM_FILEDESC);
		if (ret < 0) {
			syslog(LOG_ERROR, "sig_sem => %d", ret);
		}

		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;
	}
	case TFN_UDP_CRE_CEP:
		return E_OK;

	case TFN_UDP_RCV_DAT:
		len = *(int *)p_parblk;
		if (len <= 0)
			return E_OK;

		if (fp->readevt_w == fp->readevt_r) fp->readevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_UDP_SND_DAT:
		if (fp->writeevt_w == fp->writeevt_r) fp->writeevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_UDP_CAN_CEP:
		if (fp->errorevt_w == fp->errorevt_r) fp->errorevt_w++;

		set_flg(FLG_SELECT_WAIT, flgptn);
		return E_OK;

	case TFN_UDP_DEL_CEP:
		delete_udp_fd(cepid);
		return E_OK;

	default:
		return E_OK;
	}
}

#endif

ER shell_get_evts(struct fd_events *evts, TMO tmout)
{
	int count = 0;
	SYSTIM prev, now;

	get_tim(&prev);

	for (;;) {
		ER ret;
		FLGPTN waitptn, flgptn, readfds = 0, writefds = 0;
		struct _IO_FILE *fp = NULL;

		stdio_update_evts();

#ifndef NTSHELL_NO_SOCKET
		waitptn = *((FLGPTN *)&evts->errorfds);
#else
		waitptn = *((FLGPTN *)&evts->readfds) | *((FLGPTN *)&evts->errorfds);
#endif
		for (int fd = 0; fd < fd_table_count; fd++) {
			fp = &fd_table[fd];

#ifndef NTSHELL_NO_SOCKET
			if (FD_ISSET(fd, &evts->readfds)) {
				if ((fp->type == IO_TYPE_TCP) && (fp->psock->cepid != 0)) {
					if (fp->psock->len == 0) {
						ret = tcp_rcv_buf(fp->psock->cepid, &fp->psock->input, TMO_NBLK);
						if ((ret != E_WBLK) && (ret != E_OBJ) && (ret < 0)) {
							syslog(LOG_ERROR, "tcp_rcv_buf => %d", ret);
							//return ret;
						}
						if (ret > 0) {
							ret = wai_sem(SEM_FILEDESC);
							if (ret < 0) {
								syslog(LOG_ERROR, "wai_sem => %d", ret);
							}
							fp->psock->len += ret;
							ret = sig_sem(SEM_FILEDESC);
							if (ret < 0) {
								syslog(LOG_ERROR, "sig_sem => %d", ret);
							}
						}
					}
					else ret = 1;
					if (ret > 0) {
						FD_SET(fd, (fd_set *)&readfds);
						count++;
						if (fp->readevt_w == fp->readevt_r) fp->readevt_r--;
					}
				}
				else if ((fp->type == IO_TYPE_UDP) && (fp->psock->cepid != 0)) {
					if (fp->psock->input != NULL) {
						FD_SET(fd, (fd_set *)&readfds);
						count++;
						if (fp->readevt_w == fp->readevt_r) fp->readevt_r--;
					}
				}
				else {
					FD_SET(fd, (fd_set *)&waitptn);
				}
			}
#endif
			if (FD_ISSET(fd, &evts->writefds)) {
				if (fp->writeevt_w == fp->writeevt_r) {
					FD_SET(fd, (fd_set *)&writefds);
					count++;
					if (fp->writeevt_w == fp->writeevt_r) fp->writeevt_r--;
				}
				else {
					FD_SET(fd, (fd_set *)&waitptn);
				}
			}
		}
		memset(evts, 0, sizeof(*evts));

		if (waitptn == 0) {
			memcpy(&evts->readfds, &readfds, sizeof(evts->readfds));
			memcpy(&evts->writefds, &writefds, sizeof(evts->writefds));
			evts->count = count;
			return E_OK;
		}
		else if ((readfds | writefds) != 0) {
			set_flg(FLG_SELECT_WAIT, (readfds | writefds));
		}

		/* イベント待ち */
		flgptn = 0;
		ret = twai_flg(FLG_SELECT_WAIT, waitptn, TWF_ORW, &flgptn, tmout);
		if (ret != E_OK) {
			if (ret != E_TMOUT) {
				syslog(LOG_ERROR, "twai_flg => %d", ret);
				return ret;
			}

			stdio_flgptn(&flgptn);

			if (flgptn == 0)
				return E_TMOUT;
		}
		flgptn &= waitptn;

		/* 受け取ったフラグのみクリア */
		ret = clr_flg(FLG_SELECT_WAIT, ~flgptn);
		if (ret != E_OK) {
			syslog(LOG_ERROR, "clr_flg => %d", ret);
		}

		count = 0;
		for (int fd = 0; fd < fd_table_count; fd++) {
			if (!FD_ISSET(fd, (fd_set *)&waitptn))
				continue;

			fp = &fd_table[fd];

			if (fp->readevt_w != fp->readevt_r) {
				fp->readevt_r++;
				FD_SET(fd, &evts->readfds);
				count++;
			}
			if (fp->writeevt_w != fp->writeevt_r) {
				fp->writeevt_r++;
				fp->writable = 1;
			}
			if (fp->writable) {
				FD_SET(fd, &evts->writefds);
				count++;
			}
			if (fp->errorevt_w != fp->errorevt_r) {
				fp->errorevt_r++;
				FD_SET(fd, &evts->errorfds);
				count++;
			}
		}

		if (count > 0)
			break;

		get_tim(&now);

		SYSTIM elapse = now - prev;
		if (elapse > tmout)
			return E_TMOUT;

		prev = now;
		tmout -= elapse;
	}

	evts->count = count;

	return E_OK;
}

void clean_fd()
{
	struct _IO_FILE *fp = NULL;
	for (int fd = 3; fd < fd_table_count; fd++) {
		fp = &fd_table[fd];
		if ((fp->type == 0) || (fp->fd == 0))
			continue;

		fp->close(fp);

		delete_fp(fp);
	}
}

int shell_ioctl(int fd, int request, void *arg)
{
	struct _IO_FILE *fp = fd_to_fp(fd);
	if (fp == NULL)
		return -EBADF;

	return fp->ioctl(fp, request, arg);
}

#ifdef NTSHELL_NO_SOCKET

int shell_socket(int family, int type, int protocol)
{
	return -ENOMEM;
}

int shell_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
	return -ENOMEM;
}

int shell_listen(int fd, int backlog)
{
	return -ENOMEM;
}

int shell_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	return -ENOMEM;
}

int shell_accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict len)
{
	return -ENOMEM;
}

ssize_t shell_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t alen)
{
	return -ENOMEM;
}

ssize_t shell_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	return -ENOMEM;
}

ssize_t shell_recvfrom(int fd, void *__restrict buf, size_t len, int flags, struct sockaddr *__restrict addr, socklen_t *__restrict alen)
{
	return -ENOMEM;
}

ssize_t shell_recvmsg(int fd, struct msghdr *msg, int flags)
{
	return -ENOMEM;
}

int shell_shutdown(int fd, int how)
{
	return -ENOMEM;
}

int shell_getsockopt(int fd, int level, int optname, void *optval, socklen_t *__restrict optlen)
{
	return -ENOMEM;
}

int shell_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	return -ENOMEM;
}

int shell_getpeername(int fd, struct sockaddr *restrict addr, socklen_t *restrict len)
{
	return -ENOMEM;
}

int shell_getsockname(int fd, struct sockaddr *restrict addr, socklen_t *restrict len)
{
	return -ENOMEM;
}
#endif
