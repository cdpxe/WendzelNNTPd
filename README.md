# WendzelNNTPd

The WendzelNNTPd is an IPv6-ready Usenet server (NNTP daemon) with the main goal to maximize usability on the console level. WendzelNNTPd achieves that by breaking down complicated things to an easy-to-use configuration file + tool. The server is portable (Linux/*BSD/*nix), supports AUTHINFO authentication, contains support for Access Control Lists (ACL), role based access control (RBAC) and supports invisible newsgroups. It currently allows MySQL and SQLite backends

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
* It is open source ;-)

## Website

http://steffen-wendzel.blogspot.de/p/wendzelnntpd.html

## Documentation

The documentation can be found in the directory [docs/](docs/).
