// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2019 Junjiro R. Okajima
 */

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xoauth2.h"

/* parent's point of view */
enum {
	TO_CHILD,
	FROM_CHILD,
	Npfd
};
static int pfd[Npfd][2], cstatus;
static pid_t pid;

static void child_action(int sig, siginfo_t *info, void *ucontext)
{
	int e;

	e = errno;
	wait(&cstatus); /* unused */
	errno = e;
}

static void child(char *argv[])
{
	int err;

	err = dup2(pfd[TO_CHILD][0], STDIN_FILENO);
	if (err != STDIN_FILENO) {
		perror( "child pfd[TO_CHILD][0]");
		goto out;
	}
	err = close(pfd[TO_CHILD][1]);
	if (!err)
		err = close(pfd[FROM_CHILD][0]);
	if (err) {
		perror("child close");
		goto out;
	}
	err = dup2(pfd[FROM_CHILD][1], STDOUT_FILENO);
	if (err != STDOUT_FILENO) {
		perror("child pfd[FROM_CHILD][1]");
		goto out;
	}

	err = execve(argv[2], argv + 2, environ);
	perror("child execve");

out:
	exit(EXIT_FAILURE);
}

static int do_fork(char *argv[])
{
	int err;
	struct sigaction sa = {
		.sa_sigaction	= child_action,
		.sa_flags	= SA_SIGINFO /* | SA_NOCLDSTOP | SA_NOCLDWAIT */
	};

	err = pipe(pfd[TO_CHILD]);
	if (!err)
		err = pipe(pfd[FROM_CHILD]);
	if (err) {
		perror("pipe");
		goto out;
	}

	sigemptyset(&sa.sa_mask);
	err = sigaction(SIGCHLD, &sa, /*old sa*/NULL);
	if (err) {
		perror("sigaction");
		goto out;
	}

	pid = fork();
	if (pid < 0) {
		perror("fork");
		goto out;
	}
	if (!pid) {
		/* never return on success */
		child(argv);
		err = EXIT_FAILURE;
		goto out;
	}

	/* parent */
	g.arg.wfd = pfd[TO_CHILD][1];
	g.arg.rfd = pfd[FROM_CHILD][0];
	err = close(pfd[TO_CHILD][0]);
	if (!err)
		err = close(pfd[FROM_CHILD][1]);
	if (err)
		perror("close");

out:
	return err;
}

static void term_child(void)
{
	int err;

	err = close(pfd[TO_CHILD][1]);
	if (err)
		kill(pid, SIGTERM);
}

int main(int argc, char *argv[])
{
	int err, efd;
	unsigned int status;
	char buf[BUFSIZ], *proto;
	struct epoll_event ev;
	ssize_t ssz;

	proto = argv[1];
	if (!strcmp(proto, "smtp"))
		g.proto = SMTP;
	else {
		err = EXIT_FAILURE;
		fprintf(stderr, "%m, %s\n", proto);
		fprintf(stderr, "Usage: %s smtp cmd\n", argv[0]);
		goto out;
	}

	err = do_fork(argv);
	if (err)
		goto out;

	efd = epoll_create1(/*flags*/0);
	if (efd < 0) {
		perror("epoll_create1");
		goto out_child;
	}
	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	err = epoll_ctl(efd, EPOLL_CTL_ADD, ev.data.fd, &ev);
	if (!err) {
		ev.data.fd = pfd[FROM_CHILD][0];
		err = epoll_ctl(efd, EPOLL_CTL_ADD, ev.data.fd, &ev);
	}
	if (err) {
		perror("epoll_ctl");
		goto out_child;
	}

	status = XOAUTH2_YET;
	do {
		err = epoll_wait(efd, &ev, 1, -1);
		if (err < 0) {
			perror("epoll_wait");
			break;
		}
		err = EXIT_SUCCESS;
		Dpri("events 0x%x\n", ev.events);
		if (ev.events & EPOLLHUP)
			break;
		ssz = read(ev.data.fd, buf, sizeof(buf));
		if (ssz < 0) {
			perror("read from child");
			break;
		}

		if (ev.data.fd == STDIN_FILENO) {
			if (!ssz)
				break;
			if (!(status & XOAUTH2_DONE)) {
				err = xoauth2(buf, &status);
				if (err)
					break;
				if (status & XOAUTH2_SKIP)
					continue;
			}
			/* blocking I/O */
			ssz = write(pfd[TO_CHILD][1], buf, ssz);
			if (err < 0) {
				perror("WRITE");
				break;
			}
		} else if (ev.data.fd == pfd[FROM_CHILD][0]) {
			ssz = write(STDOUT_FILENO, buf, ssz);
			if (err < 0) {
				perror("WRITE");
				break;
			}
		} else {
			Dpri("ev.data.fd %d\n", ev.data.fd);
			assert(0);
			break;
		}
	} while (1);

out_child:
	term_child();
out:
	return err;
}
