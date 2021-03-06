CREATE TABLE users
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    user        VARCHAR(100),
    online	boolean NOT NULL,
    pid		INTEGER NOT NULL,
    UNIQUE(user),
    UNIQUE(pid)
);

CREATE TABLE dirs
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    dirname     VARCHAR(100) NOT NULL,
    UNIQUE(dirname)
);

-- Should be use with fk but:
-- http://www.sqlite.org/cvstrac/wiki?p=ForeignKeyTriggers
-- http://www.sqlite.org/cvstrac/fileview?f=sqlite/tool/genfkey.c
CREATE TABLE users_dir
(
    user_id     INTEGER NOT NULL,
    dir_id      INTEGER NOT NULL,
    PRIMARY KEY(user_id, dir_id)
);

CREATE TABLE logs
(
    user        VARCHAR(100) NOT NULL,
    action      VARCHAR(100) NOT NULL,
    fileP	VARCHAR(100) NOT NULL,
    timestamp   DATE
);



