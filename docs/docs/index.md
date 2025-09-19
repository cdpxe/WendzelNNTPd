# Introduction

WendzelNNTPd is a tiny but easy to use Usenet server (NNTP server) for
Linux, \*nix and BSD. The server is written in C. For security reasons,
it is compiled with stack smashing protection by default, if your
compiler supports that feature.

## Features

### License

WendzelNNTPd uses the GPLv3 license.

### Database Abstraction Layer

The server contains a database abstraction layer. Currently supported
database systems are SQlite3 and MySQL (and experimental PostgreSQL
support). New databases can be easily added.

### Security

WendzelNNTPd contains different security features, the most important
features are probably Access Control Lists (ACLs) and the Role Based
Access Control (RBAC) system. ACL and RBAC are described in a own
chapter. WendzelNNTPd was probably the first Usenet server with support
for RBAC.

Another feature that was introduced by WendzelNNTPd (and later adopted
by other servers) are so-called "invisible newsgroups": If access
control is activated, a user without permission to access the newsgroup
will not be able to see the existence of the newsgroup. In case the user
knows about the existence of the newsgroup nevertheless, he will not be
able to post to or read from the newsgroup.

However, **please note** that the salting for password hashing is using
SHA-256, but with a global user-definable salt that is concatenated with
the username and password, rendering it less secure than using unique
random hashes per password. WendzelNNTPd does support TLS v1.0 to v1.3
including STARTTLS!

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

## Contribute

See the *CONTRIBUTE* file in the tarball.

## History

The project started in 2004 under the name Xyria:cdpNNTPd, as part of
the Xyria project that also contained a fast DNS server, called
Xyria:DNSd. In 2007, I renamed it to WendzelNNTPd and stopped
development of Xyria:DNSd. Version 1.0.0 was released in 2007, version
2.0.0 in 2011. Since then I have primarily fixed reported bugs and added
minor features but the software is still maintained and smaller
advancements can still be expected. A detailed history can be found in
the *HISTORY* file in the tarball. Fortunately, several people
contributed to the code and documentation, see *AUTHORS* file.
