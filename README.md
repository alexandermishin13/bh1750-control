# bh1750-control

## About

The control utility and daemon require a bh1750 sensor connected to the i2c bus
and this kernel driver: https://gitlab.com/alexandermishin13/bh1750-kmod.

An idea of this tool is plain:
* There is one or more rows of defined pairs: (lighting level -> action);
* Each row of levels has its own scope;
* After measuring the ambient light level, the action with the highest reached
level value for each scope is performed.

## Prerequstives

For run `bh1750-control` You need to install following Python modules:
```
sudo pkg install py37-sqlite3
sudo pkg install py37-sysctl
```
The SQLite library must also be compiled with neither SQLITE_OMIT_FOREIGN_KEY
nor SQLITE_OMIT_TRIGGER.

## Installation and run

All You need to install:
```
make
sudo make install
```
If You want to control the `bh1750` sensor by daemon You may need to add
 following lines to `/etc/rc.conf`:
```
bh1750_daemon_enable="YES"
bh1750_daemon_number="0" # Sensor number in sysctl.
bh1750_daemon_dbfile="/var/db/bh1750/actions.sqlite"
bh1750_daemon_pidfile="/var/db/bh1750-daemon.pid"
```
...or use a profile based configuration for multiple sensors. (See
`rc.conf.d/bh1750_daemon.example`. You can copy it as `bh1750_daemon` to
`/usr/local/etc/rc.conf.d/` instead of use `/etc/rc.conf` and change it for
your needs).

Now You can run the service[-s] by:
```
sudo service bh1750-daemon start
```
The above values are hardcoded in the daemon service script as defaults.
If you omit lines with them, they will still be used.

## Usage

### DB control utility

The tool `bh1750-control` allows you to perform the following actions with the
database:
* **add** an `action` for the `level` in the `scope`;
* **delete** an `action` for the `level` in the `scope` or the whole `scope`;
* **list** all defined actions;
* **run** for measure light, compute levels and execute actions.
You can run it by `cron` at most once a minute.

### Service

Or You can use a service `bh1750-daemon` for run the actions.
The daemon reads the `/dev/bh1750/<n>` character device for measured light
level, searches the database for commands and runs it. With the daemon You may
set delays for commands. During the delay, the command can be automatically
canceled if the illumination changes significantly, or executed after the delay
has expired.

## Status

Still in development but works fine for me.
I use it mainly to adjust the brightness of the tm1637 display depending on
the ambient light: https://gitlab.com/alexandermishin13/tm1637-kmod
