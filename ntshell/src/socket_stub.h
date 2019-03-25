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
 *  @(#) $Id: socket_stub.h 1806 2019-02-13 16:34:49Z coas-nagasima $
 */
#ifndef SOCKET_STUB_H
#define SOCKET_STUB_H

struct addrinfo {
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	socklen_t ai_addrlen;
	struct sockaddr *ai_addr;
	char *ai_canonname;
	struct addrinfo *ai_next;
};

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;
struct in_addr { in_addr_t s_addr; };

struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	uint8_t sin_zero[8];
};

struct in6_addr
{
	union {
		uint8_t __s6_addr[16];
		uint16_t __s6_addr16[8];
		uint32_t __s6_addr32[4];
	} __in6_union;
};
//#define s6_addr __in6_union.__s6_addr
//#define s6_addr16 __in6_union.__s6_addr16
//#define s6_addr32 __in6_union.__s6_addr32

struct sockaddr_in6
{
	sa_family_t     sin6_family;
	in_port_t       sin6_port;
	uint32_t        sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t        sin6_scope_id;
};

typedef struct socket_t {
	int family;
	int type;
	int protocol;
	int cepid;
	int repid;
	int backlog;
	unsigned int flags;
	union {
		struct sockaddr_in laddr4;
		struct sockaddr_in6 laddr6;
	};
	union {
		struct sockaddr_in raddr4;
		struct sockaddr_in6 raddr6;
	};
	int buf_size;
	unsigned char *buf;
	void *input;
	int len;
} socket_t;

struct _IO_DIR {
	FATFS_DIR dir;
	struct dirent dirent;
};

struct _IO_FILE {
	int fd;
	int type;
	int handle;
	int (*close)(struct _IO_FILE *);
	size_t (*read)(struct _IO_FILE *, unsigned char *, size_t);
	size_t (*write)(struct _IO_FILE *, const unsigned char *, size_t);
	off_t (*seek)(struct _IO_FILE *, off_t, int);
	int (*ioctl)(struct _IO_FILE *, int, void *);
	int readevt_r;
	int readevt_w;
	int writeevt_r;
	int writeevt_w;
	int writable;
	int errorevt_r;
	int errorevt_w;
	union {
		FIL *pfile;
		struct _IO_DIR *pdir;
		socket_t *psock;
	};
};

#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

extern int stdio_close(struct _IO_FILE *fp);
extern size_t stdio_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t stdio_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern size_t stdin_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t stdout_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern size_t stderr_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);

extern int sio_close(struct _IO_FILE *fp);
extern size_t sio_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t sio_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern off_t sio_seek(struct _IO_FILE *fp, off_t ofs, int org);
extern int sio_ioctl(struct _IO_FILE *fp, int req, void *arg);

extern int file_close(struct _IO_FILE *fp);
extern size_t file_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t file_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern off_t file_seek(struct _IO_FILE *fp, off_t ofs, int org);
extern int file_ioctl(struct _IO_FILE *fp, int req, void *arg);

extern int dir_close(struct _IO_FILE *fp);
extern size_t dir_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t dir_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern off_t dir_seek(struct _IO_FILE *fp, off_t ofs, int org);
extern int dir_ioctl(struct _IO_FILE *fp, int req, void *arg);

extern int tcp_fd_close(struct _IO_FILE *fp);
extern size_t tcp_fd_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t tcp_fd_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern off_t tcp_fd_seek(struct _IO_FILE *fp, off_t ofs, int org);
extern int tcp_fd_ioctl(struct _IO_FILE *fp, int req, void *arg);

extern int udp_fd_close(struct _IO_FILE *fp);
extern size_t udp_fd_read(struct _IO_FILE *fp, unsigned char *data, size_t len);
extern size_t udp_fd_write(struct _IO_FILE *fp, const unsigned char *data, size_t len);
extern off_t udp_fd_seek(struct _IO_FILE *fp, off_t ofs, int org);
extern int udp_fd_ioctl(struct _IO_FILE *fp, int req, void *arg);

struct _IO_FILE *fd_to_fp(int fd);

struct _IO_FILE *new_sio_fd(int sioid);
int delete_sio_fd(int sioid);
struct _IO_FILE *sioid_to_fd(int sioid);

struct _IO_FILE *new_file_fd(int fileid);
int delete_file_fd(int fileid);
struct _IO_FILE *fileid_to_fd(int fileid);

struct _IO_FILE *new_dir_fd(int dirid);
int delete_dir_fd(int dirid);
struct _IO_FILE *dirid_to_fd(int dirid);

struct _IO_FILE *new_tcp_fd(int tcpid);
int delete_tcp_fd(int tcpid);
struct _IO_FILE *tcpid_to_fd(int tcpid);

struct _IO_FILE *new_udp_fd(int udpid);
int delete_udp_fd(int udpid);
struct _IO_FILE *udpid_to_fd(int udpid);

int delete_fp(struct _IO_FILE *fp);
int delete_tcp_rep(int repid);
void clean_fd();

ER socket_tcp_callback(ID cepid, FN fncd, void *p_parblk);
ER socket_udp_callback(ID cepid, FN fncd, void *p_parblk);

#endif // !SOCKET_STUB_H
