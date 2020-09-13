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

#include <sys/types.h>
#include <limits.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libutil.h>
#include <sqlite3.h>
#include <sys/sysctl.h>
#include <wordexp.h>
#include <spawn.h>
#include <sys/wait.h>

struct pidfh *pfh;
sqlite3 *db;
sqlite3_stmt *res_select;
int mib[4];
size_t mibLen = 4;
char mibSensor[32];
unsigned int pos = 0;
extern char **environ;

bool found = false;

bool backgroundRun = false;
bool debug = false;
char *dbName = "/var/db/bh1750/actions.sqlite";

/* Print usage after mistaken params */
static void
usage(char* program)
{
  printf("Usage:\n"
	 " %s [-b] [-i <pos>] [-f <dbfile>]\n", program);
}

/* Position of the device instance in the sysctl mib */
static unsigned int
get_position(char *flag_i)
{
	unsigned int number;
	const char *errstr;

	number = strtonum(flag_i, 0, 9, &errstr);
	if (errstr != NULL)
		errx(EXIT_FAILURE, "The device number is %s: %s (must be from %d to %d)", errstr, flag_i, 0, 9);

	return number;
}

/* Signals handler. Prepare the programm for end */
static void
termination_handler(int signum)
{
	int rc;
	char *drop_temp =
	    "DROP TABLE temp;";

	/* "just to make it right" */
	do {
		rc = sqlite3_exec(db, drop_temp, 0, 0, NULL);
	} while (rc == SQLITE_BUSY);

	/* Close the database */
	sqlite3_finalize(res_select);
	sqlite3_close(db);

	/* Remove pidfile and exit */
	pidfile_remove(pfh);

	exit(EXIT_SUCCESS);
}

/* Get and decode params */
static void
get_param(int argc, char **argv)
{
	int opt;

	while((opt = getopt(argc, argv, "hbdi:f:")) != -1) {
		switch(opt) {
		    case 'b': // run in background as a daemon
			backgroundRun = true;
			break;

		    case 'd': // debug messages
			debug = true;
			break;

		    case 'f': // db filename
			dbName = optarg;
			break;

		    case 'i':
			pos = get_position(optarg);
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

static const char*
wordexp_error(int err)
{
	switch (err) {
	    case WRDE_BADCHAR:
		return "One of the unquoted characters - <newline>, '|', '&', ';', '<', '>', '(', ')', '{', '}' - appears in an inappropriate context";
	    case WRDE_BADVAL:
		return "Reference to undefined shell variable when WRDE_UNDEF was set in flags to wordexp()";
	    case WRDE_CMDSUB:
		return "Command substitution requested when WRDE_NOCMD was set in flags to wordexp()";
	    case WRDE_NOSPACE:
		return "Attempt to allocate memory in wordexp() failed";
	    case WRDE_SYNTAX:
		return "Shell syntax error, such as unbalanced parentheses or unterminated string";
	    default:
		return "Unknown error from wordexp() function";
	}
}

static void
exec_cmd(char *arg)
{
	int rc, status;
	pid_t pid;
	wordexp_t we;

	if ((rc = wordexp(arg, &we, WRDE_NOCMD | WRDE_SHOWERR | WRDE_UNDEF)) == 0) {
		status = posix_spawn(&pid, we.we_wordv[0], NULL, NULL, we.we_wordv, environ);
		wordfree(&we);
//	if (debug) {
		if (status == 0) {
			if (waitpid(pid, &status, 0) == -1)
				perror("waitpid");
	//		else
	//			fprintf(stderr, "Child exited with status %i\n", status);
		}
		else
			fprintf(stderr, "posix_spawn: %s\n", strerror(status));
//	}
	}
	else
		fprintf(stderr, "Failed to parse an action string [%s]\n%d: %s\n", arg, rc, wordexp_error(rc));
}

int
main(int argc, char **argv)
{
	int rc;
	sqlite3_stmt *res_update;
	unsigned long illuminance, prevIlluminance = 0;
	size_t len = sizeof(illuminance);
	char *create_temp =
	    "PRAGMA temp_store = MEMORY;\n"
	    "PRAGMA journal_mode = OFF;\n"
	    "CREATE TEMPORARY TABLE IF NOT EXISTS temp (\n"
		"scopeid INT PRIMARY KEY NOT NULL,\n"
		"level INT NOT NULL\n"
	    ") WITHOUT ROWID;\n";

	char *update_temp =
	    "INSERT INTO temp (scopeid, level) VALUES (?, ?)\n"
	    "ON CONFLICT (scopeid)\n"
	    "DO UPDATE SET level=excluded.level;";

	char *select_actions =
	    "SELECT a1.level, a1.scopeid, a1.action, CASE WHEN b.level IS NULL THEN -1 ELSE b.level END\n"
	    "FROM illuminance a1\n"
	    "LEFT JOIN temp b\n"
	    "ON a1.scopeid = b.scopeid\n"
	    "INNER JOIN (\n"
		"SELECT MAX(level) AS max_level, scopeid\n"
		"FROM illuminance WHERE level <= ? GROUP BY scopeid\n"
	    ") a2\n"
	    "ON a1.level = a2.max_level AND a1.scopeid = a2.scopeid";

	/* Analize params and set backgroundRun and pos */
	get_param(argc, argv);

	sprintf(mibSensor, "dev.bh1750.%u.illuminance", pos);

	/* If background flag run as a daemon */
	if (backgroundRun)
		demonize();

	/* Intercept signals to our function */
	if (signal (SIGINT, termination_handler) == SIG_IGN)
		signal (SIGINT, SIG_IGN);
	if (signal (SIGTERM, termination_handler) == SIG_IGN)
		signal (SIGTERM, SIG_IGN);

	rc = sqlite3_open(dbName, &db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit (EXIT_FAILURE);
	}

	/* Create in memory temporary table */
	do {
		rc = sqlite3_exec(db, create_temp, 0, 0, NULL);
	} while (rc == SQLITE_BUSY);

	if (rc != SQLITE_OK ) {
		fprintf(stderr, "Cannot create temporary table: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit (EXIT_FAILURE);
	}

	/* Prepare to select actions for reached values of light levels */
	do {
		rc = sqlite3_prepare_v2(db, select_actions, -1, &res_select, NULL);
	} while (rc == SQLITE_BUSY);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit (EXIT_FAILURE);
	}

	/* Prepare to insert/update temporary values */
	do {
		rc = sqlite3_prepare_v2(db, update_temp, -1, &res_update, NULL);
	} while (rc == SQLITE_BUSY);

	if (rc != SQLITE_OK) {
		fprintf(stderr,
			"Failed to prepare temporary table: %s\n",
			sqlite3_errmsg(db));
		sqlite3_close(db);
		exit (EXIT_FAILURE);
	}

	/* Main loop */
	unsigned long maxLevel = ULONG_MAX;
	unsigned long colLevel, colLevelPrev, colScope;
	char *colAction;
	while(true) {
		/* Check if device is connected */
		if (!found) {
			/* If device was lost the name to mib resolution is needed again */
			if ((sysctlnametomib(mibSensor, mib, &mibLen) == 0) &&
			    (sysctl(mib, mibLen, &illuminance, &len, NULL, 0) == 0))
			{
				fprintf(stderr, "sysctl: Defice %u is found.\n", pos);
				found = true;
			}
		}
		else
			if (sysctl(mib, mibLen, &illuminance, &len, NULL, 0) == -1)
			{
				fprintf(stderr, "sysctl: Defice %u is not found.\n", pos);
				found = false;
			}

		/* If devise is not found do nothing but sleep */
		if (found) {
			/* Nothing is expected between the previous reached and
			   the previous measured (if DB is not changed).
			 */
			if ((illuminance < maxLevel) || (illuminance > prevIlluminance))
			{
				/* Get actual for the light level actions */
				maxLevel = 0;
				sqlite3_bind_int(res_select, 1, illuminance);
				while (sqlite3_step(res_select) == SQLITE_ROW) {
					colLevel = sqlite3_column_int(res_select, 0);
					colScope = sqlite3_column_int(res_select, 1);
					colLevelPrev = sqlite3_column_int(res_select, 3);
					colAction = (char *)sqlite3_column_text(res_select, 2);

					/* Calculate highest from reached levels */
					if (maxLevel < colLevel)
						maxLevel = colLevel;
        
					/* Do actions */
					if (colLevel != colLevelPrev)
						exec_cmd(colAction);

					/* Save the light levels to the temporary table */
					sqlite3_bind_int(res_update, 1, colScope);
					sqlite3_bind_int(res_update, 2, colLevel);

					if (sqlite3_step(res_update) != SQLITE_DONE)
						fprintf(stderr,
							"Failed to update temporary table: %s\n",
							sqlite3_errmsg(db));

					sqlite3_reset(res_update);

				} // while (sqlite3_step(res_select)

				/* Reset for next request */
				sqlite3_reset(res_select);

			} // if ((illuminance < maxLevel)...

			/* Store the measured value for next round */
			prevIlluminance = illuminance;

		} // if (sysctl(mib...


		sleep(5);
	}
}
