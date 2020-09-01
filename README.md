## About

An idea of this tool is plain:
* to record a sets of ambient lighting levels as database records with a
specific action for each level;
* Each set of levels have its own scope;
* For each scope an actual action is action for a level which less then
a current;
* When the tool is periodically launched, for each scope will be performed
an action corresponding to the last achieved lighting level. All other levels
will be ignored.

## Installation

For run `bh1750tool` You need to install following Python modules:
```
sudo pkg install py37-sqlite3
sudo pkg install py37-sysctl
```
