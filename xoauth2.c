// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 Junjiro R. Okajima
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xoauth2.h"

struct g g = {
	.arg = {
		.cmd	= "AUTH XOAUTH2",
		.user	= g.user
	}
};

int xoauth2(char *buf, unsigned int *status)
{
	int err, n;
	size_t sz;

	err = 0;
	switch (g.proto) {
	case SMTP:
		/* MAIL FROM: <user@gmail.com> */
		n = sscanf(buf, " %4s %4s : < %s >", g.cmd, g.cmd + 5, g.user);
		g.cmd[4] = ' ';
		if (n == 3
		    && !strncasecmp(g.cmd, "MAIL FROM", 9)) {
			sz = strlen(g.user);
			if (g.user[sz -1] == '>')
				g.user[sz - 1] = '\0';
			Dpri("err %d, cmd %s, user %s\n", err, g.cmd, g.user);
			err = gmail_xoauth2(&g.arg, g.res, sizeof(g.res));
			/* 235 2.7.0 Accepted */
			if (!err && *g.res == '2')
				*status = XOAUTH2_DONE;
		}
		break;

	case POP3:
		err = xoauth2_pop3(buf, status);
		break;

	default:
		assert(0);
		err = EXIT_FAILURE;
	}

	return err;
}
