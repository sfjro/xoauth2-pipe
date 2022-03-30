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

url='https://accounts.google.com/o/oauth2/auth'
url="$url?client_id=$client_id"
url="$url&redirect_uri=urn:ietf:wg:oauth:2.0:oob"
url="$url&response_type=code"
url="$url&scope=https://mail.google.com/"
$browser $url &
sleep 1

cat << EOF
In your browser, allow xoauth2-pipe to access your account, and get
Authorisation code.
Now Copy/paste it and hit enter key.
EOF
read authcode

curl -s \
	--data-urlencode client_id=$client_id \
	--data-urlencode client_secret=$client_secret \
	--data-urlencode code=$authcode \
	--data-urlencode redirect_uri=urn:ietf:wg:oauth:2.0:oob \
	--data-urlencode grant_type=authorization_code \
	https://accounts.google.com/o/oauth2/token \
|
tee $tmp|
fgrep -w refresh_token || :

cat << EOF
In the browser's screen, allow "xoauth2-pipe" to use Gmail API for your
Gmail account, and the screen will show you a refresh-token.
Then manually copy the refresh-token and paste it into the file
XOAUTH2_USER (/etc/xoauth2-user by default).  The format is simple, just
the account, colon, and the refresh-token in a line.
	your_account@gmail.com:your_refresh-token
EOF

rm -f $tmp $tmp.*
