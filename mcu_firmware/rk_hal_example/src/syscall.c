/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <_ansi.h>
#include <reent.h>
#include <sys/stat.h>

// _close_r
int _close_r(struct _reent *r, int file)
{
    return -1;
}

// _fstat_r
int _fstat_r(struct _reent *r, int file, struct stat *st)
{
    st->st_mode = S_IFCHR;

    return 0;
}

// _getpid_r
int _getpid_r(struct _reent *r)
{
    return 1;
}

// _isatty_r
int _isatty_r(struct _reent *r, int file)
{
    return 1;
}

// _kill_r
int _kill_r(struct _reent *r, int pid, int sig)
{
    return -1;
}

// _lseek_r
_off_t _lseek_r(struct _reent *r, int file, _off_t ptr, int dir)
{
    return (_off_t)0;
}

// _read_r
_ssize_t _read_r(struct _reent *r, int file, void *ptr, size_t len)
{
    return 0;
}
