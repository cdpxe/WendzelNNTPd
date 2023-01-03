#!/bin/sh

# create CA
openssl req \
  -x509 \
  -new \
  -newkey rsa:2048 \
  -days 3650 \
  -nodes \
  -extensions v3_ca \
  -subj "/C=DE/ST=Hagen/O=Test-Cert Inc." \
  -keyout "ca-key.pem" \
  -out "ca-root.pem"

# generate server cert
openssl genrsa -out "cert-key.pem" 2048
openssl req \
  -new -key "cert-key.pem" \
  -out "cert.csr" \
  -config "openssl.cnf"

openssl x509 \
  -req \
  -days 365 \
  -in "cert.csr" \
  -CA "ca-root.pem" \
  -CAkey "ca-key.pem" \
  -CAcreateserial \
  -extensions v3_req \
  -extfile "openssl.cnf" \
  -out "cert-pub.pem"

# generate a client cert
openssl genrsa -out "client-key.pem" 2048
openssl req \
  -new -key "client-key.pem" \
  -out "client.csr" \
  -config "openssl-client.cnf"

openssl x509 \
  -req \
  -days 365 \
  -in "client.csr" \
  -CA "ca-root.pem" \
  -CAkey "ca-key.pem" \
  -CAcreateserial \
  -extensions v3_req \
  -extfile "openssl-client.cnf" \
  -out "client-pub.pem"
