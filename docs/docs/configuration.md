# Basic Configuration

This chapter will explain how to configure WendzelNNTPd after
installation.

**Note:** The configuration file for WendzelNNTPd is named
*/usr/local/etc/wendzelnntpd/wendzelnntpd.conf*. The format of the configuration file
should be self-explanatory and the default configuration file includes
many comments which will help you to understand its content.

**Note:** On \*nix-like operating systems the default installation path
is */usr/local/\**, i.e., the configuration file of WendzelNNTPd will be
*/usr/local/etc/wendzelnntpd/wendzelnntpd.conf*, and the binaries will be placed in
*/usr/local/sbin*.

## Choosing a database engine

The first and most important step is to choose a database engine. You
can use either SQLite3 (this is the default case and easy to use, but
not suitable for larger systems with many thousand postings or users) or
MySQL (which is the more advanced solution, but also a little bit more
complicated to realize). By default, WendzelNNTPd is configured for
SQLite3 and is ready to run. If you want to keep this setting, you do
not have to read this section.

### Modifying wendzelnntpd.conf

In the configuration file you will find a parameter called
**database-engine**. You can choose to use either MySQL or SQLite as the
backend storage system by appending either **sqlite** or **mysql**.
Experimental support for PostgreSQL can be activiated with **postgres**.
```ini
database-engine mysql
```
If you choose to use MySQL then you will also need to specify the user
and password which WendzelNNTPd must use to connect to the MySQL server.
If your server does not run on localhost or uses a non-default MySQL
port then you will have to modify these values too.
```ini
; Your database hostname (not needed for sqlite3)
database-server 127.0.0.1

; the database connection port (not needed for sqlite3)
; Comment out to use the default port of your database engine
database-port 3306

; Server authentication (not needed for sqlite3)
database-username mysqluser
database-password supercoolpass
```

### Generating your database tables

Once you have chosen your database backend you will need to create the
database and the required tables.

#### SQLite

If you chose SQLite as your database backend then you can skip this step
as running `make install` does this for you.

**Note:** The SQLite database file as well as the posting management
files will be stored in */var/spool/news/wendzelnntpd/*.

#### MySQL

For MySQL, an SQL script file called *mysql_db_struct.sql* is included.
It creates the WendzelNNTPd database and all the needed tables. Use the
MySQL console tool to execute the script.
```console
$ cd /path/to/your/extracted/wendzelnntpd-archive/
$ mysql -u YOUR-USER -p
Enter password:
Welcome to the MySQL monitor.  Commands end with ; or \g.
Your MySQL connection id is 48
Server version: 5.1.37-1ubuntu5.1 (Ubuntu)

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

mysql> source mysql\_db\_struct.sql
...
mysql> quit
Bye
```

#### PostgreSQL

Similarly to MySQL, there is a SQL script file
(*postgres_db_struct.sql*) to create the WendzelNNTPd database. Create
and setup a new database (and a corresponding user) and use the
`psql(1)` command line client to load table and function definitions:
```console
$ psql --username USER -W wendzelnntpd
wendzelnntpd=> begin;
wendzelnntpd=> \i database/postgres_db_struct.sql
wendzelnntpd=> commit; quit;
```

## Network Settings

For each type of IP address (IPv4 and/or IPv6) you have to define a own
connector. You can find an example for NNTP over port 119 below.
```ini
<connector>
    ;; enables STARTTLS for this port
    ;enable-starttls
    port        119
    listen      127.0.0.1
    ;; configure SSL server certificate (required)
    ;tls-server-certificate "/usr/local/etc/wendzelnntpd/ssl/server.crt"
    ;; configure SSL private key (required)
    ;tls-server-key "/usr/local/etc/wendzelnntpd/ssl/server.key"
    ;; configure SSL CA certificate (required)
    ;tls-ca-certificate "/usr/local/etc/wendzelnntpd/ssl/ca.crt"
    ;; configure TLS ciphers for TLSv1.3
    ;tls-cipher-suites "TLS_AES_128_GCM_SHA256"
    ;; configure TLS ciphers for TLSv1.1 and TLSv1.2
    ;tls-ciphers "ALL:!COMPLEMENTOFDEFAULT:!eNULL"
    ;; configure allowed TLS version (1.0-1.3)
    ;tls-version "1.2-1.3"
    ;; possibility to force the client to authenticate 
    ;;with client certificate (none | optional | require)
    ;tls-verify-client "required"
    ;; define depth for checking client certificate
    ;tls-verify-client-depth 0
    ;; possibility to use certificate revocation list (none | leaf | chain)
    ;tls-crl "none"
    ;tls-crl-file "/usr/local/etc/wendzelnntpd/ssl/ssl.crl"
</connector>
```

To use dedicated TLS with NNTP (SNNTP) you can define another connector.
The example below is for SNNTP over port 563.
```ini
<connector>
    ;; enables TLS for this port
    ;enable-tls
    port        563
    listen      127.0.0.1
    ;; configure SSL server certificate (required)
    ;tls-server-certificate "/usr/local/etc/wendzelnntpd/ssl/server.crt"
    ;; configure SSL private key (required)
    ;tls-server-key "/usr/local/etc/wendzelnntpd/ssl/server.key"
    ;; configure SSL CA certificate (required)
    ;tls-ca-certificate "/usr/local/etc/wendzelnntpd/ssl/ca.crt"
    ;; configure TLS ciphers for TLSv1.3
    ;tls-cipher-suites "TLS_AES_128_GCM_SHA256"
    ;; configure TLS ciphers for TLSv1.1 and TLSv1.2
    ;tls-ciphers "ALL:!COMPLEMENTOFDEFAULT:!eNULL"
    ;; configure allowed TLS version (1.0-1.3)
    ;tls-version "1.2-1.3"
    ;; possibility to force the client to authenticate 
    ;;with client certificate (none | optional | require)
    ;tls-verify-client "required"
    ;; define depth for checking client certificate
    ;tls-verify-client-depth 0
    ;; possibility to use certificate revocation list (none | leaf | chain)
    ;tls-crl "none"
    ;tls-crl-file "/usr/local/etc/wendzelnntpd/ssl/ssl.crl"
</connector>
```

The configuration options *tls-server-certificate*, *tls-server-key* and
*tls-ca-certificate* are required for using TLS or STARTTLS with NNTP.
All other TLS-related options are optional. More examples are in the
existing *wendzelnntpd.conf* file.

## Setting the Allowed Size of Postings

To change the maximum size of a post to be accepted by the server,
change the variable **max-size-of-postings**. The value must be set in
Bytes and the default value is 20971520 (20 MBytes).
```ini
max-size-of-postings 20971520
```

## Verbose Mode

If you have any problems running WendzelNNTPd or if you simply want more
information about what is happening, you can uncomment the
**verbose-mode** line.
```ini
; Uncomment 'verbose-mode' if you want to find errors or if you
; have problems with the logging subsystem. All log strings are
; written to stderr too, if verbose-mode is set. Additionally all
; commands sent by clients are written to stderr too (but not to
; logfile)
verbose-mode
```

## Security Settings

### Authentication and Access Control Lists (ACL)

WendzelNNTPd contains an extensive access control subsystem. If you want
to only allow authenticated users to access the server, you should
uncomment **use-authentication**. This gives every authenticated user
access to each newsgroup.
```ini
; Activate authentication
use-authentication
```

If you need a slightly more advanced authentication system, you can
activate Access Control Lists (ACL) by uncommenting **use-acl**. This
activates the support for Role-based ACL too.
```ini
; If you activated authentication, you can also activate access
; control lists (ACL)
use-acl
```

### Anonymized Message-ID

By default, WendzelNNTPd makes a user's hostname or IP address part of
new message IDs when a user sends a post using the NNTP POST command. If
you do not want that, you can force WendzelNNTPd not to do so by
uncommenting **enable-anonym-mids**, which enables anonymized message
IDs.
```ini
; This prevents that IPs or Hostnames will become part of the
; message ID generated by WendzelNNTPd what is the default case.
; Uncomment it to enable this feature.
enable-anonym-mids
```

### Changing the Default Salt for Password Hashing

When uncommenting the keyword **hash-salt**, the default salt value that
is used to enrich the password hashes can be changed. Please note that
you have to define the salt *before* you set-up the first password since
it will otherwise be stored hashed using an old salt, rendering it
unusable. For this reason, it is necessary to define your salt right
after running **make install** (or at least before the first creation of
NNTP user accounts).
```ini
; This keyword defines a salt to be used in conjunction with the
; passwords to calculate the cryptographic hashes. The salt must
; be in the form [a-zA-Z0-9.:\/-_]+.
; ATTENTION: If you change the salt after passwords have been
; stored, they will be rendered invalid! If you comment out
; hash-salt, then the default hash salt defined in the source
; code will be used.
hash-salt 0.hG4//3baA-::_\
```

WendzelNNTPd applies the SHA-2 hash algorithm using a 256 bit hash
value. Please also note that the final hash is calculated using a string
that combines salt, username and password as an input to prevent
password-identification attacks when an equal password is used by
multiple users. However, utilizing the username is less secure than
having a completely separate salt for every password.[^2]

### Encrypted communication (TLS)

Please look at section [Network-Settings](#network-settings) when you want
to use encryption over TLS.

[^2]: Patches are appreciated!
