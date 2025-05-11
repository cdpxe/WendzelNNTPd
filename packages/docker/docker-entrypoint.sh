#!/bin/sh
set -e

if [ "$1" = "wendzelnntpd" ]
then
    if [ ! -e /usr/local/etc/wendzelnntpd/ssl/server.crt ] \
        || [ ! -e /usr/local/etc/wendzelnntpd/ssl/server.key ] \
        || [ ! -e /usr/local/etc/wendzelnntpd/ssl/ca.crt ]
    then
        create_certificate --environment local
    fi
fi

exec "$@"
