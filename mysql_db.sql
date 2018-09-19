CREATE DATABASE IF NOT EXISTS geekchat;
USE geekchat;
CREATE TABLE IF NOT EXISTS users (
    `uid` INT UNSIGNED KEY AUTO_INCREMENT,
    `username` VARCHAR(20) NOT NULL,
    `password` VARCHAR(40) NOT NULL,
    `age` INT,
    `submission_date` DATE,
    `sex` ENUM("man", "woman", "null"),
    UNIQUE KEY `catename` (`username`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.users AUTO_INCREMENT = 1;
INSERT INTO geekchat.users(username, password) VALUES ("root", "root");
INSERT INTO geekchat.users(username, password) VALUES ("zhangsan", "zhangsan");
INSERT INTO geekchat.users(username, password) VALUES ("lisi", "lisi");
INSERT INTO geekchat.users(username, password) VALUES ("liming", "liming");

CREATE TABLE IF NOT EXISTS friends (
    `fid` INT UNSIGNED KEY AUTO_INCREMENT,
    `username` VARCHAR(20) NOT NULL,
    `friend` VARCHAR(20) NOT NULL,
    UNIQUE KEY `catename` (`username`, `friend`)
) DEFAULT CHARSET=utf8;
ALTER TABLE geekchat.friends AUTO_INCREMENT = 1;
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "zhangsan");
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "lisi");
INSERT INTO geekchat.friends(username, friend) VALUES ("root", "liming");
INSERT INTO geekchat.friends(username, friend) VALUES ("lisi", "zhangsan");
INSERT INTO geekchat.friends(username, friend) VALUES ("lisi", "liming");

CREATE user geekchat@localhost IDENTIFIED BY '1234';
GRANT ALL ON geekchat.* TO 'geekchat'@'localhost';
