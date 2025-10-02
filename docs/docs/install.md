# Installation

This chapter provides a guide on how to install WendzelNNTPd 2.x.

## Linux/*nix/BSD from source

To install WendzelNNTPd from source you can either [download the provided
archive file of a stable version (e.g., *v-2.x.y.tar.gz*)](https://sourceforge.net/projects/wendzelnntpd/files/) and extract it[^1] or you can [clone the current WendzelNNTPd development repository](https://github.com/cdpxe/WendzelNNTPd).
Afterwards, run `./configure`. Please note that configure indicates missing
libraries and packages that you may first need to install using the
package system of your operating system.
```console
# 1. Unpack the content of the tarball (or: use git clone).
$ tar -xzf v2.2.0.tar.gz

# 2. Switch into the unpacked directory (the name of the
#    directory could be different on your system, e.g.,
#    cdpxe-WendzelNNTPd-17d557d.
$ cd wendzelnntpd

# 3. Run the configure script:
$ ./configure
...
```

**Please Note:** *If you wish to compile WITHOUT MySQL or WITHOUT SQLite
support*, then run `./configure --disable-mysql` or `./configure --disable-sqlite`,
respectively.

**Please Note:** Run `configure` as well as `make` in the
`bash` shell (under some BSDs you first need to install `bash`).*

**Please Note:** *If you wish to compile WITHOUT TLS support*, then run
`./configure --disable-tls`.

##### Dependencies

WendzelNNTPd depends on some programs and libraries.
`./configure` will inform you about missing dependencies.
Here is a list of packages for some distributions which provide the dependencies.
The list may omit some packages which are already installed by default.
Dependencies for the experimental PostgreSQL support are in brackets:

- Debian/Ubuntu: gcc flex bison sqlite3 libsqlite3-dev libmariadb-dev-compat ca-certificates
  libmariadb-dev libmhash-dev make openssl libssl-dev (libpq-dev)
- Fedora: gcc flex bison sqlite sqlite-devel mariadb-connector-c-devel ca-certificates mhash-devel make openssl
  openssl-devel (libpq-devel)
- openSUSE Leap: gcc flex bison sqlite3 sqlite3-devel libmariadb-devel ca-certificates mhash-devel make openssl
  libopenssl-devel (postgresql-devel)
- Arch Linux: gcc flex bison sqlite mariadb-libs ca-certificates mhash make openssl (postgresql-libs)
- FreeBSD 14: bash sqlite3 bison mhash mariadb114-client (postgresql17-client)
- OpenBSD: bash bison mhash mariadb-client (postgresql-client)
- NetBSD 10: bash bison mhash mariadb-client (postgresql17-client)

##### Compiling WendzelNNTPd

After `configure` has finished, run `make`:
```console
$ make
...
```

##### Installing WendzelNNTPd

To install WendzelNNTPd on your system, you need superuser access. Run
`make install` to install it to the default location */usr/local/\**.
```console
$ sudo make install
```

**Please Note (Upgrades):** Run `sudo make upgrade` instead of `sudo make install`
for an upgrade. Please cf. Section [Upgrading](upgrade.md#upgrading).

**Please Note (MySQL):** *If you plan to run MySQL*, then no database
was set up during `make install`. Please refer to
Section [Basic Configuration](configuration.md#basic-configuration) to learn how to generate
the MySQL database.

##### Generating SSL certificates

TLS is enabled by default in *wendzelnntpd.conf* as long as WendzelNNTPd has
not been compiled without TLS support. `make install` generates a self-signed
certificate for usage on localhost so that TLS can be used out-of-the-box.

If you want to generate an SSL certificate, which is signed by Let's Encrypt,
or a new self-signed certificate, you can use the helper script `create_certificate`:
```console
$ sudo create_certificate \
    --environment letsencrypt \
    --email <YOUR-EMAIL> \
    --domain <YOUR-DOMAIN>
```
For the parameter `--environment`, "*local*" is also a valid value. In
that case, the certificate is generated only for usage on localhost and
is self-signed. The location of the generated certificates can be adjusted
with the parameter `--targetdir`. You also need to adjust the paths in
*wendzelnntpd.conf* if you use a non-default location
(check Section [Encrypted connections over TLS](configuration.md#encrypted-connections-over-tls)).

### Automatic startup

There is an init script and a systemd service unit in the directory *scripts/startup* for automatic
startup of `wendzelnntpd`. More information can be found in
[Automating Start, Stop, and Restart](running.md#automating-start-stop-and-restart)

## Packages for Linux and BSD

WendzelNNTPd is available as a package for some Linux and BSD distributions.
You can use them to install WendzelNNTPd instead of installing it from source.
Please consult the documentation of you distribution for further information about package installation.
Here is a list of known packages:

- *Slackware Linux*:
    - Slackware: Slackbuilds.org Build Script [Slackware 14.2](https://slackbuilds.org/repository/14.2/network/wendzelnntpd/), [Slackware 15.0](https://slackbuilds.org/repository/15.0/network/wendzelnntpd/?search=wendzelnntpd)
    - Slackware64-current: [Slackware package](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.3/slackware64-current-package/) (Installation via `installpkg (filename)`)
- *NetBSD*: [WendzelNNTPd port at pkgsrc](https://pkgsrc.se/wip/wendzelnntpd)

## Docker image for Linux

WendzelNNTPd is available as a [Docker image on Docker Hub](https://hub.docker.com/r/cdpxe/wendzelnntpd).

##### Run WendzelNNTPd in a Docker container

```console
$ docker run --name wendzelnntpd -d -p 119:119 -p 563:563 cdpxe/wendzelnntpd
```

##### Specify volumes for the database and configuration files

```console
$ docker run --name my-wendzelnntpd -d -p 119:119 -p 563:563 \
    -v wendzelnntpd_config:/usr/local/etc/wendzelnntpd \
    -v wendzelnntpd_data:/var/spool/news/wendzelnntpd \
    cdpxe/wendzelnntpd
```

##### Administration with wendzelnntpadm

```console
$ docker run --rm -v wendzelnntpd_config:/usr/local/etc/wendzelnntpd \
    -v wendzelnntpd_data:/var/spool/news/wendzelnntpd \
     cdpxe/wendzelnntpd wendzelnntpadm
```
Find further information regarding the administration of WendzelNNTPd in [Running](running.md#administration-tool-wendzelnntpadm).

##### Create new certificates

```console
$ docker run --rm -v wendzelnntpd_config:/usr/local/etc/wendzelnntpd \
    cdpxe/wendzelnntpd create_certificate
```
Finy further information in [Generating SSL certificates](#generating-ssl-certificates).

##### Get the configuration file

Copy the default configuration file from the image to the host:
```console
$ docker run --rm --entrypoint=cat cdpxe/wendzelnntpd \
    /usr/local/etc/wendzelnntpd/wendzelnntpd.conf \
    > /home/youruser/wendzelnntpd.conf
```

Or copy the current configuration file from your container:
```console
$ docker cp my-wendzelnntpd:/usr/local/etc/wendzelnntpd/wendzelnntpd.conf \
    /home/youruser/wendzelnntpd.conf 
```

##### Edit the configuration file

Find further information regarding the configuration of WendzelNNTPd in [Configuration](configuration.md#basic-configuration).

##### Provide the configuration file to the container

Copy the configuration file back to the container:
```console
$ docker cp /home/youruser/wendzelnntpd.conf \
    my-wendzelnntpd:/usr/local/etc/wendzelnntpd/wendzelnntpd.conf
```

Or bind mount the configuration file (the file needs to be owned by root in this case):
```console
$ sudo chown 0:0 /home/youruser/wendzelnntpd.conf
$ docker run --name wendzelnntpd -d -p 119:119 -p 563:563 \
    -v wendzelnntpd_config:/usr/local/etc/wendzelnntpd \
    -v wendzelnntpd_data:/var/spool/news/wendzelnntpd \
    -v /home/youruser/wendzelnntpd.conf:\
/usr/local/etc/wendzelnntpd/wendzelnntpd.conf:ro \
     -d cdpxe/wendzelnntpd
```

Or build a new image with your configuration file:
```dockerfile
FROM cdpxe/wendzelnntpd
COPY wendzelnntpd.conf /usr/local/etc/wendzelnntpd/wendzelnntpd.conf
```

##### Create the Docker image from source

You can also build the Docker image from source instead of using the pre-built image from Docker Hub.
Therefore, you need to get the source code of WendzelNNTPd like explained in [Linux/*nix/BSD from source](#linuxnixbsd-from-source).
After that you can build the image with `make`:
```console
$ make docker-build
```

## Unofficial Note: Mac OS X

A user reported WendzelNNTPd-2.0.0 is installable under Mac OS X 10.5.8.
The only necessary change was to add the flag `-arch x86_64` to
compile the code on a 64 bit system. However, I never tried to compile
WendzelNNTPd on a Mac.

## Windows

Not supported.

## Installed files

This documentation assumes that you have installed WendzelNNTPd with
default paths to `/usr/local`. The installation prefix as well as the
path for the subdirectories like `sbin`  or `share/man/man8` can be changed
when calling `./configure` or `make install`. Please consult 
`./configure --help` for a list of available options.
Here is an overview of the installed files as well as files which
are created during runtime:

[The spacing for the first column in the table is intentional to make the column big enough for the paths]: #
[The columns would overlap in the PDF output without it]: #

| Path                                                                                                                                          | Description                                                                                            |
|-----------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------|
| /usr/local/etc/wendzelnntpd/wendzelnntpd.conf                                                                                                 | [Cofiguration file](configuration.md#basic-configuration)                                              |
| /usr/local/etc/wendzelnntpd/ssl/*                                                                                                             | SSL certificates for [encrypted connections over TLS](configuration.md#encrypted-connections-over-tls) |
| /usr/local/sbin/create_certificate                                                                                                            | Script for [generating SSL certificates](#generating-ssl-certificates)                                 |
| /usr/local/sbin/wendzelnntpadm                                                                                                                | [Administration tool](running.md#administration-tool-wendzelnntpadm)                                   |
| /usr/local/sbin/wendzelnntpd                                                                                                                  | The Usenet server                                                                                      |
| /usr/local/share/doc/wendzelnntpd/*                                                                                                           | Various documentation files                                                                            |
| /usr/local/share/man/man5/*                                                                                                                   | Man pages for configuration files                                                                      |
| /usr/local/share/man/man8/*                                                                                                                   | Man pages for commands                                                                                 |
| /usr/local/share/wendzelnntpd/mysql_db_struct.sql                                                                                             | SQL file to [create the database schema of the MySQL database](configuration.md#mysql)                 |
| /usr/local/share/wendzelnntpd/openssl.cnf                                                                                                     | openssl config for create_certificate                                                                  |
| /usr/local/share/wendzelnntpd/usenet.db_struct                                                                                                | SQL file to [create the database schema of the SQLite database](configuration.md#sqlite)               |
| /var/log/wendzelnntpd                                                                                                                         | Logfile                                                                                                |
| /var/spool/news/wendzelnntpd/cdp*                                                                                                             | Message bodies of the postings                                                                         |
| /var/spool/news/wendzelnntpd/nextmsgid                                                                                                        | Next unique message id                                                                                 |
| /var/spool/news/wendzelnntpd/usenet.db                                                                                                        | SQLite database                                                                                        |


[^1]: On some \*nix-like operating systems you need to first run
    `gzip -d wendzelnntpd-VERSION.tgz` and then 
    `tar -xf wendzelnntpd-VERSION.tar` instead of letting `tar` do the whole
    job as in the listing.
