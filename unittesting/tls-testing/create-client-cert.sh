#!/bin/sh

openssl genrsa -out "./test-files/client-key.pem" 2048

openssl req -new -key "./test-files/client-key.pem" -out "client.csr" -config "./test-files/openssl-client.cnf"

openssl x509 -req -days 36500 -in "client.csr" -CA "ca-self.crt" -CAkey "ca-self.key" -CAcreateserial -extensions v3_req -extfile "./test-files/openssl-client.cnf" -out "./test-files/client-pub.pem"