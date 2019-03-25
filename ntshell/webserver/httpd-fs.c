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
 *  @(#) $Id: httpd-fs.c 1342 2017-12-31 09:50:58Z coas-nagasima $
 */

#include "httpd.h"
#include "httpd-fs.h"
#include "ff.h"
#include "diskio.h"
#include <string.h>
#include "http-strings.h"
#include <kernel.h>
#include "kernel_cfg.h"
#include "syssvc/syslog.h"
#include "util/ntstdio.h"

#ifndef _MSC_VER
#ifndef strcat_s
#define strcat_s(dst, dsz, src) ntlibc_strcat(dst, src)
#endif
#endif

//#define FILE_DUMP
#ifdef FILE_DUMP
char path[256] = "";
FATFS fs;
#endif

#ifndef NULL
#define NULL 0
#endif /* NULL */

/*-----------------------------------------------------------------------------------*/
#ifdef FILE_DUMP
FRESULT scan_files(char* path, int size)
{
	FRESULT res;
	FILINFO fno;
	FATFS_DIR dir;
	int i;
	char *fn;   /* This function assumes non-Unicode configuration */

	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		i = ntlibc_strlen(path);
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;
			fn = fno.fname;
			if (fno.fattrib & AM_DIR) {
				ntstdio_sprintf(&path[i], "0:/%s", fn);
				res = scan_files(path, size);
				if (res != FR_OK) break;
				path[i] = 0;
			}
			else {
				syslog(LOG_ERROR, "%s/%s\n", path, fn);
			}
		}
	}

	return res;
}
#endif
/*-----------------------------------------------------------------------------------*/
int
httpd_fs_open(char *name, int len, struct httpd_fs_file *file)
{
	FRESULT res;
	FIL *fd = (FIL *)&file->fd;
	FILINFO fno;
	FATFS_DIR dir;

	file->pos = 0;
	file->len = 0;
	file->name = name;
	file->redirect = 0;
	memset(fd, 0, sizeof(FIL));

	if ((res = f_open(fd, name, FA_OPEN_EXISTING | FA_READ)) != FR_OK) {
		if ((res = f_opendir(&dir, name)) != FR_OK) {
			syslog(LOG_ERROR, "f_opendir(%s) => %d\n", name, res);
			return 0;
		}

		if ((res = f_readdir(&dir, &fno)) != FR_OK) {
			syslog(LOG_ERROR, "f_readdir(%s) => %d\n", name, res);
			return 0;
		}

		if (len != 0/*fno.fattrib & AM_DIR*/) {
			strcat_s(name, len, http_index_html);
			res = f_open(fd, name, FA_OPEN_EXISTING | FA_READ);
			file->redirect = res == FR_OK;
		}
		else
			res = FR_NO_FILE;

		if (res != FR_OK) {
			syslog(LOG_ERROR, "f_open(%s) => %d %x\n", name, res, fno.fattrib);
			return 0;
		}
	}

	file->len = fd->fsize;

	//syslog(LOG_ERROR, "httpd_fs_open(%d:%s) %d\n", drv, name, file->len);

	return 1;
}

/*-----------------------------------------------------------------------------------*/
int
httpd_fs_create(char *name, struct httpd_fs_file *file)
{
	FRESULT res;
	FIL *fd = (FIL *)&file->fd;

	file->pos = 0;
	file->len = 0;
	file->name = name;
	file->redirect = 0;
	memset(fd, 0, sizeof(FIL));

	if ((res = f_open(fd, name, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK) {
		syslog(LOG_ERROR, "f_open(%s) => %d\n", name, res);
		return 0;
	}

	file->pos = 0;
	file->len = 0;

	//syslog(LOG_ERROR, "httpd_fs_create(%d:%s) %d\n", drv, name, file->len);

	return 1;
}

/*-----------------------------------------------------------------------------------*/
int
httpd_fs_read(struct httpd_fs_file *file, void *dst, int len)
{
	FRESULT ret;
	UINT rlen = 0;
	FIL *fd = (FIL *)&file->fd;

	if ((ret = f_lseek(fd, file->pos)) != FR_OK) {
		syslog(LOG_ERROR, "f_lseek(%s, %d) => %d\n", file->name, file->pos, ret);
		return 0;
	}

	if (file->pos != fd->fptr) {
		syslog(LOG_ERROR, "f_lseek(%s, %d) != %d\n", file->name, file->pos, fd->fptr);
	}

	if ((ret = f_read(fd, dst, len, &rlen)) != FR_OK) {
		syslog(LOG_ERROR, "f_read(%s, 0x%p, %d) => %d\n", file->name, dst, len, ret);
		return 0;
	}

	//syslog(LOG_ERROR, "httpd_fs_read(%d:%s, %d, %d) => %d\n", file->drv, file->name, file->pos, len, rlen);

	return rlen;
}

/*-----------------------------------------------------------------------------------*/
int
httpd_fs_write(struct httpd_fs_file *file, const void *src, int len)
{
	FRESULT ret;
	UINT rlen = 0;
	FIL *fd = (FIL *)&file->fd;

	if ((ret = f_lseek(fd, file->pos)) != FR_OK) {
		syslog(LOG_ERROR, "f_lseek(%s, %d) => %d\n", file->name, file->pos, ret);
		return 0;
	}

	if (file->pos != fd->fptr) {
		syslog(LOG_ERROR, "f_lseek(%s, %d) != %d\n", file->name, file->pos, fd->fptr);
	}

	if ((ret = f_write(fd, src, len, &rlen)) != FR_OK) {
		syslog(LOG_ERROR, "f_write(%s, 0x%p, %d) => %d\n", file->name, src, len, ret);
		return 0;
	}

	file->pos += rlen;
	file->len += rlen;

	//syslog(LOG_ERROR, "httpd_fs_write(%d:%s, %d, %d) => %d\n", file->drv, file->name, file->pos, len, rlen);

	return rlen;
}

/*-----------------------------------------------------------------------------------*/
int httpd_fs_close(struct httpd_fs_file *file)
{
	FRESULT ret;
	FIL *fd = (FIL *)&file->fd;

	if ((ret = f_close(fd)) != FR_OK) {
		syslog(LOG_ERROR, "f_close(%s) => %d\n", file->name, ret);
		return 0;
	}

	memset(fd, 0, sizeof(FIL));

	return 1;
}
