#!/bin/bash -e

ADM=./bin/wendzelnntpadm

# remove old tables (if exists) and prevent stderr messages during
# the install
cat database/usenet.db_struct_clear | \
	sqlite3 /var/spool/news/wendzelnntpd/usenet.db >/dev/null 2>&1

# now create the real database
cat database/usenet.db_struct | \
	sqlite3 /var/spool/news/wendzelnntpd/usenet.db

# find wendzelnntpadm
if [ ! -x $ADM ]; then
	ADM=/usr/local/sbin/wendzelnntpadm
	if [ ! -x $ADM ]; then
		ADM=/usr/sbin/wendzelnntpadm
		if [ ! -x $ADM ]; then
			echo "$ADM nowhere found."
			exit 1
		fi
	fi
fi

# create first newsgroup
$ADM create alt.wendzelnntpd.test y
