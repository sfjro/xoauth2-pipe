#!/bin/sh

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2019-2022 Junjiro R. Okajima

set -eu

. ./$1
cat << EOF
#ifndef __GMAIL_CLIENT_H__
#define __GMAIL_CLIENT_H__

#define CLIENT_ID	"$client_id"
#define CLIENT_SECRET	"$client_secret"

/*
 * Format of XOAUTH2_USER:
 * - gmali-address ":" refresh-token
 * - no space in a line is allowed.
 * - only the lines which begins with an alphabet letter are valid.
 */
#define XOAUTH2_USER	"$2"

#endif
EOF
