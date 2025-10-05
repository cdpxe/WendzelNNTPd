#!/bin/sh

cd unittesting

./initialize_db.sh
cp test-files/wendzelnntpd.conf /etc/wendzelnntpd/
sed -i 's/\/usr\/local\/etc\/wendzelnntpd/\/etc\/wendzelnntpd/' /etc/wendzelnntpd/wendzelnntpd.conf

if ! ./test_wendzelnntpadm.sh 2>&1
then exit 1
fi

exit 0
