# WendzelNNTPd

**WendzelNNTPd** is an **easy to configure Usenet server** (NNTP daemon). WendzelNNTPd breaks down complicated things into an easy-to-use configuration file and tool. The server is portable (Linux/OpenBSD/FreeBSD/NetBSD and some similar systems), supports IPv4 and IPv6, *experimental* support for TLS (SNNTP/NNTPS), AUTHINFO authentication, contains support for Access Control Lists (ACL), Role-based Access Control (RBAC) and supports invisible newsgroups. It currently supports MySQL and SQLite backends.

This server is tailored for retro computing people as well as small workgroups, where users trust each other and where no synchronization with other usenet servers is necessary. For this reason, not all advanced NNTP features are included (e.g. commands for server synchronization).

## Download

##### Version 2.2-alpha

The [Git repository's master branch](https://github.com/cdpxe/WendzelNNTPd) contains the latest version (2.2 alpha) that includes TLS support and several more enhancements.

##### Source (version 2.1.3)
- Linux/Unix/*BSD/POSIX: [Latest stable release code](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.3/) (v. 2.1.3 -- tgz)

##### Packages & Executables
- *Slackware Linux*: 
  - Slackware: Slackbuilds.org Build Script [Slackware 14.2](https://slackbuilds.org/repository/14.2/network/wendzelnntpd/), [Slackware 15.0](https://slackbuilds.org/repository/15.0/network/wendzelnntpd/?search=wendzelnntpd)
  - Slackware64-current: [Slackware package](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.3/slackware64-current-package/) (v. 2.1.3 -- tgz)
     - Installation via `installpkg (filename)`
- *NetBSD*:
  - WendzelNNTPd port at pkgsrc: [WendzelNNTPd-2.1.3](https://pkgsrc.se/wip/wendzelnntpd)
- *Windows*:
  - Legacy WendzelNNTPd 1.4.6 branch: [WendzelNNTPd-1.4.6-Setup.exe](https://sourceforge.net/projects/wendzelnntpd/files/wendzelnntpd/1.4.6/)

## Why you want a Usenet server

Probably because you are into retro computing and already run a Gopher service! :) WendzelNNTPd is not tailored as a critical service, it is for nerds who like to play with the protocol and want to use it for fun! Also, feel invited to contribute your patches and extensions!

Read my [blog posting on WendzelNNTPd](http://www.wendzel.de/misc/2021/01/04/new-release-usenet-server.html).

## Features

* Runs on Linux, OpenSolaris, *BSD
* Supports IPv6
* Conservative design philosophy:
   * Tiny (approx. 11,000 Lines of Code), to limit potential (security) flaws, including optional features that can be deactivated at compile time.
   * Do not implement unnecessary features.
   * Do not make things too complicated and check for compatibility with old newsreaders.
* Written in C
* Database abstraction layer (supports SQLite3 and MySQL)
* Supports NNTP authentication (AUTHINFO USER/PASS)
* Supports advanced Access Control Lists (ACL) and Role-based Access Control (RBAC)
* Automatically prevents double-postings
* Supports "invisible newsgroups"
* It is open + free software! :)
* v.2.2: TLS server authentication and data encryption
* v.2.2: Supports multiple connectors so that multiple socket configurations can be used simultaneously
* v.2.2.: Supports NNTPS ans SNNTP with TLS 1.0-1.3
* v.2.2.: Support for client certificate verification (mTLS) and CRL checks

#### Features Under Development/Call for Testing

* The main branch currently provides version 2.2-alpha, including experimental support for **PostgreSQL** backends, including the option to store postings in the database and eliminate the use of */var/spool/news* (thx to Christian Barthel for the patch!). Further, the main branch provides experimental **TLS support**.

## More Information
- List of contributors [https://github.com/cdpxe/WendzelNNTPd/blob/master/AUTHORS](https://github.com/cdpxe/WendzelNNTPd/blob/master/AUTHORS)
- Website: [https://cdpxe.github.io/WendzelNNTPd/](https://cdpxe.github.io/WendzelNNTPd/)
- Forum for your Questions and Comments: [https://sourceforge.net/p/wendzelnntpd/discussion/general/](https://sourceforge.net/p/wendzelnntpd/discussion/general/)

## Documentation

The documentation can be found [here](https://cdpxe.github.io/WendzelNNTPd/).
It is also available as a PDF file [here](https://cdpxe.github.io/WendzelNNTPd/docs.pdf).

## Propaganda
Some late 1990's/early 2000's-styled 'powered by' logos. Let me know if you include these into your websites.

![Powered by WendzelNNTPd](images/wendzelnntpd_powered.png "powered by WendzelNNTPd usenet server")

![Powered by WendzelNNTPd](images/wendzelnntpd_powered2.png "powered by WendzelNNTPd usenet server")
