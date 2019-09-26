/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) 2019 Junjiro R. Okajima
 */

#ifndef __POP3_H__
#define __POP3_H__

#include <stdio.h>
#include <string.h>

#ifdef SUPPORT_POP3
#define proto_pop3_test(proto)	(!strcmp(proto, "pop3"))
#define proto_pop3()		do {} while (0)

/* pop3.c */
int xoauth2_pop3(char *buf, unsigned int *status);

#else
#define proto_pop3_test(proto)		0
#define proto_pop3()					\
	fprintf(stderr, "SUPPORT_POP3 is disabled.\n")
#define xoauth2_pop3(buf, status)	0
#endif

#endif
