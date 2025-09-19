# Installation

This chapter provides a guide on how to install WendzelNNTPd 2.x.

## Linux/\*nix/BSD

To install WendzelNNTPd from source you need to download the provided
archive file (e.g., *wendzelnntpd-2.0.0.tar.gz*) file.[^1] Extract it
and run **./configure**. Please note that configure indicates missing
libraries and packages that you may first need to install using the
package system of your operating system.

    $ tar -xzf wendzelnntpd-2.0.0.tgz
    $ cd wendzelnntpd
    $ ./configure
    ...

**Please Note:** *If you wish to compile WITHOUT MySQL or WITHOUT SQlite
support*, then run **./configure \--disable-mysql** or **./configure
\--disable-sqlite**, respectively.

 

**Please Note:** *For FreeBSD/OpenBSD/NetBSD/\*BSD: There is no MySQL
support; you need to use SQLite (i.e., run
`./configure --disable-mysql`). Run `configure` as well as `make` in the
`bash` shell (under some BSDs you first need to install `bash`).*

 

**Please Note:** *If you wish to compile WITHOUT TLS support*, then run
**./configure \--disable-tls**.

 

After `configure` has finished, run **make**:

    $ make
    ...

##### Generating SSL certifiates {#generating-ssl-certifiates .unnumbered}

If you want to generate SSL certificates you can use the helper script:

    $ sudo ./create_certificate \
        --environment letsencrypt \
        --email <YOUR-EMAIL> \\
        --domain <YOUR-DOMAIN>

For the parameter `--environment`, "*local*" is also a valid value. In
that case, the certificate is generated only for usage on localhost and
is self-signed. After generating the certificate you have to adjust
*wendzelnntpd.conf* (check
Section [\[network-settings\]](#network-settings){reference-type="ref"
reference="network-settings"}) to activate TLS (configuration option
*enable-tls*)). The paths for certificate and server key can stay as
they are.

 

##### Installing WendzelNNTPd {#installing-wendzelnntpd .unnumbered}

To install WendzelNNTPd on your system, you need superuser access. Run
**make install** to install it to the default location */usr/local/\**.

    $ sudo make install

**Please Note (Upgrades):** Run **sudo make upgrade** instead of **sudo
make install** for an upgrade. Please
cf. Section [\[Ch:Upgrade\]](#Ch:Upgrade){reference-type="ref"
reference="Ch:Upgrade"} (Upgrading).

**Please Note (MySQL):** *If you plan to run MySQL*, then no database
was set up during 'make install'. Please refer to
Section [\[Ch:Config\]](#Ch:Config){reference-type="ref"
reference="Ch:Config"} (Basic Configuration) to learn how to generate
the MySQL database.

### Init Script for Automatic Startup

There is an init script in the directory scripts/startup. It uses the
usual parameters like "start", "stop" and "restart".

## Unofficial Note: Mac OS X

A user reported WendzelNNTPd-2.0.0 is installable under Mac OS X 10.5.8.
The only necessary change was to add the flag "`-arch x86_64`" to
compile the code on a 64 bit system. However, I never tried to compile
WendzelNNTPd on a Mac.

## Windows

Not supported.

[^1]: On some \*nix-like operating systems you need to first run **gzip
    -d wendzelnntpd-VERSION.tgz** and then **tar -xf
    wendzelnntpd-VERSION.tar** instead of letting `tar` do the whole
    job.
