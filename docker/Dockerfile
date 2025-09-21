FROM debian:stable-slim

RUN apt-get update -y && apt-get install -y gcc flex bison sqlite3 libsqlite3-dev mariadb-client libmariadb-dev-compat \
    ca-certificates libmariadb-dev libmhash-dev make golang-go openssl libssl-dev gdb

# install reflex for on-demand compiling of source code
RUN go install github.com/cespare/reflex@latest

RUN touch /var/log/wendzelnntpd

COPY openssl.cnf /usr/local/etc/wendzelnntpd/ssl/openssl.cnf

# Create server certificate for TLS
RUN mkdir -p /usr/local/etc/wendzelnntpd/ssl
# create CA
RUN openssl req \
  -x509 \
  -new \
  -newkey rsa:2048 \
  -days 3650 \
  -nodes \
  -extensions v3_ca \
  -config "/usr/local/etc/wendzelnntpd/ssl/openssl.cnf" \
  -keyout "/usr/local/etc/wendzelnntpd/ssl/ca-key.pem" \
  -out "/usr/local/etc/wendzelnntpd/ssl/ca.crt"

RUN openssl genrsa -out "/usr/local/etc/wendzelnntpd/ssl/server.key" 2048
RUN openssl req \
  -new -key "/usr/local/etc/wendzelnntpd/ssl/server.key" \
  -out "/usr/local/etc/wendzelnntpd/ssl/server.csr" \
  -config "/usr/local/etc/wendzelnntpd/ssl/openssl.cnf"

RUN openssl x509 \
    -req \
    -days 365 \
    -in "/usr/local/etc/wendzelnntpd/ssl/server.csr" \
    -CA "/usr/local/etc/wendzelnntpd/ssl/ca.crt" \
    -CAkey "/usr/local/etc/wendzelnntpd/ssl/ca-key.pem" \
    -CAcreateserial \
    -extensions v3_req \
    -extfile "/usr/local/etc/wendzelnntpd/ssl/openssl.cnf" \
    -out "/usr/local/etc/wendzelnntpd/ssl/server.crt"

# for local development only copy existing certificate (GitHub Action will use different certificates there)
RUN cp /usr/local/etc/wendzelnntpd/ssl/server.crt /usr/local/etc/wendzelnntpd/ssl/server-self.crt \
    && cp /usr/local/etc/wendzelnntpd/ssl/server.key /usr/local/etc/wendzelnntpd/ssl/server-self.key \
    && cp /usr/local/etc/wendzelnntpd/ssl/ca.crt /usr/local/etc/wendzelnntpd/ssl/ca-self.crt

WORKDIR /wendzelnntpd

ENTRYPOINT ["/root/go/bin/reflex", "-c", "docker/reflex.conf"]
