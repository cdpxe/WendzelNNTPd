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
	-Wsign-compare -Wstrict-prototypes
#CFLAGS+=-Wextra -Wcast-align -Wunreachable-code
CFLAGS+=$(ADD_CFLAGS)

BUILD=-DBUILD=\"`cat build`\"
GDBON=-ggdb -g
#DEBUG=$(GDBON) -DDEBUG -DXXDEBUG
BUILDFLAGS=-O2 $(STACK_PROT) $(ADD_LNKFLAGS)

# The list of documentation files we wish to install
DOCFILES_TO_INST=AUTHOR CHANGELOG HISTORY README INSTALL LICENSE usenet.db_struct documentation.pdf

all : wendzelnntpadm main.o w32trial.o log.o database.o cdpstrings.o xml_out.o server.o lexyacc charstack.o libfunc.o
	expr `cat build` \+ 1 >build
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpd main.o log.o server.o lex.yy.o config.tab.o database.o cdpstrings.o w32trial.o xml_out.o charstack.o libfunc.o $(SOLNETLIBS) -lsqlite3 $(LIBDIRS) $(SOLNETLIBS) $(GCCLOCALPTHREAD) $(LIBPTHREAD)
	#strip bin/wendzelnntpd

lexyacc : lex.yy.o config.tab.o

lex.yy.o : $(SRC)/config.l $(HEADERS)
	$(LEX) src/config.l

config.tab.o : $(SRC)/config.y lex.yy.o $(HEADERS)
	$(YACC) -v -d -t -o config.tab.c $(SRC)/config.y
	$(CC) $(DEBUG) $(INCDIRS) $(DESTCFLAGS) -c config.tab.c
	$(CC) $(DEBUG) $(INCDIRS) $(DESTCFLAGS) -c lex.yy.c

database.o : $(SRC)/database.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

main.o : $(SRC)/main.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

log.o : $(SRC)/log.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

libfunc.o : $(SRC)/libfunc.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

w32trial.o : $(SRC)/w32trial.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

cdpstrings.o : $(SRC)/cdpstrings.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

xml_out.o : $(SRC)/xml_out.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

server.o : $(SRC)/server.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<
	
charstack.o : $(SRC)/charstack.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) $<

# admin tool

cdpnntpadm.o : $(SRC)/cdpnntpadm.c $(HEADERS)
	$(CC) $(DEBUG) $(BUILD) $(CFLAGS) $(INCDIRS) \
	-DTHIS_TOOLNAME=\"wendzelnntpd\" -c $<

wendzelnntpadm : cdpnntpadm.o
	$(CC) $(DEBUG) $(BUILDFLAGS) -o bin/wendzelnntpadm cdpnntpadm.o \
	-lsqlite3 $(LIBDIRS) $(SOLNETLIBS) $(CCCLOCALPTHREAD) $(LIBPTHREAD)
	strip bin/wendzelnntpadm

# misc targets

install : bin/wendzelnntpd bin/wendzelnntpadm
	if [ ! -d $(FAKECDIR) ]; then install -d -m 0755 $(FAKECDIR); fi
	if [ ! -d $(DESTDIR)/sbin ]; then install -d -m 0755 $(DESTDIR)/sbin; fi
	if [ ! -d $(DESTDIR)/share ]; then install -d -m 0755 $(DESTDIR)/share; fi
	if [ ! -d $(DESTDIR)/share/doc ]; then install -d -m 0755 $(DESTDIR)/share/doc; fi
	if [ ! -d $(DESTDIR)/share/doc/wendzelnntpd ]; then install -d -m 0755 $(DESTDIR)/share/doc/wendzelnntpd; fi
	# binaries
	cp bin/wendzelnntpd bin/wendzelnntpadm $(DESTDIR)/sbin/
	chown root:root $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	chmod 0755 $(DESTDIR)/sbin/wendzelnntpd $(DESTDIR)/sbin/wendzelnntpadm
	# config files
	cp $(DOCFILES_TO_INST) $(DESTDIR)/share/doc/wendzelnntpd/
	chown root:root $(DESTDIR)/share/doc/wendzelnntpd/*
	chmod 0644 $(DESTDIR)/share/doc/wendzelnntpd/*
	# config
	cp wendzelnntpd.conf $(FAKECDIR)/
	chown root:root $(FAKECDIR)/wendzelnntpd.conf
	chmod 0644 $(FAKECDIR)/wendzelnntpd.conf
	# nextmsgid and usenet.db are placed here:
	mkdir -p /var/spool/news/wendzelnntpd
	# og-rwx since the passwords are stored in the database too!
	chmod 700 /var/spool/news/wendzelnntpd
	# install the GUI (if available) ($(DESTDIR)/sbin already exists!)
	if [ -x gui/src/src ]; then cp gui/src/src $(DESTDIR)/sbin/wendzelnntpgui; chown root:root $(DESTDIR)/sbin/wendzelnntpgui; chmod 0755 $(DESTDIR)/sbin/wendzelnntpgui; fi
	# create a backup of the old usenet database, if needed
	if [ -f $(UDBFILE) ]; then mv $(UDBFILE) $(UDBFILE).`date +"%m-%d-%y-%H:%M:%S"`.bkp; chmod 0600 $(UDBFILE).`date +"%m-%d-%y-%H:%M:%S"`.bkp; echo "Your old usenet database was backuped!"; fi
	# create new database, dir already exists since mkdir call before
	install -d -m 0700 $(CMD_INSTALL_USEROPT) root -g root /var/spool/news/wendzelnntpd
	cat usenet.db_struct | sqlite3 $(UDBFILE)
	# create 1st newsgroup
	@# catch error return code of wendzelnntpadm if newsgroup does already exist
	( ./bin/wendzelnntpadm create alt.wendzelnntpd.test y || echo "finished" )

exec : bin/wendzelnntpd
	./bin/wendzelnntpd

db : bin/wendzelnntpd
	gdb ./bin/wendzelnntpd

count : clean
	wc -l `find . -name '*.[chyl]'` ./gui/src/gui.cpp | $(SORT)

clean :
	rm -f bin/wendzelnntpd bin/wendzelnntpadm *.core `find . -name '*.o'` \
	y.output gpmon.out log *.BAK lex.*.[ch] *.tab.[ch] `find . -name '*~'` \
	config.output
	(cd gui/src && rm -f doesnotexist `/bin/ls *.h *.cpp | grep -v gui` )
	rm -f gui/src/src


