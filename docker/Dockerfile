FROM debian:stable-slim

RUN apt-get update -y
RUN apt-get install -y gcc flex bison sqlite3 libsqlite3-dev mariadb-client libmariadb-dev-compat ca-certificates libmariadb-dev libmhash-dev make golang-go openssl libssl-dev gdb

RUN go install github.com/cespare/reflex@latest

RUN touch /var/log/wendzelnntpd

COPY docker/openssl/openssl.cnf /usr/local/etc/ssl/openssl.cnf

RUN mkdir -p /usr/local/etc/ssl
# create CA
RUN openssl req \
  -x509 \
  -new \
  -newkey rsa:2048 \
  -days 3650 \
  -nodes \
  -extensions v3_ca \
  -subj "/C=DE/ST=Hagen/O=Test-Cert Inc." \
  -keyout "/usr/local/etc/ssl/ca-key.pem" \
  -out "/usr/local/etc/ssl/ca-root.pem"

RUN openssl genrsa -out "/usr/local/etc/ssl/server.key" 2048
RUN openssl req \
  -new -key "/usr/local/etc/ssl/server.key" \
  -out "/usr/local/etc/ssl/server.csr" \
  -config "/usr/local/etc/ssl/openssl.cnf"

RUN openssl x509 \
    -req \
    -days 365 \
    -in "/usr/local/etc/ssl/server.csr" \
    -CA "/usr/local/etc/ssl/ca-root.pem" \
    -CAkey "/usr/local/etc/ssl/ca-key.pem" \
    -CAcreateserial \
    -extensions v3_req \
    -extfile "/usr/local/etc/ssl/openssl.cnf" \
    -out "/usr/local/etc/ssl/server.crt"

WORKDIR /wendzelnntpd

ENTRYPOINT ["/root/go/bin/reflex", "-c", "docker/reflex.conf"]