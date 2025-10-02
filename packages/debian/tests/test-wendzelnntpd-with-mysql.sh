#!/bin/sh

cd unittesting

# reconfigure service for the tests
service wendzelnntpd stop

mysql --execute \
    "set session sql_mode='ANSI_QUOTES'; source /usr/share/wendzelnntpd/mysql_db_struct.sql; \
    source create_db_test_data.sql; \
    CREATE USER 'testuser'@'%' IDENTIFIED BY 'testpass'; \
    GRANT SELECT, INSERT, UPDATE, DELETE ON WendzelNNTPd.* TO 'testuser'@'%'"

cp test-files/wendzelnntpd.conf /etc/wendzelnntpd/
sed -i 's/\/usr\/local\/etc\/wendzelnntpd/\/etc\/wendzelnntpd/' /etc/wendzelnntpd/wendzelnntpd.conf
sed -i 's/^database-engine sqlite3$/database-engine mysql/' /etc/wendzelnntpd/wendzelnntpd.conf
mkdir -p tmp
cp /etc/wendzelnntpd/ssl/ca.crt tmp/ca-self.crt
cp /etc/wendzelnntpd/ssl/ca-key.pem tmp/ca-self.key
./create-client-cert.sh 2>&1

service wendzelnntpd start
sleep 1

# run tests
if ! ./run.sh 2>&1
then exit 1
fi

exit 0
