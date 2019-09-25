#!/bin/sh

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2019 Junjiro R. Okajima

set -eu
proto="$1"
tmp=/tmp/xoauth2-pipe.$proto.$$
#tmp=/tmp/xoauth2-pipe.$proto

#cmd="/usr/bin/openssl s_client -quiet -connect"
cmd="/usr/bin/stunnel3 -c -f -P '' -D 3 -r"
case "$proto" in
smtp)
	cmd="$cmd smtp.gmail.com:smtps";;
*)
	echo $0: Unknown protocol "$proto" 1>&2
	exit 1
esac

#id > $tmp.id
#tee $tmp.input |
#strace -f -o $tmp.s
/usr/sbin/xoauth2-pipe $proto $cmd 2> $tmp
test -s $tmp ||
rm -f $tmp
