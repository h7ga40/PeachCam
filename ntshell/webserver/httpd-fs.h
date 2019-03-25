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
 *  @(#) $Id: httpd-fs.h 1192 2017-03-08 10:30:27Z coas-nagasima $
 */
#ifndef __HTTPD_FS_H__
#define __HTTPD_FS_H__

#define HTTPD_FS_STATISTICS 0

struct httpd_fs_file {
  int pos;
  int len;
  const char *name;
  int redirect;
  char fd[64/*sizeof(FIL)*/];
};

/* file must be allocated by caller and will be filled in
   by the function. */
int httpd_fs_open(char *name, int len, struct httpd_fs_file *file);
int httpd_fs_create(char *name, struct httpd_fs_file *file);
int httpd_fs_read(struct httpd_fs_file *file, void *dst, int len);
int httpd_fs_write(struct httpd_fs_file *file, const void *src, int len);
int httpd_fs_close(struct httpd_fs_file *file);

#ifdef HTTPD_FS_STATISTICS
#if HTTPD_FS_STATISTICS == 1
uint16_t httpd_fs_count(char *name);
#endif /* HTTPD_FS_STATISTICS */
#endif /* HTTPD_FS_STATISTICS */

void httpd_fs_init(void);

#endif /* __HTTPD_FS_H__ */
