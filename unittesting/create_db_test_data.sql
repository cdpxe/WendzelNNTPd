INSERT INTO newsgroups (id, name, pflag, high) VALUES (1, 'alt.wendzelnntpd.test', 'y', 2);
INSERT INTO newsgroups (id, name, pflag, high) VALUES (2, 'alt.wendzelnntpd.test.empty', 'y', 0);
INSERT INTO newsgroups (id, name, pflag, high) VALUES (3, 'alt.wendzelnntpd.test.post', 'y', 0);

INSERT INTO postings (msgid, date, author, newsgroups, subject, "lines", header)
VALUES ('<cdp1@localhost>', 1751106600, 'Testuser <test@example.com>', 'alt.wendzelnntpd.test',
        'Test', '1', 'Path: test
Message-ID: <cdp1@localhost>
Date: Sat, 28 Jun 2025 12:30:00 +0200
Lines: 1
X-WendzelNNTPdBodysize: 9
newsgroups: alt.wendzelnntpd.test
from: Testuser <test@example.com>
subject: Test
');

INSERT INTO postings (msgid, date, author, newsgroups, subject, "lines", header)
VALUES ('<cdp2@localhost>', 1751106601, 'Testuser <test@example.com>', 'alt.wendzelnntpd.test',
        'Test', '1', 'Path: test
Message-ID: <cdp2@localhost>
Date: Sat, 28 Jun 2025 12:30:01 +0200
Lines: 1
X-WendzelNNTPdBodysize: 10
newsgroups: alt.wendzelnntpd.test
from: Testuser <test@example.com>
subject: Test2
');

INSERT INTO ngposts (msgid, ng, postnum) VALUES ('<cdp1@localhost>', 'alt.wendzelnntpd.test', 1);
INSERT INTO ngposts (msgid, ng, postnum) VALUES ('<cdp2@localhost>', 'alt.wendzelnntpd.test', 2);

-- create user with name "testuser" and password "password"
INSERT INTO users (name, password) VALUES ('testuser', '1c039cb73df5bad8a0fe7711e2447c2b72c8d9ff8dc53d75d201aa9491fc1756');
-- create user with name "testuser2" and password "password2"
INSERT INTO users (name, password) VALUES ('testuser2', 'b6e30ea30cc1f9e3742e9dc98c7c6b5fb10c102dc325791535306821115c877a');

INSERT INTO roles (role) VALUES ('testrole');
INSERT INTO acl_users (username, ng) VALUES ('testuser', 'alt.wendzelnntpd.test');
INSERT INTO acl_users (username, ng) VALUES ('testuser2', 'alt.wendzelnntpd.test.empty');
INSERT INTO acl_roles (role, ng) VALUES ('testrole', 'alt.wendzelnntpd.test.post');
INSERT INTO users2roles (username, role) VALUES ('testuser', 'testrole');
