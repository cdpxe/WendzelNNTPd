#!/bin/sh

mkdir -p tmp

openssl genrsa -out "./tmp/client-key.pem" 2048

openssl req -new -key "./tmp/client-key.pem" -out "./tmp/client.csr" -config "./test-files/openssl-client.cnf"

openssl x509 -req -days 36500 -in "./tmp/client.csr" -CA "./tmp/ca-self.crt" -CAkey "./tmp/ca-self.key" -CAcreateserial -extensions v3_req -extfile "./test-files/openssl-client.cnf" -out "./tmp/client-pub.pem"
