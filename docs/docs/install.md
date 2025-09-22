# Installation

This chapter provides a guide on how to install WendzelNNTPd 2.x.

## Linux/*nix/BSD

To install WendzelNNTPd from source you can either [download the provided
archive file of a stable version (e.g., *v-2.x.y.tar.gz*)](https://sourceforge.net/projects/wendzelnntpd/files/) and extract it or you can [clone the current WendzelNNTPd development repository](https://github.com/cdpxe/WendzelNNTPd).
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

**Please Note:** *For FreeBSD/OpenBSD/NetBSD/\*BSD: There is no MySQL
support; you need to use SQLite (i.e., run
`./configure --disable-mysql`). Run `configure` as well as `make` in the
`bash` shell (under some BSDs you first need to install `bash`).*

**Please Note:** *If you wish to compile WITHOUT TLS support*, then run
`./configure --disable-tls`.

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

### Init Script for Automatic Startup

There is an init script in the directory scripts/startup. It uses the
usual parameters like "start", "stop" and "restart".

## Unofficial Note: Mac OS X

A user reported WendzelNNTPd-2.0.0 is installable under Mac OS X 10.5.8.
The only necessary change was to add the flag `-arch x86_64` to
compile the code on a 64 bit system. However, I never tried to compile
WendzelNNTPd on a Mac.

## Windows

Not supported.

[^1]: On some \*nix-like operating systems you need to first run
    `gzip -d wendzelnntpd-VERSION.tgz` and then 
    `tar -xf wendzelnntpd-VERSION.tar` instead of letting `tar` do the whole
    job.
