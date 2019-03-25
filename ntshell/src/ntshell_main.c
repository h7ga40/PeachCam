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
 *  @(#) $Id: ntshell_main.c 1816 2019-02-17 12:41:05Z coas-nagasima $
 */

/* 
 *  サンプルプログラム(1)の本体
 */

#include "shellif.h"
#include <kernel.h>
#include <t_stdlib.h>
#include <sil.h>
#include <setjmp.h>
#include <string.h>
#include "syssvc/serial.h"
#include "syssvc/syslog.h"
#include "target_syssvc.h"
#include "kernel_cfg.h"
#include "ffarch.h"
#include "ff.h"
#include "core/ntshell.h"
#include "core/ntlibc.h"
#include "util/ntstdio.h"
#include "usrcmd.h"
#include "util/ntopt.h"
#include "ntshell_main.h"
#include "socket_stub.h"

char command[NTOPT_TEXT_MAXLEN];
ntstdio_t ntstdio;

extern uint8_t mac_addr[6];
const struct utsname host_name = {
	"TOPPERS/ASP3",
	TARGET_NAME,
	"3.2.0",
	"3.2.0",
	TARGET_NAME,
	"toppers.jp"
};

int shell_uname(struct utsname *uts)
{
	memcpy(uts, &host_name, sizeof(host_name));
	return 0;
}

static int usrcmd_ntopt_callback(long *args, void *extobj);

int ntshell_exit_code;
volatile int ntshell_state;
jmp_buf process_exit;
NTSHELL_SERIAL_READ ntshell_serial_read = 0;
NTSHELL_SERIAL_WRITE ntshell_serial_write = 0;
void *ntshell_serial_extobj;

unsigned char ntstdio_xi(struct ntstdio_t *handle)
{
	unsigned char buf[1];
	ntshell_serial_read((char *)buf, 1, ntshell_serial_extobj);
	return buf[0];
}

void ntstdio_xo(struct ntstdio_t *handle, unsigned char c)
{
	char buf[1];
	buf[0] = c;
	ntshell_serial_write(buf, 1, ntshell_serial_extobj);
}

void ntshell_task_init(NTSHELL_SERIAL_READ func_read,
	NTSHELL_SERIAL_WRITE func_write, void *extobj)
{
	ntshell_serial_read = func_read;
	ntshell_serial_write = func_write;
	ntshell_serial_extobj = extobj;

	ntstdio_init(&ntstdio, NTSTDIO_OPTION_LINE_ECHO | NTSTDIO_OPTION_CANON | NTSTDIO_OPTION_LF_CRLF | NTSTDIO_OPTION_LF_CR, ntstdio_xi, ntstdio_xo);
}

/*
 *  ntshellタスク
 */
void ntshell_task(intptr_t exinf)
{
	ntshell_state = 1;
	ntshell_exit_code = ntopt_parse(command, usrcmd_ntopt_callback, NULL);
	ntshell_state = 2;
}

void ntshell_change_netif_link(uint8_t link_up, uint8_t up)
{
	FLGPTN flgptn;
	T_RTSK rtsk;
	ER ret;

	ret = ref_tsk(NTSHELL_TASK, &rtsk);
	if ((ret != E_OK) || (rtsk.tskstat == TTS_DMT))
		return;

	FD_SET(0, (fd_set *)&flgptn);

	set_flg(FLG_SELECT_WAIT, flgptn);
}

static int usrcmd_ntopt_callback(long *args, void *extobj)
{
	const cmd_table_t *p = cmd_table_info.table;
	int result = 0;
	int found = 0;

	if (*args == 0)
		return result;

	if (strcmp((const char *)args[1], "help") == 0) {
		found = 1;
		result = usrcmd_help(args[0], (char **)&args[1]);
	}
	else for (int i = 0; i < cmd_table_info.count; i++) {
		if (strcmp((const char *)args[1], p->cmd) == 0) {
			found = 1;
			result = p->func(args[0], (char **)&args[1]);
			break;
		}
		p++;
	}

	if ((found == 0) && (((const char *)args[1])[0] != '\0'))
		printf("Unknown command found.\n");

	clean_fd();

	return result;
}

int usrcmd_help(int argc, char **argv)
{
	const cmd_table_t *p = cmd_table_info.table;
	for (int i = 0; i < cmd_table_info.count; i++) {
		ntstdio_puts(&ntstdio, p->cmd);
		ntstdio_puts(&ntstdio, "\t:");
		ntstdio_puts(&ntstdio, p->desc);
		ntstdio_puts(&ntstdio, "\n");
		p++;
	}
	return 0;
}

void shell_abort()
{
	ntshell_exit_code = -1;
	longjmp(process_exit, 1);
}

void shell_exit(int exitcd)
{
	ntshell_exit_code = exitcd;
	longjmp(process_exit, 1);
}

void shell_exit_group(int exitcd)
{
	ntshell_exit_code = exitcd;
	longjmp(process_exit, 1);
}

int execute_command(int wait)
{
	ER ret;

	ret = ter_tsk(NTSHELL_TASK);
	if ((ret != E_OK) && (ret != E_OBJ)) {
		syslog(LOG_ERROR, "ter_tsk => %d", ret);
	}

	tslp_tsk(100000);

	clean_fd();

	ntshell_state = 0;
	ret = act_tsk(NTSHELL_TASK);
	if (ret != E_OK) {
		syslog(LOG_ERROR, "act_tsk => %d", ret);
	}

	if (wait == 0)
		return 0;

	do {
		tslp_tsk(100000);
	} while(ntshell_state == 1);

	return ntshell_exit_code;
}

int cmd_execute(const char *text, void *extobj)
{
	ntlibc_strlcpy(command, text, sizeof(command));
	return execute_command(1);
}

int stdio_close(struct _IO_FILE *fp)
{
	return -EPERM;
}

size_t stdio_read(struct _IO_FILE *fp, unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t stdio_write(struct _IO_FILE *fp, const unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t stdin_read(struct _IO_FILE *fp, unsigned char *data, size_t len)
{
	int i = 0;
	while (i < len) {
		int c = ntstdio_getc(&ntstdio);
		data[i++] = c;
		if ((c == EOF) || (c == '\n'))
			break;
	}
	return i;
}

size_t stdout_write(struct _IO_FILE *fp, const unsigned char *data, size_t len)
{
	for (int i = 0; i < len; i++) {
		ntstdio_putc(&ntstdio, data[i]);
	}
	return len;
}

size_t stderr_write(struct _IO_FILE *fp, const unsigned char *data, size_t len)
{
	for (int i = 0; i < len; i++) {
		ntstdio_putc(&ntstdio, data[i]);
	}
	return len;
}

int sio_close(struct _IO_FILE *fp)
{
	return -EPERM;
}

size_t sio_read(struct _IO_FILE *fp, unsigned char *data, size_t len)
{
	return -EPERM;
}

size_t sio_write(struct _IO_FILE *fp, const unsigned char *data, size_t len)
{
	return -EPERM;
}

off_t sio_seek(struct _IO_FILE *fp, off_t ofs, int org)
{
	return -EPERM;
}

int sio_ioctl(struct _IO_FILE *fp, int request, void *arg)
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

int shell_clock_getres(clockid_t clk_id, struct timespec *res)
{
	if ((clk_id != CLOCK_REALTIME) && (clk_id != CLOCK_MONOTONIC))
		return -EINVAL;

	memset(&res->tv_sec, 0xFF, sizeof(res->tv_sec));
	res->tv_nsec = 0;

	return 0;
}

int shell_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	SYSTIM now = 0;

	if ((clk_id != CLOCK_REALTIME) && (clk_id != CLOCK_MONOTONIC))
		return -EINVAL;

	get_tim(&now);
	tp->tv_sec = now / 1000000;
	tp->tv_nsec = (now % 1000000) * 1000;

	return 0;
}

int shell_clock_settime(clockid_t clk_id, const struct timespec *tp)
{
	if ((clk_id != CLOCK_REALTIME) && (clk_id != CLOCK_MONOTONIC))
		return -EINVAL;

	SYSTIM time;
	ER ret;

	time = (tp->tv_sec * 1000000ll) + (tp->tv_nsec / 1000ll);

	ret = set_tim(time);
	if (ret != E_OK) {
		return -EPERM;
	}

	return 0;
}

sigset_t g_sigmask;

int shell_sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict old)
{
	if (old != NULL)
		memcpy(old, &g_sigmask, sizeof(sigset_t));

	switch (how) {
	case SIG_BLOCK:
		for (int i = 0; i < sizeof(g_sigmask.__bits) / sizeof(g_sigmask.__bits[0]); i++) {
			g_sigmask.__bits[i] |= set->__bits[i];
		}
		break;
	case SIG_UNBLOCK:
		for (int i = 0; i < sizeof(g_sigmask.__bits) / sizeof(g_sigmask.__bits[0]); i++) {
			g_sigmask.__bits[i] &= ~set->__bits[i];
		}
		break;
	case SIG_SETMASK:
		memcpy(&g_sigmask, set, sizeof(sigset_t));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

struct sigaction sigtable[7];

int shell_sigaction(int sig, const struct sigaction *restrict sa, struct sigaction *restrict old)
{
	struct sigaction *sat;

	switch(sig){
	case SIGALRM:
		sat = &sigtable[0];
		break;
	case SIGFPE:
		sat = &sigtable[1];
		break;
	case SIGILL:
		sat = &sigtable[2];
		break;
	case SIGSEGV:
		sat = &sigtable[3];
		break;
	case SIGBUS:
		sat = &sigtable[4];
		break;
	case SIGABRT:
		sat = &sigtable[5];
		break;
	case SIGPIPE:
		sat = &sigtable[6];
		break;
	default:
		return -EINVAL;
	}

	if (old != NULL)
		memcpy(old, sat, sizeof(struct sigaction));

	memcpy(sat, sa, sizeof(struct sigaction));

	return 0;
}

int shell_madvise(void *a, size_t b, int c)
{
	return 0;
}

int shell_gettid()
{
	ID tskid;
	ER ret;

	ret = get_tid(&tskid);
	if (ret != E_OK)
		return -1;

	return tskid;
}

int shell_tkill(int tid, int sig)
{
	if ((tid == NTSHELL_TASK) && (sig == SIGABRT)) {
		shell_abort();
	}

	no_implement("tkill");
	return -1;
}

