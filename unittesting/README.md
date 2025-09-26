# Testing

This directory contains tests for manual and automatic testing of `wendzelnntpd` and `wendzelnntpadm`.
The subdirectory [manual](manual) contains some files for manual testing of the POST NNTP command.
The subdirectory [tests](tests) contains scripts, which use the `expect` command to test the `wendzelnntpd` server
and need a properly configured and initialized `wendzelnntpd` server.
The script `run.sh` can be used to run the tests, but they can also be executed individually.
The testing of `wendzelnntpadm` is done by the script `test_wendzelnntpadm.sh`.
The testing of the Docker image for WendzelNNTPd is done by the script `test_dockerimage.sh`.

## Setup of the testing environment

The tests for `wendzelnntpd` and `wendzelnntpadm` assume specific data in the database and a specific configuration.
Additionally, the tests for `wendzelnntpd` require a running instance of `wendzelnntpd`.
Most of the following commands require root privileges.
The setup of the database can be done with the script `initialize_db.sh` which creates a new SQLite database and
initializes it with the test data. It also copies some files of existing posts to `/var/spool/news/wendzelnntpd`
and creates the file nextmsgid there.
`unittesting/wendzelnntpd.conf` contains the configuration as expected by the tests and needs to be copied to the
location of the configuration file of `wendzelnntpd` (`/usr/local/etc/wendzelnntpd/wendzelnntpd.conf` by default):
```shell
./initialize_db.sh
cp test-files/wendzelnntpd.conf /usr/local/etc/wendzelnntpd/
```

Some tests are testing the TLS functionality of `wendzelnntpd` and require some certificates.
Most certificates are already generated during `make install` by the script `create_certificate`.
Additionally, you need to copy the CA certificate to the directory `tmp` and generate client certificates
with the script `create-client-cert.sh`:
```shell
cd unittesting
mkdir tmp
cp /usr/local/etc/wendzelnntpd/ssl/ca.crt tmp/ca-self.crt
cp /usr/local/etc/wendzelnntpd/ssl/ca-key.pem tmp/ca-self.key
./create-client-cert.sh
```

Then you can start `wendzelnntpd`:
```shell
wendzelnntpd -d
```

## Execution of the tests for wendzelnntpd

The Dockerfile in this directory can be used to run the Unit-Tests in [tests](tests).
Its also in Docker-Hub-Registry: https://hub.docker.com/r/svenlie/expect

```shell
docker run --rm -v .:/data -e nntp_address='YOUR_DOMAIN_HERE' -it svenlie/expect bash
```

Then run:

```shell
cd /data/unittesting && chmod +x run.sh && ./run.sh
```

It will test against the NNTP-Server which is specified under YOUR_DOMAIN_HERE.
The tests run against `localhost` by default if `nntp_address` is not defined.

For local-testing you need to have installed TCL (https://core.tcl-lang.org/tcl/download) and Expect
(https://core.tcl-lang.org/expect/index?name=Expect) and set the environment variable "nntp_address":

```shell
export nntp_address=127.0.0.1
cd unittesting && chmod +x run.sh && ./run.sh
```

## Execution of the tests in [tests/special](tests/special)

The tests in [tests/special](tests/special) are not executed by `run.sh` because they need special configuration
which would break the other tests.

The test [nntp-acl-test.exp](tests/special/nntp-mandatory-tls-test.exp) tests the configuration option 
`use-acl` which enables the support for access control lists (ACL) which prevents users from accessing newsgroups
without authentication and permissions for accessing the group.
To execute this test it is required to setup the testing environment as described above and add the lines
`use-acl` and `use-authentication` to the configuration file `wendzelnntpd.conf`.
The test can then be executed with `expect`:
```shell
nntp_address=127.0.0.1 expect tests/special/nntp-acl-test.exp
```

The test [nntp-mandatory-tls-test.exp](tests/special/nntp-mandatory-tls-test.exp) tests the configuration option
`tls-is-mandatory` which makes TLS mandatory for all connections to `wendzelnntpd`.
To execute this test it is required to setup the testing environment as described above and add the line
`tls-is-mandatory` to the configuration file `wendzelnntpd.conf`.
The test can then be executed with `expect`:
```shell
nntp_address=127.0.0.1 expect tests/special/nntp-mandatory-tls-test.exp
```

## Execution of the tests for wendzelnntpadm

The tests for `wendzelnntpadm` are a plain shell script, so they do not need `expect` to be executed.
To execute this tests it is required to setup the testing environment as described above.
The test can be executed by running the script `test_wendzelnntpadm.sh`:
```shell
./test_wendzelnntpadm.sh
```

## Execution of the tests with other database backends

The tests can also be executed with other database backends than SQLite3.
The procedure is basically the same as described above, but the database cannot be initialized with the script
`initialize_db.sh`. Instead, you need to set up the database by yourself and can use `create_db_test_data.sql` to
initialize the test data in the database.
Additionally, you need to change the configuration in `wendzelnntpd.conf` to use the different database by specifying
the configuration options `database-engine`, `database-server` and `database-port` accordingly.

Example for initializing a MySQL database:
```shell
mysql --host=127.0.0.1 --user=root --password=rootpass --execute "set session sql_mode='ANSI_QUOTES'; source database/mysql_db_struct.sql; source unittesting/create_db_test_data.sql; GRANT SELECT, INSERT, UPDATE, DELETE ON WendzelNNTPd.* TO 'testuser'@'%'"
```

Example for initializing a PostgreSQL database:
```shell
psql postgresql://testuser:testpass@127.0.0.1:5432/wendzelnntpd --file database/postgres_db_struct.sql --file unittesting/create_db_test_data.sql
```

## Execution of the tests for the Docker image

You need to build the Docker image with `make docker-build` before running the tests.
These tests require no further setup of the testing environment.
```shell
make docker-build
cd unittesting/
./test_dockerimage.sh
```
