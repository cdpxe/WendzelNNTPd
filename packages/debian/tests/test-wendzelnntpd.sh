#!/bin/sh

cd unittesting

# reconfigure service for the tests
service wendzelnntpd stop

./initialize_db.sh
cp test-files/wendzelnntpd.conf /etc/wendzelnntpd/
sed -i 's/\/usr\/local\/etc\/wendzelnntpd/\/etc\/wendzelnntpd/' /etc/wendzelnntpd/wendzelnntpd.conf
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

# enable mandatory tls
echo "tls-is-mandatory" >> /etc/wendzelnntpd/wendzelnntpd.conf
service wendzelnntpd restart
sleep 1

# run tests
if ! nntp_address=localhost expect tests/special/nntp-mandatory-tls-test.exp 2>&1
then exit 1
fi

# enable acl
cp test-files/wendzelnntpd.conf /etc/wendzelnntpd/
sed -i 's/\/usr\/local\/etc\/wendzelnntpd/\/etc\/wendzelnntpd/' /etc/wendzelnntpd/wendzelnntpd.conf
echo "use-authentication" >> /etc/wendzelnntpd/wendzelnntpd.conf
echo "use-acl" >> /etc/wendzelnntpd/wendzelnntpd.conf
service wendzelnntpd restart
sleep 1

# run tests
if ! nntp_address=localhost expect tests/special/nntp-acl-test.exp 2>&1
then exit 1
fi

exit 0
