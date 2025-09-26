#!/bin/bash
image="${1:-wendzelnntpd}"

check_returncode() {
    actual=$1
    expected=$2
    if [ "$actual" -ne "$expected" ]
    then
        echo "Failure"
        echo "Expected returncode $expected but was $actual"
        echo "Output:"
        echo "$3"
        exit 1
    fi
}

echo "== Test wendzelnntp"
#docker run --name wendzelnntpd --rm -d -p 119:119 -p 563:563 wendzelnntpd
output=$(docker run --name "$image" --rm -d -p 119:119 -p 563:563 wendzelnntpd 2>&1)
returncode=$?
check_returncode $returncode 0 "$output"

echo "= Check connection to wendzelnntpd"
nntp_address=localhost expect tests/nntp-help-test.exp
returncode=$?
check_returncode $returncode 0

docker stop wendzelnntpd

echo "== Test wendzelnntpadm"
output=$(docker run --rm "$image" wendzelnntpadm listgroups 2>&1)
returncode=$?
check_returncode $returncode 0 "$output"

echo "= Check whether the group which gets created by default exists"
if ! echo "$output" | grep -q -- "alt.wendzelnntpd.test"
then
	echo "Failure"
	echo "Can't find group alt.wendzelnntpd.test"
	echo "Received output:"
	echo "$output"
	exit 1
fi

echo "== Test create_certificate"
output=$(docker run --rm -v "$(pwd)"/out:/usr/local/etc/wendzelnntpd/ssl "$image" create_certificate)
returncode=$?
check_returncode $returncode 0 "$output"

echo "= Check certificates"
if [ ! -f out/server.crt ] || [ ! -f out/server.key ] || [ ! -f out/ca.crt ]
then
	echo "Can't find certificates in out/"
	exit 1
fi
