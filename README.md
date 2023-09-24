# WendzelNNTPd

**WendzelNNTPd** is an **easy to configure Usenet server** (NNTP daemon). WendzelNNTPd breaks down complicated things into an easy-to-use configuration file and tool. The server is portable (Linux/*BSD/*nix), supports IPv4 and IPv6, AUTHINFO authentication, contains support for Access Control Lists (ACL), Role-based Access Control (RBAC) and supports invisible newsgroups. It currently supports MySQL and SQLite backends.

This server is tailored for workgroups, where users trust each other and where no synchronization with other usenet servers is necessary. For this reason, not all advanced NNTP features are included (e.g. commands for server synchronization). The server is also not a suitable for confidential environments as it lacks TLS support and strong hardening.

## Download

##### [Github Repository / Current Development Branch](https://github.com/cdpxe/WendzelNNTPd)

##### Source
- Linux/Unix/*BSD/POSIX: [Latest stable release code](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.3/) (v. 2.1.3 -- tgz)

##### Packages & Executables
- *Slackware Linux*: 
  - Slackware 14.2: [Slackbuilds.org Build Script](https://slackbuilds.org/repository/14.2/network/wendzelnntpd/)
  - Slackware64-current: [Slackware package](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.3/slackware64-current-package/) (v. 2.1.3 -- tgz)
     - Installation via `installpkg (filename)`
- *NetBSD*:
  - Port for WendzelNNTPd on *pkgsrc*: [WendzelNNTPd-2.1.3](https://pkgsrc.se/wip/wendzelnntpd)
- *Windows*:
  - Legacy WendzelNNTPd 1.4.6 branch: [WendzelNNTPd-1.4.6-Setup.exe](https://sourceforge.net/projects/wendzelnntpd/files/wendzelnntpd/1.4.6/)


## Features

* Runs on Linux, OpenSolaris, *BSD
* Supports IPv6
* Conservative design philosophy:
   * Tiny (approx. 10,000 Lines of Code), to limit potential (security) flaws, including optional features that can be deactivated at compile time.
   * Do not implement unnecessary features.
   * Do not make things too complicated and check for compatibility with old newsreaders.
* Written in C
* Database abstraction layer (supports SQLite3 and MySQL)
* Supports NNTP authentication (AUTHINFO USER/PASS)
* Supports advanced Access Control Lists (ACL) and Role-based Access Control (RBAC)
* Automatically prevents double-postings
* Supports "invisible newsgroups"
* It is open + free software! :)

## Why you want a Usenet server

Probably because you are into retro computing and already run a Gopher service! :) WendzelNNTPd is not tailored as a critical service, it is for nerds who like to play with the protocol and want to use it for fun! Also, feel invited to contribute your patches and extensions!

Read my [blog posting on WendzelNNTPd](http://www.wendzel.de/misc/2021/01/04/new-release-usenet-server.html).

## Documentation

The documentation can be found [here](https://github.com/cdpxe/WendzelNNTPd/blob/master/docs/docs.pdf).

## Forum for your Questions and Comments

[https://sourceforge.net/p/wendzelnntpd/discussion/general/](https://sourceforge.net/p/wendzelnntpd/discussion/general/)

## Propaganda

![Powered by WendzelNNTPd](images/wendzelnntpd_powered.png "powered by WendzelNNTPd usenet server")

![Powered by WendzelNNTPd](images/wendzelnntpd_powered2.png "powered by WendzelNNTPd usenet server")
