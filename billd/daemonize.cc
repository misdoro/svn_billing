/*---------------------------------------------------------------------------*\
  $Id: daemonize.c 7134 2008-01-13 00:07:56Z bmc $

  NAME
        daemonize - run a command as a Unix daemon

  DESCRIPTION

        See accompanying man page for full details.

  LICENSE

        This source code is released under a BSD-style license. See the
        LICENSE file for details.

  Copyright (c) 2003-2007 Brian M. Clapper, bmc <at> clapper <dot> org
  Modified 2008 Misha V. Doronin, misha <at> doronin <dot> org
\*---------------------------------------------------------------------------*/

#include "billing.h"

/*---------------------------------------------------------------------------*\
                                  Globals
\*---------------------------------------------------------------------------*/

static int nullFd = -1;
static int outFd = -1;
static int errFd = -1;

/*---------------------------------------------------------------------------*\
                             Private Functions
\*---------------------------------------------------------------------------*/

int switchUser(const char *userName,
	   const char *pidFile,
	   uid_t uid)
{
	struct passwd *pw;

	if (uid != 0) {
		logmsg(DBG_ALWAYS,"Must be root to specify a different user.");
	} else {
		if ((pw = getpwnam(userName)) == NULL)
			return 1;
		if (pidFile != NULL) {
			if (chown(pidFile, pw->pw_uid, pw->pw_gid) == -1) {
				return 2;
			}
		}
		if (setgid(pw->pw_gid) != 0)
			return 3;

		if (setegid(pw->pw_gid) != 0)
			return 4;

		if (setuid(pw->pw_uid) != 0)
			return 5;

		if (seteuid(pw->pw_uid) != 0)
			return 6;
	};
	return 0;
}

static int
open_output_file(const char *path)
{
	int flags = O_CREAT | O_WRONLY;

	if (cfg.appendlogs) {
		logmsg(DBG_DAEMON,"Appending to %s\n", path);
		flags |= O_APPEND;
	} else {
		logmsg(DBG_DAEMON,"Overwriting %s\n", path);
		flags |= O_TRUNC;
	}

	return open(path, flags, 0666);
}

int open_output_files()
{
	/* open files for stdout/stderr */

	if (cfg.logfile.length() > 0) {
		if ((nullFd = open("/dev/null", O_WRONLY)) == -1)
			return 1;

		close(STDIN_FILENO);
		dup2(nullFd, STDIN_FILENO);

		if (cfg.logfile.length() > 0) {
			if ((outFd = open_output_file(cfg.logfile.c_str())) == -1) {
				return 2;
			}
		} else {
			outFd = nullFd;
		}
		errFd = outFd;
	}
	return 0;
}

static int
redirect_stdout_stderr()
{
	int rc = 0;

	/* Redirect stderr/stdout */

	if (cfg.logfile.length() > 0) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		dup2(nullFd, STDIN_FILENO);
		dup2(outFd, STDOUT_FILENO);
		dup2(errFd, STDERR_FILENO);
		rc = 1;
	}
	return rc;
}

/*---------------------------------------------------------------------------*\
                               Main Program
\*---------------------------------------------------------------------------*/

int
daemonize(void)
{
	uid_t uid = getuid();
	FILE *fPid = NULL;
	int noclose = 0;
	int lockFD;

	if (geteuid() != uid){
		logmsg(DBG_DAEMON,"This executable is too dangerous to be setuid.\n");
		exit(1);
	};

	if (cfg.lockfile.length() > 0) {
		lockFD = open(cfg.lockfile.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
		if (lockFD < 0)
			return 1;
		if (flock(lockFD, LOCK_EX | LOCK_NB) != 0)
			return 2;
	}
	if (cfg.pidfile.length() > 0) {
		logmsg(DBG_DAEMON,"Opening PID file \"%s\".\n", cfg.pidfile.c_str());
		if ((fPid = fopen(cfg.pidfile.c_str(), "w")) == NULL) {
			return 3;
		}
	}
	open_output_files();

	if (cfg.user.length() > 0)
		switchUser(cfg.user.c_str(), cfg.pidfile.c_str(), uid);

	if (chdir(cfg.workingdir.c_str()) != 0) {
		return 4;
	}
	logmsg(DBG_DAEMON,"Daemonizing...");

	if (redirect_stdout_stderr())
		noclose = 1;

	if (daemon(1, noclose) != 0)
		return 5;

	if (fPid != NULL) {
		fprintf(fPid, "%d\n", getpid());
		fclose(fPid);
	}
	if (chdir(cfg.workingdir.c_str()) != 0) {
		return 6;
	}
	return 0;
}
