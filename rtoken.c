// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019-2022 Junjiro R. Okajima
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xoauth2.h"

int refresh_token(char *user, char *rbuf, size_t rsz)
{
	int err;
	FILE *fp;
	char *p;
	size_t len;
	struct stat st;

	Dpri("user %s\n", user);
	err = -1;
	fp = fopen(XOAUTH2_USER, "r");
	if (!fp)
		goto out;
	err = fstat(fileno(fp), &st);
	if (err)
		goto out_fp;
	if (st.st_uid
	    || (st.st_mode & (S_IWGRP | S_IXGRP | S_IRWXO))) {
		err = -1;
		Dpri(XOAUTH2_USER ", owner %d, mode 0%o\n",
		     st.st_uid, st.st_mode);
		goto out_fp;
	}

	len = strlen(user);
	do {
		p = fgets(rbuf, rsz, fp);
		if (!p) {
			if (feof(fp))
				goto out_no;
			err = ferror(fp);
			if (err)
				goto out_fp;
			/* should not happen */
			abort();
		}

		if (!*p || !isalpha(*p))
			continue;
		if (strncmp(rbuf, user, len)
		    || rbuf[len] != ':')
			continue;

		/* found */
		Dpri("rbuf %s", rbuf);
		p += len + 1;
		len = strlen(rbuf) - 1 - len - 1;
		memmove(rbuf, p, len);
		rbuf[len] = '\0';
		goto out_fp; /* success */
	} while (1);

out_no:
	*rbuf = '\0';
out_fp:
	fclose(fp);
out:
	return err;
}
