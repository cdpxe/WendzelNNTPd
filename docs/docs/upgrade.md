# Upgrading

## Upgrade from version 2.1.y to 2.2.x

Please stop WendzelNNTPd and check the *wendzelnntpd.conf*. There is a
new configuration style that breaks parts of the previous configuration
style (especially due to the introduction of "connectors").
Additionally, the configuration file has been moved into the subdirectory
*wendzelnntpd*. So, for example, if the file was previously located at
*/usr/local/etc/wendzelnntpd.conf*, it must now be moved to
*/usr/local/etc/wendzelnntpd/wendzelnntpd.conf*.

Existing user passwords will no longer work. You need to recreate all
users and their passwords using **wendzelnntpadm**. Additionally, you
must reapply any previously configured ACL roles and group memberships
for the recreated users.

The behavior of `./configure` has changed. The environment variables
for enabling and disabling features are no longer supported and replaced
by CLI flags. The support for the databases can now be enabled/disabled.
Examples: *\--disable-mysql*, *\--enable-sqlite*, *\--enable-postgres*. The
support for TLS can be disabled by *\--disable-tls*. You can use
`./configure --help` for an overview of the available CLI flags.

## Upgrade from version 2.1.x to 2.1.y

Same as upgrading from v.2.0.x to v.2.0.y, see Section
[Upgrade from version 2.0.x to 2.0.y](#upgrade-from-version-20x-to-20y).

## Upgrade from version 2.0.x to 2.1.y

Please follow the upgrade instructions for upgrading from 2.0.x to 2.0.y
below. However, once you use cryptographic hashes in your
*wendzelnntpd.conf*, your previous passwords will not work anymore,
i.e., you need to reset all passwords or deactivate the hashing feature.

## Upgrade from version 2.0.x to 2.0.y

Stop WendzelNNTPd if it is currently running. Install WendzelNNTPd as
described but run `make upgrade` instead of `make install`.
Afterwards, start WendzelNNTPd again.

## Upgrade from version 1.4.x to 2.0.x

**Acknowledgement:** I would like to thank Ann from Href.com for helping
a lot with finding out how to upgrade from 1.4.x to 2.0.x!

An upgrade from version 1.4.x was not foreseen due to the limited
available time I have for the development. However, here is a dirty
hack:

##### Preparation Step:

You **need to create a backup of your existing installation first**, at
least everything from **/var/spool/news/wendzelnntpd** as you will need
all these files later. **Perform the following steps on your own risk --
it is possible that they do not work on your system as only two
WendzelNNTPd installations were tested!**

##### First Step:

Install Wendzelnntpd-2.x on a Linux system (Windows is not supported
anymore). This requires some libraries and tools.

Under *Ubuntu* they all come as packages:
```console
$ sudo apt-get install libmysqlclient-dev libsqlite3-dev flex bison sqlite3
```

Under *CentOS* they come as packages as well:
```console
$ sudo yum install make gcc bison flex sqlite-devel
```

*Other* operating systems should provide the same or similar
packages/ports.

Run `MYSQL=NO ./configure`, followed by `make`, and `sudo make install`.
This will compile, build and install WendzelNNTPd without
MySQL support as you only rely on SQLite3 from v.1.4.x (and it would be
significantly more difficult to bring the SQLite database content to a
MySQL database).

##### Second Step:

Please make sure WendzelNNTPd-2 is configured in a way that we can
\*hopefully\* make it work with your own database file. Therefore, in
the configuration file **/usr/local/etc/wendzelnntpd.conf** make sure
that WendzelNNTPd uses sqlite3 instead of mysql:
```init
database-engine sqlite3
```

##### Third Step:

Now comes the tricky part. The install command should have created
**/var/spool/news/wendzelnntpd/usenet.db**. However, it is an empty
Usenet database file in the new format. Now REPLACE that file with the
file you use on your existing WendzelNNTPd installation, which uses the
old 1.4.x format. Also copy all of your old **cdp\*** files and the old
**nextmsgid** file from your Windows system/from your backup directory
to **/var/spool/news/wendzelnntpd/**.

The following step is a very dirty hack but I hope it works for you. It
is not 100% perfect as important table columns are then still of the
type 'STRING' instead of the type 'TEXT'!

Load the sqlite3 tool with your replaced database file:
```console
$ sudo sqlite3 /var/spool/news/wendzelnntpd/usenet.db
```

This will open the new file in editing mode. We now add the tables which
are not part of v.1.4.x to your existing database file. Therefore enter
the following commands:
```sql
CREATE TABLE roles (role TEXT PRIMARY KEY);
CREATE TABLE users2roles (username TEXT, role TEXT, PRIMARY KEY(username, role));
CREATE TABLE acl_users (username TEXT, ng TEXT, PRIMARY KEY(username, ng));
CREATE TABLE acl_roles (role TEXT, ng TEXT, PRIMARY KEY(role, ng));
.quit
```

##### Fix Postings

You will probably see no post bodies right now if posts are requested by
your client. Therefore, switch into **/var/spool/news/wendzelnntpd** and
run (as superuser) this command, it will replace the broken trailings
with corrected ones:
```console
$ cd /var/spool/news/wendzelnntpd
$ for filn in `/bin/ls cdp*`; do echo $filn; cat $filn | \
sed 's/\.\r/.\r\n/' > new;  num=`wc -l new| \
awk '{$minone=$1-1; print $minone}'` ; \
head -n $num new > $filn; done
```

##### Last Step (Checking whether it works!):

First check, whether the database file is accepted at all:
```console
$ sudo wendzelnntpadm listgroups
```

It should list all your newsgroups
```console
$ sudo wendzelnntpadm listusers
```

It should list all existing users. Accordingly
```console
$ sudo wendzelnntpadm listacl
```

should list all access control entries (which will be empty right now
but if no error message appears, the related tables are now part of your
database file!).

Now start WendzelNNTPd via `sudo wendzelnntpd` and try to connect with
a NNTP client to your WendzelNNTPd and then try reading posts, sending
new posts and replying to these posts.

If this works, you can now run v2.x on 32bit and 64bit Linux :)
Congrats, you made it and chances are you are the second user who did
that. Let me know via e-mail if it worked for you.
