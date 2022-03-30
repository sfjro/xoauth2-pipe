
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2019-2022 Junjiro R. Okajima

SUPPORT_POP3 ?= 1
USER_FILE ?= /etc/xoauth2-user
MTA_GROUP ?= Debian-exim
Debug ?= 0

Tgt = xoauth2-pipe
Etc = $(addprefix /etc/,xoauth2-client)
Cmd = /usr/sbin/xoauth2-pipe.sh /usr/bin/xoauth2-rtoken
Bin = /usr/sbin/xoauth2-pipe
Install = ${Etc} ${Cmd} ${Bin}

all: ${Tgt}

clean:
	${RM} ${Tgt} ${DynamicH} *.o *~ .[A-z]*~

InstallCmd = install -o root -g root -m 555 -C -v
install: ${Install}

# mode 511 protects client-secret against strings(1),
# but how about ltrace/strace? or should we really hide client-secret?
# I don't know the answer.
${Bin}: /usr/sbin/%: %
	strip $<
	${InstallCmd} -m 511 $< $@
/usr/sbin/xoauth2-pipe.sh: /usr/sbin/%: %
	${InstallCmd} $< $@
/usr/bin/xoauth2-rtoken: /usr/bin/%: %.sh
	${InstallCmd} $< $@
${Etc}: /etc/%: %
	${InstallCmd} -g ${MTA_GROUP} -m 640 $< $@
install_user_file: ${USER_FILE}
${USER_FILE}: /etc/%: %
	${InstallCmd} -b -g ${MTA_GROUP} -m 640 $< $@

########################################

CPPFLAGS += -D_GNU_SOURCE -DNDEBUG
ifneq (${SUPPORT_POP3},0)
CPPFLAGS += -DSUPPORT_POP3
endif
CFLAGS += -O -Wall
ifneq (${Debug},0)
CPPFLAGS += -UNDEBUG
CFLAGS += -O0 -g
endif

Obj = xoauth2-pipe.o xoauth2.o gmail.o rtoken.o
ifneq (${SUPPORT_POP3},0)
Obj += pop3.o
endif
DynamicH = xoauth2-client.h

xoauth2-client.h: gen_h.sh xoauth2-client
	sh $^ ${USER_FILE} > $@

${Obj}: %.o: %.c $(filter-out ${DynamicH}, *.h) ${DynamicH}

xoauth2-pipe: LDLIBS += -lcurl -lmodpbase64
xoauth2-pipe: ${Obj}

-include priv.mk
