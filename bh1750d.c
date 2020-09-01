/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (C) 2020, Alexander Mishin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <libutil.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

struct pidfh *pfh;

bool foregroundRun = false;

/* Print usage after mistaken params */
static void
usage(char* program)
{
  printf("Usage:\n %s [-f]\n", program);
}

/* Get and decode params */
static void
get_param(int argc, char **argv)
{
	int opt;

	while((opt = getopt(argc, argv, "hf")) != -1) {
		switch(opt) {
		    case 'f': // stay on foreground
			foregroundRun = true;
			break;

		    case 'h': // help request
		    case '?': // unknown option...
		    default:
			usage(argv[0]);
        exit (0);
		}
	}
}

/* Demonize wrapper */
static void
demonize(void)
{
	pid_t otherpid;

	/* Try to create a pidfile */
	pfh = pidfile_open(NULL, 0600, &otherpid);
	if (pfh == NULL) {
		if (errno == EEXIST)
			errx (EXIT_FAILURE, "Daemon already running, pid: %jd.", (intmax_t)otherpid);

		/* If we cannot create pidfile from other reasons, only warn. */
		warn ("Cannot open or create pidfile");
		/*
		* Even though pfh is NULL we can continue, as the other pidfile_*
		* function can handle such situation by doing nothing except setting
		* errno to EDOOFUS.
		*/
	}

	/* Try to demonize the process */
	if (daemon(0, 0) == -1) {
		pidfile_remove(pfh);
		errx (EXIT_FAILURE, "Cannot daemonize");
	}

	pidfile_write(pfh);
}

int
main(int argc, char **argv)
{
	/* Analize params and set
	 * configureOnly, backgroundRun
	 */
	get_param(argc, argv);

	/* If no foreground flag run as a daemon */
	if (!foregroundRun)
		demonize();

	/* Main loop */
	while(true) {
		sleep(30);
	}

	exit (EXIT_SUCCESS);
}
