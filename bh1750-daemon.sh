#!/bin/sh
#
# $FreeBSD: 2020-09-10 13:20:00Z mishin $
#
# PROVIDE: bh1750_daemon
# REQUIRE: DAEMON
# KEYWORD: nojail shutdown
#
# Add the following lines to /etc/rc.conf to enable bh1750_daemon:
#
# bh1750_daemon_enable (bool):	Set to "NO"  by default.
#				Set to "YES" to enable bh1750_daemon.
# bh1750_daemon_profiles (str):	Set to "" by default.
#				Define your profiles here.
# bh1750_daemon_devfile (str):	Set to "/dev/bh1750/0" by default.
#				Define a cdev
# bh1750_daemon_dbfile (str):	Set to "/var/db/bh1750/actions.sqlite" by default.
#				Set sqlite3 db file here
# bh1750_daemon_pidfile (str):	Set to "/var/run/bh1750-daemon.pid" by default
#				Define a pid file here.
# bh1750_daemon_flags (str):	Set to "" by default.
#				Extra flags passed to start command.
#
# For a profile based configuration use variables like this:
#
# bh1750_daemon_profiles="XXX"
# bh1750_daemon_XXX_devfile
# bh1750_daemon_XXX_dbfile
# bh1750_daemon_XXX_pidfile
# bh1750_daemon_XXX_flags

. /etc/rc.subr

name=bh1750_daemon
rcvar=bh1750_daemon_enable

load_rc_config $name

: ${bh1750_daemon_enable:="NO"}
: ${bh1750_daemon_flags:="-b"}
: ${bh1750_daemon_devfile:="/dev/bh1750/0"}
: ${bh1750_daemon_dbfile:="/var/db/bh1750/actions.sqlite"}
: ${bh1750_daemon_pidfile:="/var/run/bh1750-daemon.pid"}

pidfile=${bh1750_daemon_pidfile}
bh1750_daemon_bin="/usr/local/sbin/bh1750-daemon"

is_profile_exists() {
	local profile

	for profile in $bh1750_daemon_profiles; do
		[ "$profile" = "$1" ] && return 0;
	done

	return 1
}

if [ -n "${bh1750_daemon_profiles}" ]; then
	if [ -n "$2" ]; then
		profile="$2"
		if ! is_profile_exists $profile; then
			echo "$0: no such profile defined in bh1750_daemon_profiles."
			exit 1
		fi
		eval pidfile=\${bh1750_daemon_${profile}_pidfile:-"/var/run/bh1750-daemon-${profile}.pid"}
		eval bh1750_daemon_devfile=\${bh1750_daemon_${profile}_devfile:-"${bh1750_daemon_devfile}"}
		eval bh1750_daemon_dbfile=\${bh1750_daemon_${profile}_dbfile:-"${bh1750_daemon_dbfile}"}
	elif [ -n "$1" ]; then
		for profile in ${bh1750_daemon_profiles}; do
			echo "===> bh1750-daemon profile: ${profile}"
			/usr/local/etc/rc.d/bh1750-daemon $1 ${profile}
			retcode="$?"
			if [ "0${retcode}" -ne 0 ]; then
				failed="${profile} (${retcode}) ${failed:-}"
			else
				success="${profile} ${success:-}"
			fi
		done
		exit 0
	fi
fi

command=${bh1750_daemon_bin}
command_args="-s ${bh1750_daemon_devfile} -f ${bh1750_daemon_dbfile} -p ${pidfile} ${bh1750_daemon_flags}"

run_rc_command "$@"
