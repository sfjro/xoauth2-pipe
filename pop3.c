// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 Junjiro R. Okajima
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "xoauth2.h"

int xoauth2_pop3(char *buf, unsigned int *status)
{
	int err, n;

	/*
	 * USER user
	 * PASS password
	 */
	err = 0;
	n = sscanf(buf, " %s %s ", g.cmd, g.user);
	if (n != 2)
		goto out;

	if (!strncasecmp(g.cmd, "USER", 4)) {
		if (!strchr(g.user, '@'))
			strcat(g.user, "@gmail.com");
		err = gmail_xoauth2(&g.arg, g.res, sizeof(g.res));
		if (err)
			goto out;
		/* +OK Welcome. */
		if (*g.res != '+')
			goto out;
		*status = XOAUTH2_SKIP | XOAUTH2_PASSED;
		printf("+OK Accepted via %s.\r\n",
		       program_invocation_short_name);
		fflush(stdout);
	} else if (!strncasecmp(g.cmd, "PASS", 4)) {
		/* ignore the command */
		*status |= XOAUTH2_SKIP;
		if (*status & XOAUTH2_PASSED)
			*status |= XOAUTH2_DONE;
		printf("+OK Ignored by %s.\r\n",
		       program_invocation_short_name);
		fflush(stdout);
	}

out:
	return err;
}
