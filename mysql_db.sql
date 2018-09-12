CREATE DATABASE IF NOT EXISTS geekchat;
USE geekchat;
CREATE TABLE IF NOT EXISTS users (
    `usrid` INT UNSIGNED KEY AUTO_INCREMENT,
    `username` VARCHAR(20) NOT NULL,
    `password` VARCHAR(40) NOT NULL,
    `age` INT,
    `submission_date` DATE,
    `sex` ENUM("man", "woman", "null")
) DEFAULT CHARSET=utf8;
INSERT INTO geekchat.users(usrid, username, password) VALUES ("1", "root", "root");
INSERT INTO geekchat.users(usrid, username, password) VALUES ("2", "zhangsan", "lisi");
INSERT INTO geekchat.users(usrid, username, password) VALUES ("3", "lisi", "lisi");
CREATE user geekchat@localhost IDENTIFIED BY '1234';
GRANT ALL ON geekchat.* TO 'geekchat'@'localhost';
