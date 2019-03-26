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
 *  @(#) $Id: socket_stub.c 1781 2019-02-01 00:02:42Z coas-nagasima $
 */
#include "shellif.h"
#include <kernel.h>
#include <t_syslog.h>
#include <t_stdlib.h>
#include <string.h>
#include <sil.h>
#include "syssvc/syslog.h"
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
#include <netinet/udp_var.h>
#include <net/net_buf.h>
//#include <netinet/tcp_var.h>
#include <netapp/resolver.h>
extern const ID tmax_tcp_cepid;
#include "ff.h"
#include "socket_stub.h"
#include "kernel_cfg.h"

#define SOCKET_TIMEOUT 2000000

typedef struct id_table_t {
	int used;
	ID id;
} id_table_t;

id_table_t tcp_repid_table[] = {
	{0, USR_TCP_REP1}, {0, USR_TCP_REP2}, {0, USR_TCP_REP3}, {0, USR_TCP_REP4}
};
#define tcp_repid_table_count (sizeof(tcp_repid_table) / sizeof(tcp_repid_table[0]))

id_table_t tcp_cepid_table[] = {
	{0, USR_TCP_CEP1}, {0, USR_TCP_CEP2}, {0, USR_TCP_CEP3}, {0, USR_TCP_CEP4},
#ifndef TOPPERS_GRSAKURA
	{0, USR_TCP_CEP5}, {0, USR_TCP_CEP6}, {0, USR_TCP_CEP7}, {0, USR_TCP_CEP8}
#endif
};
#define tcp_cepid_table_count (sizeof(tcp_cepid_table) / sizeof(tcp_cepid_table[0]))

id_table_t udp_cepid_table[] = {
	{0, USR_UDP_CEP1}, {0, USR_UDP_CEP2}, {0, USR_UDP_CEP3}, {0, USR_UDP_CEP4}
};
#define udp_cepid_table_count (sizeof(udp_cepid_table) / sizeof(udp_cepid_table[0]))

ID new_id(id_table_t *table, int count)
{
	for (int i = 0; i < count; i++) {
		id_table_t *item = &table[i];
		if (item->used != 0)
			continue;

		item->used = 1;
		return item->id;
}

	return -ENOMEM;
}

int delete_id(id_table_t *table, int count, ID id)
{
	for (int i = 0; i < count; i++) {
		id_table_t *item = &table[i];
		if ((item->used == 0) || (item->id != id))
			continue;

		item->used = 0;
		return 0;
	}
	return -EINVAL;
}

int delete_tcp_rep(int repid)
{
	return delete_tcp_fd(tmax_tcp_cepid + repid);
}

typedef struct _IO_FILE SOCKET;

int shell_socket(int family, int type, int protocol)
{
	SOCKET *fp;
	unsigned int flags;

	switch (family) {
	case AF_INET:
	case AF_INET6:
		break;
	default:
		return -EAFNOSUPPORT;
	}

	flags = type & (SOCK_CLOEXEC|SOCK_NONBLOCK);
	type &= ~flags;

	switch (type) {
	case SOCK_STREAM:
		fp = new_tcp_fd(0);
		break;
	case SOCK_DGRAM:
		fp = new_udp_fd(0);
		break;
	default:
		return -ENOPROTOOPT;
	}

	if (fp == NULL) {
		return -ENOMEM;
	}

	fp->psock->family = family;
	fp->psock->type = type;
	fp->psock->protocol = protocol;
	fp->psock->flags = flags;

	return fp->fd;
}

int shell_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL)
		return -EBADF;
	if (fp->psock->family != addr->sa_family)
		return -EINVAL;

	ER ret;
	switch (addr->sa_family) {
	case AF_INET: {
		if (len < 8) {
			return -EINVAL;
		}
		struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
		memcpy(&fp->psock->laddr4, addr, len);
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			ID cepid = new_id(tcp_cepid_table, tcp_cepid_table_count);
			if (cepid < 0)
				return -ENOMEM;

			fp->psock->buf_size = 512 + 512;
			fp->psock->buf = malloc(fp->psock->buf_size);
#ifdef _DEBUG
			memset(fp->psock->buf, 0, fp->psock->buf_size);
#endif
			T_TCP_CCEP ccep = { 0, fp->psock->buf, 512, &fp->psock->buf[512], 512, (FP)socket_tcp_callback };
			ret = tcp_cre_cep(cepid, &ccep);
			if (ret != E_OK) {
				delete_id(tcp_cepid_table, tcp_cepid_table_count, cepid);
				return -ENOMEM;
			}
			fp->handle = cepid;
			fp->psock->cepid = cepid;
			break;
		}
		case SOCK_DGRAM: {
			ID cepid = new_id(udp_cepid_table, udp_cepid_table_count);
			if (cepid < 0)
				return -ENOMEM;

			T_UDP_CCEP ccep = { 0, {ntohl(addr_in->sin_addr.s_addr), ntohs(addr_in->sin_port)}, (FP)socket_udp_callback };
			ret = udp_cre_cep(cepid, &ccep);
			if (ret != E_OK) {
				delete_id(udp_cepid_table, udp_cepid_table_count, cepid);
				return -ENOMEM;
			}
			fp->handle = cepid;
			fp->psock->cepid = cepid;
			break;
		}
		default:
			return -ENOPROTOOPT;
		}
		break;
	}
	case AF_INET6: {
		if (len < 20) {
			return -EINVAL;
		}
		memcpy(&fp->psock->laddr4, addr, len);
		break;
	}
	}

	return 0;
}

int shell_listen(int fd, int backlog)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL)
		return -EBADF;
	if (fp->psock->type != SOCK_STREAM)
		return -EINVAL;

	fp->psock->backlog = backlog;

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		ID repid = new_id(tcp_repid_table, tcp_repid_table_count);
		if (repid < 0)
			return -ENOMEM;

		struct sockaddr_in *laddr = &fp->psock->laddr4;
		T_TCP_CREP crep = { 0, {ntohl(laddr->sin_addr.s_addr), ntohs(laddr->sin_port)} };
		ret = tcp_cre_rep(repid, &crep);
		if (ret != E_OK) {
			delete_id(tcp_repid_table, tcp_repid_table_count, repid);
			return -ENOMEM;
		}
		fp->psock->repid = repid;
		break;
	}
	case AF_INET6: {
		break;
	}
	}

	return 0;
}

int shell_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL)
		return -EBADF;
	if (fp->psock->type != SOCK_STREAM)
		return -EINVAL;

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		if (len < 8) {
			return -EINVAL;
		}
		if (fp->psock->cepid == 0) {
			ID cepid = new_id(tcp_cepid_table, tcp_cepid_table_count);
			if (cepid < 0)
				return -ENOMEM;

			fp->psock->buf_size = 512 + 512;
			fp->psock->buf = malloc(fp->psock->buf_size);
#ifdef _DEBUG
			memset(fp->psock->buf, 0, fp->psock->buf_size);
#endif
			T_TCP_CCEP ccep = { 0, fp->psock->buf, 512, &fp->psock->buf[512], 512, (FP)socket_tcp_callback };
			ret = tcp_cre_cep(cepid, &ccep);
			if (ret != E_OK) {
				delete_id(tcp_cepid_table, tcp_cepid_table_count, cepid);
				return -ENOMEM;
			}
			fp->handle = cepid;
			fp->psock->cepid = cepid;
		}
		struct sockaddr_in *laddr = &fp->psock->laddr4;
		struct sockaddr_in *raddr = &fp->psock->raddr4;
		memset(raddr, 0, sizeof(*raddr));
		memcpy(raddr, addr, len);
		T_IPV4EP lep = { ntohl(laddr->sin_addr.s_addr), ntohs(laddr->sin_port) };
		T_IPV4EP rep = { ntohl(raddr->sin_addr.s_addr), ntohs(raddr->sin_port) };
		ret = tcp_con_cep(fp->psock->cepid, &lep, &rep, SOCKET_TIMEOUT);
		if (ret < 0) {
			return -EHOSTUNREACH;
		}
		break;
	}
	case AF_INET6: {
		break;
	}
	}

	return 0;
}

int shell_accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict len)
{
	SOCKET *lfp = fd_to_fp(fd);
	if (lfp == NULL)
		return -EBADF;
	if (lfp->psock->type != SOCK_STREAM)
		return -EINVAL;

	SOCKET *fp = new_tcp_fd(0);
	if (fp == NULL) {
		return -ENOMEM;
	}

	memcpy(fp->psock, lfp->psock, offsetof(socket_t, buf_size));

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		ID cepid;
		if (fp->psock->cepid == 0) {
			cepid = new_id(tcp_cepid_table, tcp_cepid_table_count);
			if (cepid < 0)
				return -ENOMEM;

			fp->psock->buf_size = 512 + 512;
			fp->psock->buf = malloc(fp->psock->buf_size);
#ifdef _DEBUG
			memset(fp->psock->buf, 0, fp->psock->buf_size);
#endif
			T_TCP_CCEP ccep = { 0, fp->psock->buf, 512, &fp->psock->buf[512], 512, (FP)socket_tcp_callback };
			ret = tcp_cre_cep(cepid, &ccep);
			if (ret != E_OK) {
				delete_id(tcp_cepid_table, tcp_cepid_table_count, cepid);
				return -ENOMEM;
			}
			fp->handle = cepid;
			fp->psock->cepid = cepid;
		}
		else {
			cepid = lfp->psock->cepid;
			fp->handle = cepid;
			lfp->handle = tmax_tcp_cepid + lfp->psock->repid;
			lfp->psock->cepid = 0;
			lfp->psock->buf_size = 0;
			lfp->psock->buf = 0;
		}
		T_IPV4EP rep = { 0, 0 };
		ret = tcp_acp_cep(fp->psock->cepid, fp->psock->repid, &rep, TMO_FEVR);
		if (ret < 0) {
			return -ENOMEM;
		}
		struct sockaddr_in *raddr = &fp->psock->raddr4;
		memset(raddr, 0, sizeof(*raddr));
		raddr->sin_family = AF_INET;
		raddr->sin_port = htons(rep.portno);
		raddr->sin_addr.s_addr = htonl(rep.ipaddr);
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	if (addr != NULL && len != NULL) {
		int sz = *len;
		if (sz < 8) {
			return -EINVAL;
		}
		struct sockaddr_in *raddr = &fp->psock->raddr4;
		if (sz > sizeof(*raddr))
			sz = sizeof(*raddr);
		memcpy(addr, raddr, sz);
		*len = sizeof(*raddr);
	}

	return fp->fd;
}

ssize_t shell_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t alen)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}

	int ret = 0;
	switch (fp->psock->family) {
	case AF_INET: {
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			if ((addr != NULL) || (alen != 0)) {
				return -EISCONN;
			}

			if (flags & MSG_OOB) {
				ret = tcp_snd_oob(fp->psock->cepid, (void *)buf, len, SOCKET_TIMEOUT);
				if (ret < 0) {
					return -ECOMM;
				}
			}
			else {
				for (;;) {
					ret = tcp_snd_dat(fp->psock->cepid, (void *)buf, len, SOCKET_TIMEOUT);
					if (ret < 0) {
						if (ret == E_TMOUT)
							return -ETIME;
						return -ECOMM;
					}
					len -= ret;
					if (len <= 0)
						break;
					buf = (const void *)&((uint8_t *)buf)[ret];
				} 
			}
			break;
		}
		case SOCK_DGRAM: {
			int sz = alen;
			if ((addr == NULL) || (sz < 8)) {
				return -EINVAL;
			}
			struct sockaddr_in *raddr = &fp->psock->raddr4;
			memset(raddr, 0, sizeof(*raddr));
			memcpy(raddr, addr, sz);
			T_IPV4EP rep = { ntohl(raddr->sin_addr.s_addr), ntohs(raddr->sin_port) };
			ret = udp_snd_dat(fp->psock->cepid, &rep, (void *)buf, len,
				(fp->psock->flags & O_NONBLOCK) ? TMO_POL : SOCKET_TIMEOUT);
			if (ret < 0) {
				return (ret == E_TMOUT) ? -ETIME : -ECOMM;
			}
			break;
		}
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return ret;
}

ssize_t shell_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	no_implement("sendmsg\n");
	return -ENOSYS;
}

ssize_t shell_recvfrom(int fd, void *__restrict buf, size_t len, int flags, struct sockaddr *__restrict addr, socklen_t *__restrict alen)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}

	int ret = 0;
	switch (fp->psock->family) {
	case AF_INET: {
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			if (flags & MSG_OOB) {
				ret = tcp_rcv_oob(fp->psock->cepid, buf, len);
				if (ret < 0) {
					syslog(LOG_ERROR, "tcp_rcv_oob => %d", ret);
					return -ECOMM;
				}
			}
			else {
				int rsz, tmp;
				if (fp->psock->input == NULL) {
					ret = wai_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "wai_sem => %d", ret);
					}
					fp->psock->len = 0;
					ret = sig_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "sig_sem => %d", ret);
					}
					ret = tcp_rcv_buf(fp->psock->cepid, &fp->psock->input, TMO_FEVR);
					if (ret < 0) {
						syslog(LOG_ERROR, "tcp_rcv_buf => %d", ret);
						return -ECOMM;
					}
					rsz = ret;
				}
				else
					rsz = fp->psock->len;
				tmp = rsz;
				if (rsz > len)
					rsz = len;
				if (rsz >= 0) {
					memcpy(buf, fp->psock->input, rsz);
					ret = wai_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "wai_sem => %d", ret);
					}
					fp->psock->len = tmp - rsz;
					ret = sig_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "sig_sem => %d", ret);
					}
					if (tmp - rsz == 0) {
						fp->psock->input = NULL;
					}
					else
						fp->psock->input = (void *)&((uint8_t *)fp->psock->input)[rsz];
					ret = tcp_rel_buf(fp->psock->cepid, rsz);
					if ((ret != E_OBJ) && (ret < 0)) {
						syslog(LOG_ERROR, "tcp_rel_buf => %d", ret);
						//return -ECOMM;
					}
				}
				ret = rsz;
			}
			break;
		}
		case SOCK_DGRAM: {
			struct sockaddr_in *raddr = &fp->psock->raddr4;
			int rsz;
			ret = wai_sem(SEM_FILEDESC);
			if (ret < 0) {
				syslog(LOG_ERROR, "wai_sem => %d", ret);
			}
			T_NET_BUF *input = fp->psock->input;
			if (input == NULL) {
				ret = sig_sem(SEM_FILEDESC);
				if (ret < 0) {
					syslog(LOG_ERROR, "sig_sem => %d", ret);
				}

				T_IPV4EP rep = { 0, 0 };
				ret = udp_rcv_dat(fp->psock->cepid, &rep, buf, len,
					(fp->psock->flags & O_NONBLOCK) ? TMO_POL : SOCKET_TIMEOUT);
				if (ret < 0) {
					if ((fp->psock->flags & O_NONBLOCK) == 0)
						syslog(LOG_ERROR, "udp_rcv_buf => %d", ret);
					return (ret == E_TMOUT) ? -ETIME : -ECOMM;
				}
				rsz = ret;
				if ((addr != NULL) && (alen != NULL)) {
					ret = wai_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "wai_sem => %d", ret);
					}
					int sz = *alen;
					memset(raddr, 0, sizeof(fp->psock->raddr4));
					raddr->sin_family = AF_INET;
					raddr->sin_port = htons(rep.portno);
					raddr->sin_addr.s_addr = htonl(rep.ipaddr);
					if (sz > sizeof(fp->psock->raddr4))
						sz = sizeof(fp->psock->raddr4);
					memcpy(addr, raddr, sz);
					*alen = sz;
					ret = sig_sem(SEM_FILEDESC);
					if (ret < 0) {
						syslog(LOG_ERROR, "sig_sem => %d", ret);
					}
				}
			}
			else {
				rsz = fp->psock->len;
				void *pbuf = fp->psock->buf;
				fp->psock->input = NULL;
				fp->psock->len = 0;
				fp->psock->buf = NULL;
				if ((addr != NULL) && (alen != NULL)) {
					int sz = *alen;
					if (sz > sizeof(fp->psock->raddr4))
						sz = sizeof(fp->psock->raddr4);
					memcpy(addr, raddr, sz);
					*alen = sz;
				}
				ret = sig_sem(SEM_FILEDESC);
				if (ret < 0) {
					syslog(LOG_ERROR, "sig_sem => %d", ret);
				}
				if (rsz > len)
					rsz = len;
				memcpy(buf, pbuf, rsz);
				ret = rel_net_buf(input);
				if (ret < 0) {
					syslog(LOG_ERROR, "rel_net_buf => %d", ret);
					//return -ECOMM;
				}
			}
			ret = rsz;
		}
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return ret;
}

ssize_t shell_recvmsg(int fd, struct msghdr *msg, int flags)
{
	no_implement("recvmsg\n");
	return -ENOSYS;
}

int shell_shutdown(int fd, int how)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			ret = tcp_sht_cep(fp->psock->cepid);
			if (ret < 0) {
				return -ECOMM;
			}
			break;
		}
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

int shell_getsockopt(int fd, int level, int optname, void *optval, socklen_t *__restrict optlen)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			switch (level) {
			case SOL_SOCKET:
				switch (optname) {
				case SO_REUSEADDR:
					if (fp->psock->flags & SO_REUSEADDR) {
						*(bool *)optval = true;
					}
					else {
						*(bool *)optval = false;
					}
					break;
				case SO_KEEPALIVE:
					if (fp->psock->flags & SO_KEEPALIVE) {
						*(bool *)optval = true;
					}
					else {
						*(bool *)optval = false;
					}
					break;
				case SO_ERROR:
					*(int *)optval = 0;
					break;
				default:
					return -EINVAL;
				}
				break;
			case IPPROTO_TCP:
				ret = tcp_get_opt(fp->psock->cepid, optname, (void *)optval, *optlen);
				if (ret < 0) {
					return -EINVAL;
				}
				*optlen = ret;
				break;
			default:
				return -EINVAL;
			}
			break;
		}
		case SOCK_DGRAM: {
			switch (level) {
			case IPPROTO_UDP:
				ret = udp_get_opt(fp->psock->cepid, optname, (void *)optval, *optlen);
				if (ret < 0) {
					return -EINVAL;
				}
				*optlen = ret;
				break;
			default:
				return -EINVAL;
			}
			break;
		}
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

int shell_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}

	ER ret;
	switch (fp->psock->family) {
	case AF_INET: {
		switch (fp->psock->type) {
		case SOCK_STREAM: {
			switch (level){
			case SOL_SOCKET:
				switch (optname) {
				case SO_REUSEADDR:
					if (*(bool *)optval) {
						fp->psock->flags |= SO_REUSEADDR;
					}
					else {
						fp->psock->flags &= ~SO_REUSEADDR;
					}
					break;
				case SO_KEEPALIVE:
					if (*(bool *)optval) {
						fp->psock->flags |= SO_KEEPALIVE;
					}
					else {
						fp->psock->flags &= ~SO_KEEPALIVE;
					}
					break;
				default:
					return -EINVAL;
				}
				break;
			case IPPROTO_TCP:
				ret = tcp_set_opt(fp->psock->cepid, optname, (void *)optval, optlen);
				if (ret < 0) {
					return -EINVAL;
				}
				break;
			default:
				return -EINVAL;
			}
			break;
		}
		case SOCK_DGRAM: {
			switch (level){
			case IPPROTO_UDP:
				ret = udp_set_opt(fp->psock->cepid, optname, (void *)optval, optlen);
				if (ret < 0) {
					return -EINVAL;
				}
				break;
			default:
				return -EINVAL;
			}
			break;
		}
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

int shell_getpeername(int fd, struct sockaddr *restrict addr, socklen_t *restrict len)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}
	if (len == NULL) {
		return -EINVAL;
	}

	socklen_t size = *len;
	switch (fp->psock->family) {
	case AF_INET: {
		struct sockaddr_in *raddr = &fp->psock->raddr4;
		*len = sizeof(struct sockaddr_in);
		if (size > sizeof(struct sockaddr_in))
			size = sizeof(struct sockaddr_in);
		memcpy(addr, raddr, size);
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

int shell_getsockname(int fd, struct sockaddr *restrict addr, socklen_t *restrict len)
{
	SOCKET *fp = fd_to_fp(fd);
	if (fp == NULL) {
		return -EBADF;
	}
	if (len == NULL) {
		return -EINVAL;
	}

	socklen_t size = *len;
	switch (fp->psock->family) {
	case AF_INET: {
		const T_IN4_ADDR *laddr4 = in4_get_ifaddr(0);
		struct sockaddr_in laddr;
		laddr.sin_family = AF_INET;
		laddr.sin_addr.s_addr = htonl(*laddr4);
		laddr.sin_port = fp->psock->laddr4.sin_port;
		memset(&laddr.sin_zero, 0, sizeof(laddr.sin_zero));
		*len = sizeof(struct sockaddr_in);
		if (size > sizeof(struct sockaddr_in))
			size = sizeof(struct sockaddr_in);
		memcpy(addr, &laddr, size);
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

int tcp_fd_close(struct _IO_FILE *fp)
{
	ER ret, ret2;

	switch (fp->psock->family) {
	case AF_INET: {
		if (fp->psock->cepid != 0) {
			ID cepid = fp->psock->cepid;
			ret = tcp_sht_cep(cepid);
			if (ret < 0) {
				//return -1;
			}
			ret = tcp_cls_cep(cepid, (fp->psock->repid != 0) ? 0 : SOCKET_TIMEOUT);
			ret2 = tcp_del_cep(cepid);
			free(fp->psock->buf);
			fp->psock->buf = NULL;
			delete_tcp_fd(cepid);
			delete_id(tcp_cepid_table, tcp_cepid_table_count, cepid);
			if ((ret < 0) || (ret2 < 0)) {
				return (ret == E_TMOUT) ? -ETIME : -EINVAL;
			}
		}
		else if (fp->psock->repid != 0) {
			ID repid = fp->psock->repid;
			ret = tcp_del_rep(repid);
			free(fp->psock->buf);
			fp->psock->buf = NULL;
			delete_tcp_fd(tmax_tcp_cepid + repid);
			delete_id(tcp_repid_table, tcp_repid_table_count, repid);
			if (ret < 0) {
				return -EINVAL;
			}
		}
		else {
			free(fp->psock->buf);
			fp->psock->buf = NULL;
			return -EINVAL;
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

size_t tcp_fd_read(struct _IO_FILE *fp, unsigned char *dst, size_t dstsz)
{
	return shell_recvfrom(fp->fd, dst, dstsz, 0, NULL, NULL);
}

size_t tcp_fd_write(struct _IO_FILE *fp, const unsigned char *src, size_t srcsz)
{
	return shell_sendto(fp->fd, src, srcsz, 0, NULL, 0);
}

off_t tcp_fd_seek(struct _IO_FILE *fp, off_t ofs, int org)
{
	return -EPERM;
}

int tcp_fd_ioctl(struct _IO_FILE *fp, int req, void *arg)
{
	return -EINVAL;
}

int udp_fd_close(struct _IO_FILE *fp)
{
	ER ret;
	ID cepid;

	switch (fp->psock->family) {
	case AF_INET: {
		cepid = fp->psock->cepid;
		ret = udp_del_cep(cepid);
		//free(fp->psock->buf);
		//fp->psock->buf = NULL;
		delete_udp_fd(cepid);
		delete_id(udp_cepid_table, udp_cepid_table_count, cepid);
		if (ret < 0) {
			return -EINVAL;
		}
		break;
	}
	case AF_INET6: {
		return -EAFNOSUPPORT;
	}
	}

	return 0;
}

size_t udp_fd_read(struct _IO_FILE *fp, unsigned char *dst, size_t dstsz)
{
	return shell_recvfrom(fp->fd, dst, dstsz, 0, NULL, NULL);
}

size_t udp_fd_write(struct _IO_FILE *fp, const unsigned char *src, size_t srcsz)
{
	return shell_sendto(fp->fd, src, srcsz, 0, NULL, 0);
}

off_t udp_fd_seek(struct _IO_FILE *fp, off_t ofs, int org)
{
	return -EPERM;
}

int udp_fd_ioctl(struct _IO_FILE *fp, int req, void *arg)
{
	return -EINVAL;
}

#ifndef TCP_CFG_EXTENTIONS
ER tcp_cre_rep(ID repid, T_TCP_CREP *pk_crep)
{
	syslog(LOG_ERROR, "tcp_cre_rep not implement");
	shell_abort();
	return E_SYS;
}

ER tcp_cre_cep(ID cepid, T_TCP_CCEP *pk_ccep)
{
	syslog(LOG_ERROR, "tcp_cre_cep not implement");
	shell_abort();
	return E_SYS;
}
#endif

#ifndef UDP_CFG_EXTENTIONS
ER udp_cre_cep(ID cepid, T_UDP_CCEP *pk_ccep)
{
	syslog(LOG_ERROR, "udp_cre_cep not implement");
	shell_abort();
	return E_SYS;
}
#endif

#ifndef TCP_CFG_EXTENTIONS
ER_UINT tcp_snd_oob(ID cepid, void *data, int_t len, TMO tmout)
{
	syslog(LOG_ERROR, "tcp_snd_oob not implement");
	shell_abort();
	return E_SYS;
}

ER_UINT tcp_rcv_oob(ID cepid, void *data, int_t len)
{
	syslog(LOG_ERROR, "tcp_rcv_oob not implement");
	shell_abort();
	return E_SYS;
}

ER tcp_set_opt(ID cepid, int_t optname, void *optval, int_t optlen)
{
	syslog(LOG_ERROR, "tcp_set_opt not implement");
	shell_abort();
	return E_SYS;
}

ER tcp_get_opt(ID cepid, int_t optname, void *optval, int_t optlen)
{
	syslog(LOG_ERROR, "tcp_get_opt not implement");
	shell_abort();
	return E_SYS;
}
#endif

#ifndef UDP_CFG_EXTENTIONS
ER udp_get_opt(ID cepid, int_t optname, void *optval, int_t optlen)
{
	syslog(LOG_ERROR, "udp_get_opt not implement");
	shell_abort();
	return E_SYS;
}

ER udp_set_opt(ID cepid, int_t optname, void *optval, int_t optlen)
{
	syslog(LOG_ERROR, "udp_set_opt not implement");
	shell_abort();
	return E_SYS;
}
#endif

// musl-1.1.18/network/lookup.h
struct address {
	int family;
	unsigned scopeid;
	uint8_t addr[16];
	int sortkey;
};

#define MAXNS 3

struct resolvconf {
	struct address ns[MAXNS];
	unsigned nns, attempts, ndots;
	unsigned timeout;
};

// musl-1.1.18/network/resolvconf.c
int __get_resolv_conf(struct resolvconf *conf, char *search, size_t search_sz)
{
	int nns = 0;

	conf->ndots = 1;
	conf->timeout = 5;
	conf->attempts = 2;
	if (search) *search = 0;

#if defined(SUPPORT_INET4)
	T_IN4_ADDR	in4_addr;
	conf->ns[nns].family = AF_INET;
	conf->ns[nns].scopeid = 0;
	dns_in4_get_addr(&in4_addr);
	*(uint32_t *)conf->ns[nns].addr = ntohl(in4_addr);
	nns++;
#endif

#if defined(SUPPORT_INET6)
	conf->ns[nns].family = AF_INET6;
	conf->ns[nns].scopeid = 0;
	dns_in6_get_addr((T_IN6_ADDR *)conf->ns[nns].addr);
	nns++;
#endif
	conf->nns = nns;

	return 0;
}
