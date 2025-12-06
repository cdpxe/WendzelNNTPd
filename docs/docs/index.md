# Introduction

WendzelNNTPd is a tiny but easy to use Usenet server (NNTP server) for
Linux, \*nix and BSD. The server is written in C.

## Features

### License

WendzelNNTPd uses the GPLv3 license.

### Database Abstraction Layer

The server contains a database abstraction layer. Currently supported
database systems are SQLite3 and MySQL (and experimental PostgreSQL
support). New databases can be added in an easy way.

### Security

WendzelNNTPd contains different security features, the most important
features are probably Access Control Lists (ACLs) and the Role Based
Access Control (RBAC) system. ACL and RBAC are described in a own
chapter. WendzelNNTPd was probably the first Usenet server with support
for RBAC.

Encrypted connections are feasible as WendzelNNTPd supports TLS v1.0
to v1.3 including STARTTLS!

Another feature that was introduced by WendzelNNTPd (and later adopted
by other servers) are so-called "invisible newsgroups": If access
control is activated, a user without permission to access the newsgroup
will not be able to see the existence of the newsgroup. In case the user
knows about the existence of the newsgroup nevertheless, he will not be
able to post to or read from the newsgroup.

However, **please note** that the salting for password hashing is using
SHA-256, but with a global user-definable salt that is concatenated with
the username and password, rendering it less secure than using unique
random hashes per password.

### Auto-prevention of double-postings

In case a user sends a posting that lists the same newsgroup multiple
times within one post command's "Newsgroups:" header tag, the server
will add it only once to that newsgroup to save memory on the server and
the time of the readers.

### IPv6

WendzelNNTPd supports IPv6. The server can listen on multiple IP
addresses as well as multiple ports.

### Why this is not a perfect Usenet server

WendzelNNTPd does not implement all NNTP commands, but the (most)
important ones. Another problem is that the regular expression library
used is not 100% compatible with the NNTP matching in commands like
"XGTITLE". Another limitation is that WendzelNNTPd cannot share messages
with other NNTP servers.

### Supported RFCs, NNTP commands and capabilities

The initial [RFC 977 (Network News Transfer Protocol)](https://datatracker.ietf.org/doc/html/rfc977)
for NNTP is partially supported:

- Supported commands: ARTICLE, BODY, HEAD, STAT, GROUP, HELP, LIST, POST, QUIT
- Unsupported commands: IHAVE, LAST, NEWGROUPS, NEWNEWS, NEXT, SLAVE

WendzelNNTPd also supports some commands from 
[RFC 2980 (Common NNTP Extensions)](https://datatracker.ietf.org/doc/html/rfc2980):
LIST NEWSGROUPS, LIST OVERVIEW.FMT, LISTGROUP, MODE READER, XGTITLE, XHDR, XOVER, DATE

The newer NNTP standard [RFC 3977 (Network News Transfer Protocol)](https://datatracker.ietf.org/doc/html/rfc3977)
supersedes RFC 977 and RFC 2980. WendzelNNTPd supports this standard partially.
The new command CAPABILITIES is supported.
The RFC contains some changes to the commands from RFC 977 and 2980.
WendzelNNTPd does not support all changes.
It is generally safer to assume that WendzelNNTPd behaves as described in RFC 977 and 2980.

[RFC 4642 (Using Transport Layer Security (TLS) with NNTP)](https://datatracker.ietf.org/doc/html/rfc4642)
is supported (including the new command STARTTLS).

[RFC 4643 (Extension for authentication)](https://datatracker.ietf.org/doc/html/rfc4643) is partially supported.
WendzelNNTPd supports authentication with username and password
(the commands AUTHINFO USER and AUTHINFO PASS).
Authentication with the Simple Authentication and Security Layer (SASL)
is not supported (command AUTHINFO SASL).

The RFCs
[4644 (Extension for Streaming Feeds)](https://datatracker.ietf.org/doc/html/rfc4644),
[6048 (Additions to LIST Command)](https://datatracker.ietf.org/doc/html/rfc6048)
and [8054 (Extension for Compression)](https://datatracker.ietf.org/doc/html/rfc8054) are unsupported.

Supported capabilities: AUTHINFO, LIST, MODE-READER, POST, STARTTLS, VERSION

## Contribute

See the [*CONTRIBUTING* file](https://github.com/cdpxe/WendzelNNTPd/blob/master/CONTRIBUTING.md).

## History

The project started in 2004 and was written by [Steffen Wendzel](https://www.wendzel.de).
Version 1.0 was released in 2007, version 2.0 in 2011. Ten years later (2021), version 2.1 was released.
The current version 2.2 was released in autumn of 2025.

Since around 2018 the development was strongly supported by Wendzel's students at the
[University of Hagen](https://en.wikipedia.org/wiki/University_of_Hagen). See the
[*AUTHORS* file](https://github.com/cdpxe/WendzelNNTPd/blob/master/AUTHORS) for a list
of major contributors.

A detailed history can be found in the [*HISTORY* file](https://github.com/cdpxe/WendzelNNTPd/blob/master/HISTORY).
