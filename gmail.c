// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019-2022 Junjiro R. Okajima
 */

#include <ctype.h>
#include <stdlib.h>
#include <modp_b64.h>
#include <unistd.h>
#include <curl/curl.h>

#include "xoauth2-client.h"
#include "xoauth2.h"

#define REQ_URL		"https://accounts.google.com/o/oauth2/token"
#define GRANT_TYPE	"refresh_token"

struct gmail_atoken {
	size_t sz;
	char *p;
};

struct reply {
	size_t rest;
	char *o, *p;
};

static size_t cb_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct reply *reply = userdata;

	size *= nmemb;
	if (reply->rest < size) {
		size = reply->rest;
		reply->rest = 0;
		goto out;
	}

	strncpy(reply->p, ptr, size);
	reply->p += size;
	reply->rest -= size;

out:
	return size;
}

static int xoauth2_atoken(char *access_token, size_t sz, char *refresh_token)
{
	int err, n;
	char a[BUFSIZ], *q;
	struct reply reply = {
		.rest	= sizeof(a),
		.o	= a,
		.p	= a
	};
	CURL *curl;
	CURLcode ccode;

	err = -1;
	ccode = curl_global_init(CURL_GLOBAL_SSL);
	if (ccode) {
		Dpri("curl_global_init(): %s\n", curl_easy_strerror(ccode));
		goto out;
	}
	curl = curl_easy_init();
	if (!curl) {
		Dpri("curl_easy_init() error.\n");
		goto out;
	}

	curl_easy_setopt(curl, CURLOPT_URL, REQ_URL);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	n = snprintf(a, sizeof(a),
		     "client_id=%s"
		     "&client_secret=%s"
		     "&refresh_token=%s"
		     "&grant_type=%s",
		     CLIENT_ID,
		     CLIENT_SECRET,
		     refresh_token,
		     GRANT_TYPE);
	if (n >= sizeof(a)) {
		Dpri("buffer is too small (%lu < %d)\n", sizeof(a), n);
		goto out_curl_easy;
	}
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &a);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
	/* curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, cb_header); */

	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	/* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

	ccode = curl_easy_perform(curl);
	if (ccode == CURLE_OK)
		err = 0;
	else {
		Dpri("curl_easy_perform() failed: %s\n",
		     curl_easy_strerror(ccode));
		goto out_curl_easy;
	}
	curl_easy_cleanup(curl);

	/* extract the access_token */
	if (!reply.rest) {
		Dpri("need more reply buffer.\n");
		goto out_curl;
	}
	reply.p = strstr(reply.o, "access_token");
	if (!reply.p) {
		err = -1;
		Dpri("there is no access_token.\n%s\n", reply.o);
		goto out_curl;
	}
	reply.p = strchr(reply.p, ':');
	reply.p += 3;
	strncpy(access_token, reply.p, sz);
	q = strchr(access_token, '"');
	if (q - access_token < sz) {
		*q = '\0';
		Dpri("%s", a);
	} else {
		err = -1;
		Dpri("need more space +%zu\n", q - access_token - sz);
	}
	goto out_curl;

out_curl_easy:
	curl_easy_cleanup(curl);
out_curl:
	curl_global_cleanup();
out:
	return err;
}

int gmail_xoauth2(struct xoauth2_arg *arg, char *rbuf, size_t rsz)
{
	int err, n;
	ssize_t ssz;
	size_t sz;
	char a[1024], b[1024], rtoken[128];

	err = refresh_token(arg->user, rtoken, sizeof(rtoken));
	if (err)
		goto out;
	Dpri("rtoken %s\n", rtoken);
	err = xoauth2_atoken(a, sizeof(a), rtoken);
	if (err)
		goto out;
	Dpri("a %s\n", a);

	err = -1;
	n = snprintf(b, sizeof(b), "user=%s%cauth=Bearer %s%c%c",
		     arg->user, 1, a, 1, 1);
	if (n < 0 || n >= sizeof(b)) {
		Dpri("b failed\n");
		goto out;
	}
	sz = n;
	n = snprintf(a, sizeof(a), "%s ", arg->cmd);
	if (n > 16) {
		Dpri("illegal cmd '%s'\n", arg->cmd);
		goto out;
	}
	if (n + modp_b64_encode_len(sz) > sizeof(a) - 1) {
		Dpri("enlarge a\n");
		goto out;
	}
	err = modp_b64_encode(a + n, b, sz);
	if (err < 0) {
		Dpri("encode error\n");
		goto out;
	}
	/* strcat(a, "\r\n"); */
	strcat(a, "\n");

	err = -1;
	sz = strlen(a);
	Dpri("a %s", a);
	ssz = write(arg->wfd, a, sz);
	if (ssz < 0)
		Dpri("write: %m\n");
	if (ssz != sz) {
		Dpri("write sz %zu, ssz %zd\n", sz, ssz);
		goto out;
	}

	ssz = read(arg->rfd, a, sizeof(a));
	if (ssz < 0) {
		Dpri("read: %m\n");
		goto out;
	}
	a[ssz] = '\0';
	Dpri("a %s", a);
	if (ssz <= rsz) {
		err = 0;
		strcpy(rbuf, a);
	} else
		Dpri("read ssz %zd, rsz %zu\n", ssz, rsz);

out:
	return err;
}
