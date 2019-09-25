#!/bin/sh

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2019 Junjiro R. Okajima

tmp=/tmp/$$
prefix=xoauth2-pipe
set -eu

TEST_USER=${TEST_USER:-Debian-exim}
cmd=$1
sender=$2
recipient=$3

cat << EOF > $tmp.body
From: $sender
To: $recipient
Subject: $prefix, $sender --> $recipient

test
EOF

{
cat << EOF
w
EHLO your.domain
w
MAIL FROM: <$sender>
w
RCPT TO: <$recipient>
w
DATA
w
EOF
cat $tmp.body
cat << EOF
.
w
QUIT
w
EOF
} > $tmp

wait_for_server() # pipe_server
{
	local stopme=0
	local server

	while [ $stopme -eq 0 ]
	do
		read server < $1
		echo "$server"
		case "$(echo $server | cut -f1 -d' ')" in
		[0-9][0-9][0-9])
			stopme=1
			;;
		esac
	done
}

conversation() # input pipe_server
{
	local i

	while read i
	do
	case "$i" in
	w)
		wait_for_server $2 1>&2
		;;
	*)
		echo "$i" 1>&2
		echo "$i\r"
		;;
	esac
	done < $1
}

stdalone()
{
	rm -f $tmp.p
	mknod -m 600 $tmp.p p
	conversation $tmp $tmp.p |
	{
	set -x
	sudo -u $TEST_USER xoauth2-pipe.sh smtp >> $tmp.p
	}
}

exim()
{
	set -x
	{
		cat $tmp.body
		echo by exim
	} |
	/usr/sbin/exim -odf -v -f $sender $recipient
}

mail()
{
	set -x
	{
		tail -1 $tmp.body
		echo by mail
	} |
	/usr/bin/mail -r $sender \
		-s "$(grep '^Subject:' $tmp.body | cut -f2- -d:)" \
		$recipient
}

$cmd

rm -f $tmp $tmp.*
