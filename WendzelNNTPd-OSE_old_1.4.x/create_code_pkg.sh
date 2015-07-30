#!/bin/bash -e

DST=/tmp/wendzelnntpd/
CWD=`pwd`
VER="1.4.7"

echo "r u sure that the usenet.db file only includes an empty"
echo -n "database? (y/n) "
read a
if [ "$a" != "y" ]; then
	echo "exiting."
	exit 0
fi

make clean
rm -f Makefile.inc

echo -n "Do I need to clean the gui/src too? (y/n) "
read a
if [ "$a" = "y" ]; then
	cd gui/src
	make clean
	cd ../..
	rm -f gui/src/src
fi

rm -rf $DST /tmp/wendzelnntpd*.tgz /tmp/wendzelnntp*.zip
mkdir -p $DST
cp -r * $DST/

cd $DST
rm -rf `find . -name '.svn'`
rm -rf create_code_pkg.sh bad.posting Makefile.inc www

# now create the packages
cd ..
tar -czvf wendzelnntpd-${VER}-src.tgz wendzelnntpd
#zip -r wendzelnntpd-${VER}-src.zip wendzelnntpd

cd ${CWD}

