/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) 2019-2022 Junjiro R. Okajima
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <sys/types.h>
#include <unistd.h>

#ifndef NDEBUG
#define Dpri(fmt, ...)	fprintf(stderr, "[%d]:%s:%u: " fmt, \
				getpid(), __func__, __LINE__, ##__VA_ARGS__)
#else
#define Dpri(fmt, ...)	do {} while (0)
#endif

#endif
