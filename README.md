# WendzelNNTPd

The WendzelNNTPd is an IPv6-ready Usenet server (NNTP daemon) with the main goal to maximize usability on the console level. WendzelNNTPd achieves that by breaking down complicated things to an easy-to-use configuration file + tool. The server is portable (Linux/*BSD/*nix), supports AUTHINFO authentication, contains support for Access Control Lists (ACL), role based access control (RBAC) and supports invisible newsgroups. It currently allows MySQL and SQLite backends

### Download

##### Stable Source
- Linux/Unix/*BSD/POSIX: [Latest stable release code](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.2/) (v. 2.1.2 -- tgz)

##### Packages & Executables

- Slackware64-current: [Slackware package](https://sourceforge.net/projects/wendzelnntpd/files/v2.1.2/slackware64-current-package/) (v. 2.1.2 -- txz)
     - Installation via `installpkg (filename)`
- Windows: Legacy WendzelNNTPd 1.4.6: [WendzelNNTPd-1.4.6-Setup.exe](https://sourceforge.net/projects/wendzelnntpd/files/wendzelnntpd/1.4.6/)

#### Current Development Branch

The current version of the development code is available [here](https://github.com/cdpxe/WendzelNNTPd).


## Features

* Runs on Linux, OpenSolaris, *BSD
* Supports IPv6
* Database abstraction layer (supports SQLite3 and MySQL)
* Tiny (only about 7,500 Lines of Code)
* Written in C
* Supports NNTP authentication (AUTHINFO USER/PASS)
* Supports advanced ACL and role based access control (RBAC)
* Automatically prevents double-postings
* Supports "invisible newsgroups"
* It is open, free software! :)

## Why you want a Usenet server

Probably because you are into retro computing and already run a Gopher service! :) WendzelNNTPd is not tailored as a critical service, it is for nerds who like to play with the protocol and want to use it for fun! Also, feel invited to contribute your patches and extensions!

Read my [blog posting on WendzelNNTPd](http://www.wendzel.de/misc/2021/01/04/new-release-usenet-server.html).

## Documentation

The documentation can be found [here](https://github.com/cdpxe/WendzelNNTPd/blob/master/docs/docs.pdf).

## Propaganda

![Powered by WendzelNNTPd](images/wendzelnntpd_powered.png "powered by WendzelNNTPd usenet server")

![Powered by WendzelNNTPd](images/wendzelnntpd_powered2.png "powered by WendzelNNTPd usenet server")
