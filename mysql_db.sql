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
    `send_timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    `message` VARCHAR(200) NOT NULL,
    UNIQUE KEY `message_uni` (`uuid`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.messages AUTO_INCREMENT = 1;
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("1", "zhangsan", "zhangsan", "Hi, zhangsan.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("2", "lisi", "root", "Hi, root.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("3", "liming", "zhangsan", "Hi, zhangsan.");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("4", "zhangsan", "zhangsan", "Hi, zhangsan");
INSERT INTO geekchat.messages(uuid, from_user, to_user, message) VALUES ("5", "liming", "lisi", "Hi, lisi");

CREATE user geekchat@localhost IDENTIFIED BY '1234';
GRANT ALL ON geekchat.* TO 'geekchat'@'localhost';
