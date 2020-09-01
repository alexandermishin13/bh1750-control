#!/usr/local/bin/python3.7
# -*- coding: utf-8 -*-

import sys, subprocess
import sysctl, sqlite3, argparse

""" Class of illuminance sensor sysctl OID
"""
class illuminance():
    def __init__(self):
        self.oid = "dev.bh1750.0.illuminance"

    """ Get brightness level from sysctl """
    def get_level(self):
        try:
            level = sysctl.filter(self.oid)[0].value
        except Exception as e:
            if str(e) == "Invalid ctl_type: 0":
                exit("No bh1750.ko loaded.\n")
            else:
                raise e
        return level


""" Class for manipulate of actions DB
"""
class action_db():
    def __init__(self, filename):
        sql_create = ("""
            CREATE TABLE IF NOT EXISTS illuminance (
                level INT NOT NULL,
                scope TEXT NOT NULL,
                action TEXT NOT NULL,
                PRIMARY KEY (level, scope)
            ) WITHOUT ROWID;

            CREATE INDEX IF NOT EXISTS index_scope_level ON illuminance(scope, level);
        """)
        try:
            self.conn = sqlite3.connect(filename)
        except sqlite3.OperationalError as e:
            sys.stderr.write(
                "%s\n" % e
            )
        self.create_database(sql_create)

    """ Creates a database
    """
    def create_database(self, sql):
        self.conn.executescript(sql)
        self.conn.commit()

    """ Adds an action for illuminance level and scope
    """
    def add(self, args):
        sql_add = ("""
            INSERT INTO illuminance (level, scope, action)
            VALUES (?, ?, ?)
            """)

        try:
            self.conn.execute(sql_add, (args.level, args.scope, args.execute,))
            self.conn.commit()
        except sqlite3.IntegrityError:
            sys.stderr.write(
                "Action on level:%slx for scope:\"%s\" already exists: Do You mean another scope?\n"
                % (args.level, args.scope)
            )
        except sqlite3.OperationalError as e:
            sys.stderr.write(
                "%s\n" % e
            )

    """ Deletes an action for scope and level if set
        or all actions for scope
    """
    def delete(self, args):
        level = args.level
        if level != None:
            sql_del = ("""
                DELETE FROM illuminance WHERE scope=? AND level=?
                """)
            self.conn.execute(sql_del, (args.scope, level,));
        else:
            sql_del = ("""
                DELETE FROM illuminance WHERE scope=?
                """)
            self.conn.execute(sql_del, (args.scope,));
        self.conn.commit()

    """ Get all actions for max illuminance level below
        specified one for all scopes
    """
    def run_actions(self, level):
        sql_get_actions = ("""
            SELECT a.* FROM illuminance a
            INNER JOIN (
                SELECT MAX(level) AS max_level, scope
                FROM illuminance WHERE level <= ? GROUP BY scope
            ) b
            ON a.level = b.max_level AND a.scope = b.scope;
            """)
        cursor = self.conn.cursor()
        for level, scope, action in cursor.execute(sql_get_actions, (level,)):
            subprocess.run(action.split())

    """ Prints all action definitions from DB
    """
    def list_all(self):
        scope_prev = ""
        sql_list_all = ("""
            SELECT * FROM illuminance ORDER BY scope, level
            """)

        cursor = self.conn.cursor()

        for level, scope, action in cursor.execute(sql_list_all):
            if scope == scope_prev:
                scope = " " * len(scope)
            else:
                scope_prev = scope
            sys.stdout.write("%s %5d %s\n" % (scope, level, action,))


cmd_help = {
            'run' : 'run the actions for actual lighting level',
            'list' : 'output all the actions list',
            'add' : 'add an action for a lighting level [and for a scope]',
            'delete' : 'delete an action'
           }

parser = argparse.ArgumentParser(description='An illuminance level action')
sub_parsers = parser.add_subparsers(title="commands", dest="command")

for command in ["list", "run"]:
    p = sub_parsers.add_parser(command, help=cmd_help[command])
for command in ["add", "delete"]:
    p = sub_parsers.add_parser(command, help=cmd_help[command])
    p.add_argument("--scope", default="Default",
                   help="scope for row of actions")
    if (command == "add"):
        p.add_argument("--level", required=True,
                       help="illuminance level for action")
        p.add_argument("--execute",
                       metavar="COMMAND",
                       help="command to execute")
    else:
        p.add_argument("--level",
                       help="illuminance level for action")


db_filename = '/var/db/bh1750d/action.sqlite'
action = action_db(db_filename)
light = illuminance()

if __name__ == "__main__":
    args = parser.parse_args()

    if args.command == "list":
        action.list_all()
    elif args.command == "run":
        level = light.get_level()
        print(level)
        action.run_actions(level)
    elif args.command == "delete":
        action.delete(args)
    elif args.command == "add":
        action.add(args)
