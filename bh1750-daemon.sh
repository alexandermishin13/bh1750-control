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
# bh1750_daemon_flags (str):	Set to "" by default.
#				Extra flags passed to start command.

. /etc/rc.subr

name=bh1750_daemon
rcvar=bh1750_daemon_enable

load_rc_config $name

: ${bh1750_daemon_enable:="NO"}
: ${bh1750_daemon_flags:="-b"}
: ${bh1750_daemon_number:="0"}
: ${bh1750_daemon_dbfile:="/var/db/bh1750/actions.sqlite"}
: ${bh1750_daemon_pidfile:="/var/run/bh1750-daemon.pid"}

pidfile=${bh1750_daemon_pidfile}
bh1750_daemon_bin="/usr/local/sbin/bh1750-daemon"
command=${bh1750_daemon_bin}
command_args="-i ${bh1750_daemon_number} -f ${bh1750_daemon_dbfile} -p ${pidfile}"

run_rc_command "$@"
