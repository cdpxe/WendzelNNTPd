#!/bin/bash -e

DST=/tmp/wendzelnntpd/
CWD=`pwd`
VER=`grep '#define VERSION' src/include/main.h | head -n 1 | tr '"' ' ' | awk '{print $3}'`

if [ ! -e Makefile.inc ]; then
	echo "No Makefile.inc present. Run ./configure first."
	exit 1
fi

echo -n 'Is the version "'${VER}'" correct? (y/n) '
read a
if [ "$a" != "y" ]; then
	echo "exiting."
	exit 0
fi

echo -n "Are you sure that the usenet.db file only includes an empty "
echo -n "database? (y/n) "
read a
if [ "$a" != "y" ]; then
	echo "exiting."
	exit 0
fi

make clean
rm -f Makefile.inc

rm -rf $DST /tmp/wendzelnntpd*.tgz /tmp/wendzelnntp*.zip
mkdir -p $DST
mkdir -v $DST/src $DST/docs $DST/docs/docs $DST/src/include $DST/images $DST/scripts $DST/scripts/startup $DST/bin
cp -v AUTHOR BUGS build CHANGELOG configure CONTRIBUTE FAQ.txt HISTORY INSTALL LICENSE log.txt Makefile README tiny_doc.txt usenet.db wendzelnntpd.conf re_create_db.sh TODO usenet.db_struct usenet.db_struct_clear WISHLIST mysql_db_struct.sql getver.sh $DST/
cp -v src/*.[cyl] $DST/src/
cp -v scripts/startup/init.d_script $DST/scripts/startup/
cp -v src/include/*.h $DST/src/include/
cp -v images/*.png $DST/images
cp -v docs/Makefile docs/*.tex docs/docs.pdf $DST/docs
cp -v docs/docs/*.{html,png,css} $DST/docs/docs

cd $DST
rm -rf `find . -name '.svn'`
rm -rf create_code_pkg.sh bad.posting Makefile.inc www

# now create the packages
cd ..
tar -czvf wendzelnntpd-${VER}-src.tgz wendzelnntpd
#zip -r wendzelnntpd-${VER}-src.zip wendzelnntpd

cd ${CWD}

echo "done."

