SORT=sort -bg
CMD_INSTALL_USEROPT=-o
SOLNETLIBS=
GCCLOCALPTHREAD=
LIBPTHREAD=-lpthread
MYSQLLIB=-lmysqlclient
MYSQLOBJ=db_mysql.o
SQLITELIB=-lsqlite3
SQLITEOBJ=db_sqlite3.o
TABCPATH=src/
ORDER=-DORDER=

STACK_PROT=-fstack-protector
ADD_CFLAGS=
ADD_LNKFLAGS=

DESTDIR=/usr/local
CONFDIR=/usr/local/etc
FAKECDIR=/usr/local/etc

SRC=src
LIBDIRS=-L/usr/local/lib

INCDIRS=-I. -I./src -I./src/include -I/usr/local/include
CPP=g++
CC=gcc
LEX=flex
YACC=bison


