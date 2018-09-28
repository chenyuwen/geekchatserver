CREATE DATABASE IF NOT EXISTS geekchat;
USE geekchat;
CREATE TABLE IF NOT EXISTS users (
    `uid` INT UNSIGNED KEY AUTO_INCREMENT,
    `username` VARCHAR(20) NOT NULL,
    `password` VARCHAR(40) NOT NULL,
    `age` INT,
    `submission_timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `sex` ENUM("man", "woman", "null"),
    `token` VARCHAR(32),
    `token_timestamp` BIGINT UNSIGNED,
    `token_valid` TINYINT(1) DEFAULT "0",
    UNIQUE KEY `users_uni` (`username`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.users AUTO_INCREMENT = 1;
INSERT INTO geekchat.users(username, password) VALUES ("root", "root");
INSERT INTO geekchat.users(username, password) VALUES ("zhangsan", "zhangsan");
INSERT INTO geekchat.users(username, password) VALUES ("lisi", "lisi");
INSERT INTO geekchat.users(username, password) VALUES ("liming", "liming");

CREATE TABLE IF NOT EXISTS friends (
    `fid` INT UNSIGNED KEY AUTO_INCREMENT,
    `username` VARCHAR(20) NOT NULL,
    `create_timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `friend` VARCHAR(20) NOT NULL,
    UNIQUE KEY `friends_uni` (`username`, `friend`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.friends AUTO_INCREMENT = 1;
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "zhangsan");
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "lisi");
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "liming");
INSERT INTO geekchat.friends(username, friend) VALUES ("lisi", "zhangsan");
INSERT INTO geekchat.friends(username, friend) VALUES ("lisi", "liming");

CREATE TABLE IF NOT EXISTS messages (
    `msgid` INT UNSIGNED KEY AUTO_INCREMENT,
    `uuid` VARCHAR(32) NOT NULL,
    `from_user` VARCHAR(20) NOT NULL,
    `to_user` VARCHAR(20) NOT NULL,
    `have_read` TINYINT(1) NOT NULL DEFAULT 0,
    `send_timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `message` VARCHAR(200) NOT NULL,
    UNIQUE KEY `message_uni` (`uuid`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.messages AUTO_INCREMENT = 1;
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("8FBE5280E892C0DEECB1E9795769B89A", "zhangsan", "zhangsan", "Hi, zhangsan.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("C723765A8E89BBE7F50D351891036897", "lisi", "root", "Hi, root.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("47941EF88FC11655F6BB4A14D73923C5", "liming", "zhangsan", "Hi, zhangsan.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("17525C08363E8D3835588B269F10B3D8", "zhangsan", "zhangsan", "Hi, zhangsan");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("0D61B083790D0C2AAED71E6DE5B9D7B9", "liming", "lisi", "Hi, lisi");

CREATE user geekchat@localhost IDENTIFIED BY '1234';
GRANT ALL ON geekchat.* TO 'geekchat'@'localhost';
