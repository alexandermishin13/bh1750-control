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
        sql_create = """
            CREATE TABLE IF NOT EXISTS scopes (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                scope TEXT NOT NULL UNIQUE
            );
            INSERT OR IGNORE INTO scopes VALUES(0,'Default');
            CREATE TABLE IF NOT EXISTS illuminance (
                level INT NOT NULL,
                scopeid INT NOT NULL,
                delay INT NOT NULL DEFAULT 0,
                action TEXT NOT NULL,
                PRIMARY KEY (level, scopeid),
                CONSTRAINT fk_scopes
                    FOREIGN KEY (scopeid)
                    REFERENCES scopes(id)
                    ON UPDATE CASCADE
                    ON DELETE CASCADE
            ) WITHOUT ROWID;
            CREATE INDEX IF NOT EXISTS index_scope_level ON illuminance(scopeid, level);
            PRAGMA foreign_keys=ON;
        """

        try:
            self.conn = sqlite3.connect(filename)
        except sqlite3.OperationalError as e:
            sys.stderr.write("%s\n" % e)

        self.cursor = self.conn.cursor()
        self.create_database(sql_create)

    """ Creates a database
    """
    def create_database(self, sql):
        self.cursor.executescript(sql)
        self.conn.commit()

    """ Adds an action for illuminance level and scope
    """
    def add(self, args):
        sql_add_scope = ("""
            INSERT OR IGNORE INTO scopes(scope) VALUES(?);
            """)
        sql_add_level = ("""
            INSERT INTO illuminance (level, scopeid, delay, action)
            VALUES (?, (SELECT id FROM scopes WHERE scope = ?), ?, ?);
            """)

        try:
            self.cursor.execute(sql_add_scope, (args.scope,))
            self.cursor.execute(sql_add_level, (args.level, args.scope, args.delay, args.execute,))
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
                DELETE FROM illuminance
                WHERE level=? AND scopeid=(SELECT id FROM scopes WHERE scope=?);
                """)
            self.cursor.execute(sql_del, (level, args.scope,));
        else:
            sql_del = ("""
                DELETE FROM scopes WHERE scope=?;
                """)
            self.cursor.execute(sql_del, (args.scope,));
        self.conn.commit()

    """ Get all actions for max illuminance level below
        specified one for all scopes
    """
    def run_actions(self, level):
        sql_get_actions = ("""
            SELECT a1.level, a1.scopeid, a1.action FROM illuminance a1
            INNER JOIN (
                SELECT MAX(level) AS max_level, scopeid
                FROM illuminance WHERE level <= ? GROUP BY scopeid
            ) a2
            ON a1.level = a2.max_level AND a1.scopeid = a2.scopeid;
            """)
        for level, scopeid, action in self.cursor.execute(sql_get_actions, (level,)):
            subprocess.run(action.split())

    """ Prints all action definitions from DB
    """
    def list_all(self):
        scope_prev = ""
        sql_list_all = ("""
            SELECT level, scopeid, scope, delay, action FROM illuminance, scopes
            WHERE illuminance.scopeid = scopes.id
            ORDER BY scopeid, level
            """)

        for level, scopeid, scope, delay, action in self.cursor.execute(sql_list_all):
            if scope == scope_prev:
                sys.stdout.write("{:10d} {}\n".format(level, action))
            else:
                sys.stdout.write("[{}:{}]\n".format(scopeid, scope))
                if delay > 0:
                   sys.stdout.write("{:10d} {} (After: {} sec)\n".format(level, action, delay))
                else:
                   sys.stdout.write("{:10d} {}\n".format(level, action))
                scope_prev = scope


cmd_help = {
            'run' : 'execute the command for actual lighting level',
            'list' : 'output a list of all planned actions',
            'add' : 'add a command to execute for a lighting level in a scope',
            'delete' : 'delete a planned action'
           }

parser = argparse.ArgumentParser(description='An illuminance level action')
sub_parsers = parser.add_subparsers(title="commands", dest="command")

for command in ["list", "run"]:
    p = sub_parsers.add_parser(command, help=cmd_help[command])
for command in ["add", "delete"]:
    p = sub_parsers.add_parser(command, help=cmd_help[command])
    p.add_argument("-s", "--scope", default="Default",
                   help="scope for row of actions")
    if (command == "add"):
        p.add_argument("-t", "--delay",
                       help="seconds to delay before command")
        p.add_argument("-l", "--level", required=True,
                       help="illuminance level for command")
        p.add_argument("-e", "--execute", required=True,
                       metavar="COMMAND",
                       help="command to execute")
    else:
        p.add_argument("-l", "--level",
                       help="illuminance level for command")

db_filename = '/var/db/bh1750/actions.sqlite'

if __name__ == "__main__":
    try:
        action = action_db(db_filename)
    except sqlite3.OperationalError as e:
        sys.stderr.write("Operation not permitted: %s\n" % e)
        exit(-1)

    light = illuminance()

    args = parser.parse_args()

    if args.command == "list":
        action.list_all()
    elif args.command == "run":
        level = light.get_level()
        action.run_actions(level)
    elif args.command == "delete":
        action.delete(args)
    elif args.command == "add":
        action.add(args)

    action.conn.close()
