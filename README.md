# bh1750-control

## About

The tools an daemon require a bh1750 sensor connected to the i2c bus and
this kernel driver: https://gitlab.com/alexandermishin13/bh1750-kmod.

An idea of this tool is plain:
* There is one or more rows of defined pairs: (lighting level -> action);
* Each such row of levels has its own scope;
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

## Installation

All You need to install:
```
make
sudo make install
```
If You want to control the `bh1750` sensor by daemon You may need to add to
`/etc/rc.conf` following line:
```
bh1750_daemon_enable="YES"
bh1750_daemon_number="0" # Sensor number in sysctl
bh1750_daemon_dbfile="/var/db/bh1750/actions.sqlite"
```
...or execute a command from sources root directory:
```
sudo cp ./rc.conf.d/ /usr/local/etc/
```
..change it for your needs and run:
```
sudo service bh1750-daemon start
```
The above values are hardcoded in the daemon source code as defaults.
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

### Daemon

Or You can use a daemon `bh1750-daemon` for run the actions.
The daemon reads the `dev.bh1750.%u.illuminance` variable for measured light
level, searches the database, and runs the commands it finds.

## Status

Still in development but works fine for me.
I use it mainly to adjust the brightness of the tm1637 display depending on
the ambient light: https://gitlab.com/alexandermishin13/tm1637-kmod
