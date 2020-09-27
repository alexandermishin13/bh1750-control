BEGIN TRANSACTION;
CREATE TABLE scopes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    scope TEXT NOT NULL UNIQUE
);
INSERT INTO scopes VALUES(0,'Default');
CREATE TABLE illuminance (
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
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('scopes',0);
CREATE INDEX index_scope_level ON illuminance(scopeid, level);
COMMIT;
