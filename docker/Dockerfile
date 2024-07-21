FROM debian:stable-slim

RUN apt-get update -y
RUN apt-get install -y gcc flex bison sqlite3 libsqlite3-dev mariadb-client libmariadb-dev-compat ca-certificates libmariadb-dev libmhash-dev make golang-go

RUN go install github.com/cespare/reflex@latest

RUN touch /var/log/wendzelnntpd

WORKDIR /wendzelnntpd

ENTRYPOINT ["/root/go/bin/reflex", "-c", "docker/reflex.conf"]