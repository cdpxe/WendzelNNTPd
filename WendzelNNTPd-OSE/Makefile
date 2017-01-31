#
# WendzelNNTPd is distributed under the GPLv3 license you can find a
# copy of the license in the 'LICENSE' file.
#

include Makefile.inc

UDBFILE=/var/spool/news/wendzelnntpd/usenet.db
DESTCFLAGS=-DCONFDIR=\"$(CONFDIR)\"
HEADERS=$(SRC)/include/cdpstrings.h $(SRC)/include/main.h $(SRC)/include/wendzelnntpdpath.h
CFLAGS= -c -Wall $(STACK_PROT) $(DESTCFLAGS)
CFLAGS+=-Wmissing-prototypes -Wmissing-declarations -Wshadow -Wcast-qual \
	-Wsign-compare -Wstrict-prototypes -Wcast-align #\
#	-pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror-Wno-unused
#CFLAGS+=-Wextra -Wunreachable-code
CFLAGS+=$(ADD_CFLAGS)

BUILD=-DBUILD=\"`cat build`\"
GDBON=-ggdb -g

#DEBUG=$(GDBON) -DDEBUG -DXXDEBUG

BUILDFLAGS=-O2 $(STACK_PROT) $(ADD_LNKFLAGS)

# The list of documentation files we wish to install
DOCFILES_TO_INST=AUTHOR CHANGELOG HISTORY README INSTALL LICENSE usenet.db_struct mysql_db_struct.sql

all : wendzelnntpadm main.o db_rawcheck.o log.o database.o cdpstrings.o server.o lexyacc charstack.o libfunc.o acl.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) globals.o
	expr `cat build` \+ 1 >build
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpd main.o log.o server.o lex.yy.o config.tab.o database.o globals.o cdpstrings.o db_rawcheck.o acl.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) charstack.o libfunc.o $(SOLNETLIBS) $(SQLITELIB) $(MYSQLLIB) $(LIBDIRS) $(SOLNETLIBS) $(GCCLOCALPTHREAD) $(LIBPTHREAD)
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

db_mysql.o : $(SRC)/db_mysql.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

cdpstrings.o : $(SRC)/cdpstrings.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

server.o : $(SRC)/server.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

charstack.o : $(SRC)/charstack.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

globals.o : $(SRC)/globals.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

# admin tool

cdpnntpadm.o : $(SRC)/cdpnntpadm.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) \
	-DTHIS_TOOLNAME=\"wendzelnntpd\" -c $<

wendzelnntpadm : cdpnntpadm.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) log.o server.o lex.yy.o config.tab.o charstack.o cdpstrings.o database.o acl.o libfunc.o globals.o
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpadm cdpnntpadm.o db_abstraction.o $(SQLITEOBJ) $(MYSQLOBJ) log.o server.o lex.yy.o config.tab.o charstack.o cdpstrings.o database.o acl.o libfunc.o globals.o $(SQLITELIB) $(MYSQLLIB) $(LIBDIRS) $(SOLNETLIBS) $(CCCLOCALPTHREAD) $(LIBPTHREAD)
	#strip bin/wendzelnntpadm

# misc targets

install : bin/wendzelnntpd bin/wendzelnntpadm
	if [ ! -d $(FAKECDIR) ]; then install -d -m 0755 $(FAKECDIR); fi
	if [ ! -d $(DESTDIR)/sbin ]; then install -d -m 0755 $(DESTDIR)/sbin; fi
	if [ ! -d $(DESTDIR)/share ]; then install -d -m 0755 $(DESTDIR)/share; fi
	if [ ! -d $(DESTDIR)/share/doc ]; then install -d -m 0755 $(DESTDIR)/share/doc; fi
	if [ ! -d $(DESTDIR)/share/doc/wendzelnntpd ]; then install -d -m 0755 $(DESTDIR)/share/doc/wendzelnntpd; fi
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
	# config
	cp wendzelnntpd.conf $(FAKECDIR)/
	chown 0:0 $(FAKECDIR)/wendzelnntpd.conf
	chmod 0644 $(FAKECDIR)/wendzelnntpd.conf
	# nextmsgid and usenet.db are placed here:
	mkdir -p /var/spool/news/wendzelnntpd
	# og-rwx since the passwords are stored in the database too!
	chmod 700 /var/spool/news/wendzelnntpd
	# create a backup of the old usenet database, if needed
	@if [ -f $(UDBFILE) ]; then mv $(UDBFILE) $(UDBFILE).`date +"%m-%d-%y-%H:%M:%S"`.bkp; chmod 0600 $(UDBFILE).`date +"%m-%d-%y-%H:%M:%S"`.bkp; echo "***Your old usenet database was backuped!***"; fi
	@# create new database, dir already exists due to earlier mkdir call
	install -d -m 0700 $(CMD_INSTALL_USEROPT) 0 -g 0 /var/spool/news/wendzelnntpd
	@#
	@# create sqlite initial database if Sqlite3 is used
	@# AND
	@# create initial newsgroup for sqlite3
	@#
	@if [ "$(SQLITEINST)" != "NO" ]; then echo "Setting up sqlite3 database ..."; cat usenet.db_struct | sqlite3 $(UDBFILE) && ( ./bin/wendzelnntpadm addgroup alt.wendzelnntpd.test y || echo "no new newsgroup created." ); else echo "*** NO sqlite3 database setup performed (you use MySQL). Please read the manual (docs/docs.pdf) to learn how to set up the MySQL database within a few minutes. ***"; fi
	@echo "Installation finished."

upgrade : bin/wendzelnntpd bin/wendzelnntpadm
	@echo "*** Please only upgrade if you run WendzelNNTPd 2.0.0 or newer. This script replaces only binaries and documentation files. Your databases and configuration file remain untouched. Press RETURN to perform an upgrade or CTRL+C to abort. ***"
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
	@echo "Upgrade finished."

exec : bin/wendzelnntpd
	./bin/wendzelnntpd

db : bin/wendzelnntpd
	gdb ./bin/wendzelnntpd

count : clean
	wc -l `find . -name '*.[chyl]'` | $(SORT)

clean :
	rm -f bin/wendzelnntpd bin/wendzelnntpadm *.core `find . -name '*.o'` \
	y.output gpmon.out log *.BAK lex.*.[ch] *.tab.[ch] `find . -name '*~'` \
	config.output
	@# documentation cleanup
	rm -f docs/docs.ilg docs/docs.ind docs/*.idx docs/*.aux docs/*.toc \
	docs/*.log \
	docs/docs/*.tex docs/docs/*.pl docs/docs/*.log docs/docs/*.idx \
	docs/docs/WARNINGS docs/docs/*.old docs/docs/*.aux

print_version :
	@/bin/sh ./getver.sh


