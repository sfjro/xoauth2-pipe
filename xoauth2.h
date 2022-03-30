/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Copyright (C) 2019-2022 Junjiro R. Okajima
 */

#ifndef __XOAUTH2_H__
#define __XOAUTH2_H__

#include <stddef.h>

#include "debug.h"
#include "xoauth2-cred.h"
#include "pop3.h"

/* status */
#define XOAUTH2_YET	1
#define XOAUTH2_DONE	(1 << 1)
#define XOAUTH2_SKIP	(1 << 2)
#define XOAUTH2_PASSED	(1 << 3)

enum proto {
	SMTP,
	POP3
	/* will be added more */
};

struct xoauth2_arg {
	int wfd, rfd;
	char *cmd;
	char *user;
};

struct g {
	struct xoauth2_arg arg;
	char cmd[16], user[64], res[256];
	enum proto proto;
};

/* xoauth2.c */
extern struct g g;
int xoauth2(char *buf, unsigned int *status);

/* rtoken.c */
int refresh_token(char *user, char *rbuf, size_t rsz);

/* gmail.c */
int gmail_xoauth2(struct xoauth2_arg *arg, char *rbuf, size_t rsz);

#endif
