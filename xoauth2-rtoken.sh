#!/bin/sh

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2019-2022 Junjiro R. Okajima

tmp=/tmp/$$
set -eu
me=$(basename $0)

abort()
{
	echo "$@" 1>&2
	exit 1
}

usage()
{
	abort Usage: $me browser_command
}

test $# -ne 1 &&
usage

browser="$1"
which $browser > /dev/null ||
abort could not found $browser

. /etc/xoauth2-client

curl -D $tmp \
	--data-urlencode client_id=$client_id \
	--data-urlencode redirect_uri=urn:ietf:wg:oauth:2.0:oob \
	--data-urlencode response_type=code \
	--data-urlencode scope=https://mail.google.com/ \
	https://accounts.google.com/o/oauth2/auth
grep '^location:' $tmp |
cut -f2- -d' ' |
xargs -L1 -n1 $browser &
sleep 1

cat << EOF
In the browser's screen, allow "xoauth2-pipe" to use Gmail API for your
Gmail account, and the screen will show you a refresh-token.
Then manually copy the refresh-token and paste it into the file
XOAUTH2_USER (/etc/xoauth2-user by default).  The format is simple, just
the account, colon, and the refresh-token in a line.
	your_account@gmail.com:your_refresh-token
EOF

rm -f $tmp $tmp.*
