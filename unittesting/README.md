The Dockerfile in this directory can be used to run TLS Unit-Tests. Its also in Docker-Hub-Registry: https://hub.docker.com/r/svenlie/expect

```
docker run --rm -v .:/data -e nntp_address='YOUR_DOMAIN_HERE' -it svenlie/expect bash
```

Then run:

```
chmod +x /data/unittesting/run.sh && /data/unittesting/run.sh
```

It will test against the NNTP-Server which is specified under YOUR_DOMAIN_HERE

For local-testing you need have installed TCL (https://core.tcl-lang.org/tcl/download) and Expect (https://core.tcl-lang.org/expect/index?name=Expect) and set the environment variable "nntp_address":

```
export nntp_address=127.0.0.1
chmod +x ./unittesting/run.sh && ./unittesting/run.sh
```

The test nntp-mandatory-tls-test.exp_ can not be run automatically because then only TLS connection is possible in user acceptance test
