-- MySQL Table Creation Script

CREATE DATABASE WendzelNNTPd;
USE WendzelNNTPd;


-- MsgID-Len is always 196 here
-- NG-Name-Len is always 196

CREATE TABLE newsgroups (
   `id` INTEGER NOT NULL AUTO_INCREMENT,
   `name` VARCHAR(196),
   `pflag` CHARACTER,
   `high` INTEGER,
   INDEX(`name`),
   PRIMARY KEY(`id`)
 ) ENGINE=INNODB;

CREATE TABLE postings (
   `msgid` VARCHAR(196),
   `date` INTEGER,
   `author` VARCHAR(196),
   `newsgroups` VARCHAR(2048),
   `subject` VARCHAR(2048),
   `lines` VARCHAR(10),
   `header` VARCHAR(20000),
   INDEX(`msgid`),
   PRIMARY KEY(`msgid`)
 ) ENGINE=INNODB;

-- Would be better to use ng's ID here instead of their name
-- but I will keep this firstly. Will change it later.
-- I guess that MySQL will handle it very well nevertheless.
CREATE TABLE ngposts (
   `msgid` VARCHAR(196),
   `ng` VARCHAR(196),
   `postnum` INTEGER,
   PRIMARY KEY(`msgid`, `ng`),
   INDEX `I_msgid` (`msgid`),
   INDEX `I_ng` (`ng`),
   FOREIGN KEY `FK_msgid` (`msgid`) REFERENCES `postings` (`msgid`) ON DELETE CASCADE,
   FOREIGN KEY `FK_ng` (`ng`) REFERENCES `newsgroups` (`name`) ON DELETE CASCADE
 ) ENGINE=INNODB;

-- Authentification (as well as ACL)
CREATE TABLE users (
   `name` VARCHAR(50),
   `password` VARCHAR(50),
   INDEX(`name`),
   PRIMARY KEY(`name`)
) ENGINE=INNODB;

--
-- This is all needed for ACL
--

-- Define roles
CREATE TABLE roles (
   `role` VARCHAR(50),
   PRIMARY KEY(`role`)
 ) ENGINE=INNODB;

-- User 2 Roles
CREATE TABLE users2roles (
   `username` VARCHAR(50),
   `role` VARCHAR(50),
   PRIMARY KEY(`username`, `role`),
   INDEX(`username`),
   INDEX(`role`),
   FOREIGN KEY `FK_usr` (`username`) REFERENCES `users` (`name`) ON DELETE CASCADE,
   FOREIGN KEY `FK_rle` (`role`) REFERENCES `roles` (`role`) ON DELETE CASCADE
 ) ENGINE=INNODB;

-- User 2 Newsgroup
CREATE TABLE acl_users (
   `username` VARCHAR(50),
   `ng` VARCHAR(196),
   PRIMARY KEY(`username`, `ng`),
   INDEX(`username`),
   INDEX(`ng`),
   FOREIGN KEY `FK_usr` (`username`) REFERENCES `users` (`name`) ON DELETE CASCADE,
   FOREIGN KEY `FK_ng` (`ng`) REFERENCES `newsgroups` (`name`) ON DELETE CASCADE
 ) ENGINE=INNODB;

-- Roles 2 Newsgroup
CREATE TABLE acl_roles (
   `role` VARCHAR(50),
   `ng` VARCHAR(196),
   INDEX(`role`),
   INDEX(`ng`),
   PRIMARY KEY(`role`, `ng`),
   FOREIGN KEY `FK_ng` (`ng`) REFERENCES `newsgroups` (`name`) ON DELETE CASCADE,
   FOREIGN KEY `FK_rle` (`role`) REFERENCES `roles` (`role`) ON DELETE CASCADE
 ) ENGINE=INNODB;

-- Check:
-- SELECT * FROM users2roles ur,acl_roles aclr WHERE ur.username='swendzel' AND ur.role=aclr.role AND aclr.ng='alt.katze';
