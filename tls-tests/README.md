# Testing TLS in WendzelNNTPd

This folder contains files to test the TLS functionality of WendzelNNTPd.

**Important note: DO NOT do this on your production system as we overwrite the configuration file!**

## Compile WendzelNNTPd with TLS support

Compile the program with TLS support and install it. You may need to disable MySQL or SQLite

To use the default GnuTLS:

```
MYSQL=NO TLS=YES ./configure
make && make install
```

To use OpenSSL instead:

```
MYSQL=NO TLS=YES GNUTLS=NO ./configure
make && make install
```

## Required tools to test

These tools are required to test the TLS included in WendzelNNTPd:

* openssl (connect with TLS)
* telnet (connect without TLS)
* checkssl (optional, check for supported ciphers)
* expect (automated interaction with the server)

## Generate TLS Certificates

The helper script creates example files required to activate TLS:

* a somple Certificate Authority (CA)
* a server certificate
* a client certificate to test 'mutual TLS'

Run the script and copy the created files:

```
cd tls-tests
sh ./create-certs.sh
mkdir /usr/local/etc/ssl/
cp ca-root.pem cert-pub.pem cert-key.pem /usr/local/etc/ssl/
```

## Run a 'smoke test'

Now copy the first config file to the default location:

```
cp wendzelnntpd-mandatory-tls.conf /usr/local/etc/wendzelnntpd.conf
```

And run the server, i.e. via startup-script or in a separate shell.

Next, check for available TLS on the configured port 563:
```
checkssl localhost:563
```

You shoud see some output, looking like this:
```
...
  SSL/TLS Protocols:
SSLv2     disabled
SSLv3     disabled
TLSv1.0   disabled
TLSv1.1   disabled
TLSv1.2   enabled
TLSv1.3   enabled
...
```

This means TLS is successfully activated on the server.

## Run simple test with mandatory TLS enabled

Next, check correct functionality of the server with some commands:
```
expect nntp-starttls-test.exp
expect nntp-tls-port-test.exp
expect nntp-mandatory-tls-test.exp
```

All 3 tests should end with:
```
== All checks succeeded ==
```

## Run test with mandatory client certificate

Now stop the server process and copy the second config file to the default location:
```
cp wendzelnntpd-mutual-tls.conf /usr/local/etc/wendzelnntpd.conf
```

Start the server again and run the test scrip:
```
expect nntp-mutual-tls-fails-test.exp
expect nntp-mutual-tls-test.exp
```

The 2 tests should end with:
```
== All checks succeeded ==
```
