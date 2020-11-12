# WendzelNNTPd

**WendzelNNTPd** is an IPv6-ready Usenet server (NNTP daemon) with the main goal to maximize usability on the console level. WendzelNNTPd achieves this by breaking down complicated things into an easy-to-use configuration file and tool. The server is portable (Linux/*BSD/*nix), supports AUTHINFO authentication, contains support for Access Control Lists (ACL), Role-based Access Control (RBAC) and supports invisible newsgroups. It currently supports MySQL and SQLite backends.

## Why you want a Usenet server

Probably because you are into retro computing and already run a Gopher service! :) WendzelNNTPd is not tailored as a critical service, it is for nerds who like to play with the protocol and want to use it for fun! Also, feel invited to contribute your patches and extensions!

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

## Website

https://cdpxe.github.io/WendzelNNTPd/

## Documentation/FAQ

The documentation can be found [here](https://github.com/cdpxe/WendzelNNTPd/blob/master/docs/docs.pdf). However, if you are looking for a quick and easy to read frequently asked questions section then see [FAQ.md](FAQ.md).

## Open Thesis Topics
- Add TLS support (NTTPS) for WendzelnNNTPd
- Add support for MariaDB and perform code auditing checks
