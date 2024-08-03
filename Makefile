#
# WendzelNNTPd is distributed under the GPLv3 license you can find a
# copy of the license in the 'LICENSE' file.
#

include Makefile.inc

UDBFILE=/var/spool/news/wendzelnntpd/usenet.db
DESTCFLAGS=-DCONFDIR=\"$(CONFDIR)\"
HEADERS=$(SRC)/include/cdpstrings.h $(SRC)/include/main.h $(SRC)/include/wendzelnntpdpath.h
CFLAGS= -c -Wall $(STACK_PROT) $(DESTCFLAGS)
#CFLAGS+=-Wmissing-prototypes -Wmissing-declarations -Wshadow -Wcast-qual \
#	-Wsign-compare -Wstrict-prototypes -Wcast-align \
#	-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wshadow \
#	-Wstrict-overflow=5 -Wundef -Werror -Wno-unused
#CFLAGS+=-Wextra -Wunreachable-code  #-Wsign-conversion -Wswitch-default
CFLAGS+=$(ADD_CFLAGS)

BUILD=-DBUILD=\"`cat build`\"
GDBON=-ggdb -g #-lefence

DEBUG=$(GDBON) -DDEBUG -DXXDEBUG

BUILDFLAGS=-O2 $(STACK_PROT) $(ADD_LNKFLAGS)

# The list of documentation files we wish to install
DOCFILES_TO_INST=AUTHORS CHANGELOG HISTORY README.md INSTALL LICENSE database/usenet.db_struct database/mysql_db_struct.sql
MANPAGES=docs/wendzelnntpd.8 docs/wendzelnntpadm.8

all : wendzelnntpadm main.o db_rawcheck.o log.o database.o cdpstrings.o server.o lexyacc charstack.o libfunc.o acl.o db_abstraction.o hash.o $(SQLITEOBJ) $(MYSQLOBJ) $(POSTGRESOBJ) $(OPENSSLOBJ) globals.o
	expr `cat build` \+ 1 >build
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpd main.o log.o server.o lex.yy.o config.tab.o database.o globals.o cdpstrings.o db_rawcheck.o acl.o db_abstraction.o hash.o $(SQLITEOBJ) $(MYSQLOBJ) $(POSTGRESOBJ) charstack.o libfunc.o $(OPENSSLOBJ) $(SOLNETLIBS) $(SQLITELIB) $(MYSQLLIB) $(POSTGRESLIB) $(LIBDIRS) $(SOLNETLIBS) $(GCCLOCALPTHREAD) $(LIBPTHREAD) $(LIBMHASH) $(OPENSSLLIB)
	#strip bin/wendzelnntpd

lexyacc : lex.yy.o config.tab.o

lex.yy.o : $(SRC)/config.l $(HEADERS)
	$(LEX) src/config.l

config.tab.o : $(SRC)/config.y lex.yy.o $(HEADERS)
	$(YACC) -v -d -t -o config.tab.c $(SRC)/config.y
	$(CC) $(DEBUG) $(ADD_CFLAGS) $(INCDIRS) $(DESTCFLAGS) -c config.tab.c
	$(CC) $(DEBUG) $(ADD_CFLAGS) $(INCDIRS) $(DESTCFLAGS) -c lex.yy.c

database.o : $(SRC)/database.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

main.o : $(SRC)/main.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

log.o : $(SRC)/log.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

libfunc.o : $(SRC)/libfunc.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

db_rawcheck.o : $(SRC)/db_rawcheck.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

acl.o : $(SRC)/acl.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

db_abstraction.o : $(SRC)/db_abstraction.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

db_sqlite3.o : $(SRC)/db_sqlite3.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

db_postgres.o : $(SRC)/db_postgres.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

db_mysql.o : $(SRC)/db_mysql.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

cdpstrings.o : $(SRC)/cdpstrings.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

server.o : $(SRC)/server.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

hash.o : $(SRC)/hash.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

charstack.o : $(SRC)/charstack.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

globals.o : $(SRC)/globals.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

libssl.o : $(SRC)/libssl.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

# admin tool

cdpnntpadm.o : $(SRC)/cdpnntpadm.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) \
	-DTHIS_TOOLNAME=\"wendzelnntpd\" -c $<

wendzelnntpadm : cdpnntpadm.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) $(POSTGRESOBJ) log.o hash.o server.o lex.yy.o config.tab.o charstack.o cdpstrings.o database.o acl.o libfunc.o $(OPENSSLOBJ) globals.o
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpadm cdpnntpadm.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) $(POSTGRESOBJ) log.o server.o hash.o lex.yy.o config.tab.o charstack.o cdpstrings.o database.o acl.o libfunc.o globals.o $(OPENSSLOBJ) $(SQLITELIB) $(MYSQLLIB) $(POSTGRESLIB) $(LIBDIRS) $(SOLNETLIBS) $(GCCLOCALPTHREAD) $(LIBPTHREAD) $(LIBMHASH) $(OPENSSLLIB)
	#strip bin/wendzelnntpadm

# misc targets

install : bin/wendzelnntpd bin/wendzelnntpadm
	if [ ! -d $(FAKECDIR) ]; then install -d -m 0755 $(FAKECDIR); fi
	if [ ! -d $(DESTDIR)/sbin ]; then install -d -m 0755 $(DESTDIR)/sbin; fi
	if [ ! -d $(DESTDIR)/share ]; then install -d -m 0755 $(DESTDIR)/share; fi
	if [ ! -d $(DESTDIR)/share/doc ]; then install -d -m 0755 $(DESTDIR)/share/doc; fi
	if [ ! -d $(DESTDIR)/share/doc/wendzelnntpd ]; then install -d -m 0755 $(DESTDIR)/share/doc/wendzelnntpd; fi
	if [ ! -d $(DESTDIR)/share/man/man8 ]; then install -d -m 0755 $(DESTDIR)/share/man/man8; fi
	# binaries
	cp bin/wendzelnntpd bin/wendzelnntpadm $(DESTDIR)/sbin/
	chown 0:0 $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	chmod 0755 $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	# documentation and config files
	cp $(DOCFILES_TO_INST) $(DESTDIR)/share/doc/wendzelnntpd/
	cp -r docs/docs $(DESTDIR)/share/doc/wendzelnntpd/
	cp docs/docs.pdf $(DESTDIR)/share/doc/wendzelnntpd/docs.pdf
	chown 0:0 $(DESTDIR)/share/doc/wendzelnntpd/*
	chmod 0644 $(DESTDIR)/share/doc/wendzelnntpd/*
	# manpages
	cp $(MANPAGES) $(DESTDIR)/share/man/man8/
	chmod 644 $(DESTDIR)/share/man/man8/wendzelnntpd.8
	chmod 644 $(DESTDIR)/share/man/man8/wendzelnntpadm.8
	# config
	@if [ -f $(FAKECDIR)/wendzelnntpd.conf ]; then cp $(FAKECDIR)/wendzelnntpd.conf $(FAKECDIR)/wendzelnntpd.conf.bkp; chmod 0644 $(FAKECDIR)/wendzelnntpd.conf.bkp; echo "***Your old wendzelnntpd.conf was backuped!***"; fi
	cp $(CONFFILE) $(FAKECDIR)/wendzelnntpd.conf
	chown 0:0 $(FAKECDIR)/wendzelnntpd.conf
	chmod 0644 $(FAKECDIR)/wendzelnntpd.conf
	# nextmsgid and database/usenet.db are placed here:
	mkdir -p /var/spool/news/wendzelnntpd
	# og-rwx since the passwords are stored in the database too!
	chmod 700 /var/spool/news/wendzelnntpd
	# create a backup of the old usenet database, if needed (only if not dev-mode)
	@if [ -f $(UDBFILE) ] && [ $(CONFFILE) != *"dev"* ]; then mv $(UDBFILE) $(UDBFILE).`date +"%m-%d-%y-%H:%M"`.bkp; chmod 0600 $(UDBFILE).`date +"%m-%d-%y-%H:%M"`.bkp; echo "***Your old usenet database was backuped!***"; fi
			
	@# create new database, dir already exists due to earlier mkdir call
	install -d -m 0700 $(CMD_INSTALL_USEROPT) 0 -g 0 /var/spool/news/wendzelnntpd
	@#
	@# create sqlite initial database if Sqlite3 is used
	@# AND
	@# create initial newsgroup for sqlite3
	@#
	@# only create if there is no database file already
	@if [ ! -f $(UDBFILE) ]; then if [ "$(SQLITEINST)" != "NO" ]; then echo "Setting up sqlite3 database ..."; cat database/usenet.db_struct | sqlite3 $(UDBFILE) && ( ./bin/wendzelnntpadm addgroup alt.wendzelnntpd.test y || echo "no new newsgroup created." ); else echo "*** NO sqlite3 database setup performed (you use MySQL). Please read the manual (docs/docs.pdf) to learn how to set up the MySQL database within a few minutes. ***"; fi; fi
	@echo "Installation finished. Please note that your existing wendzelnntpd.conf might have been replaced (a backup should be located in the same folder as your original configuration file)."
	@echo "Thank you for using this software! Have fun using it!"

upgrade : bin/wendzelnntpd bin/wendzelnntpadm
	@echo "*** Please only upgrade your WendzelNNTPd if your existing installation is WendzelNNTPd version 2.0.0 or newer. This script replaces only binaries and documentation files. Your databases and configuration files remain untouched. Press RETURN to perform an upgrade or press CTRL+C to abort. ***"
	@read uselessinput
	# binaries
	cp bin/wendzelnntpd bin/wendzelnntpadm $(DESTDIR)/sbin/
	chown 0:0 $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	chmod 0755 $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	# documentation
	cp -r docs/docs $(DESTDIR)/share/doc/wendzelnntpd/
	cp docs/docs.pdf $(DESTDIR)/share/doc/wendzelnntpd/docs.pdf
	chown 0:0 $(DESTDIR)/share/doc/wendzelnntpd/*
	chmod 0644 $(DESTDIR)/share/doc/wendzelnntpd/*
	@echo "Upgrade finished. Thank you for upgrading and using this software. Have fun!"

exec : bin/wendzelnntpd
	./bin/wendzelnntpd

db : bin/wendzelnntpd
	gdb ./bin/wendzelnntpd

count : clean
	wc -l `find . -name '*.[chyl]'` | $(SORT)

clean :
	rm -f bin/wendzelnntpd bin/wendzelnntpadm *.core `find . -name '*.o'` \
	y.output gpmon.out log *.BAK lex.*.[ch] *.tab.[ch] `find . -name '*~'` \
	config.output temp.c
	@# documentation cleanup
	rm -f docs/docs.ilg docs/docs.ind docs/*.idx docs/*.aux docs/*.toc \
	docs/*.log docs/docs.out \
	docs/docs/*.tex docs/docs/*.pl docs/docs/*.log docs/docs/*.idx \
	docs/docs/WARNINGS docs/docs/*.old docs/docs/*.aux docs/docs/images.out \
	docs/docs/images.pdf docs/docs/crossref.png

print_version :
	@/bin/sh ./getver.sh

man_wendzelnntpd :
	groff -Tascii -man wendzelnntpd.8  > test.man
	man -l test.man

man_wendzelnntpadm :
	groff -Tascii -man wendzelnntpadm.8  > test.man
	man -l test.man

docker-build:
	docker build -f ./docker/Dockerfile -t wendzelnntpd:latest .

docker-run:
	docker run --name wendzelnntpd --rm -it -p 119:119 -p 563:563 -v ${PWD}:/wendzelnntpd -v wendzelnntpd_data:/var/spool/news/wendzelnntpd wendzelnntpd:latest

docker-stop:
	docker stop wendzelnntpd
