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
INSERT INTO users (name, password) VALUES ('testuser', 'ff36ffffffffffffff0fff3cff32ffff794d4815ffff02ffff6fffff40ffff7d');
-- create user with name "testuser2" and password "password2"
INSERT INTO users (name, password) VALUES ('testuser2', 'ffffff3cffff151050ffff1cff1807ffff777cffffffff11ff67ff65ffff315c');

INSERT INTO acl_users (username, ng) VALUES ('testuser', 'alt.wendzelnntpd.test');
INSERT INTO acl_users (username, ng) VALUES ('testuser2', 'alt.wendzelnntpd.test.empty');
INSERT INTO acl_roles (role, ng) VALUES ('testrole', 'alt.wendzelnntpd.test.post');
INSERT INTO users2roles (username, role) VALUES ('testuser', 'testrole');
