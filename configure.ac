# WendzelNNTPd is distributed under the following license:
#
# Copyright (c) 2004-2025 Steffen Wendzel <steffen (at) wendzel (dot) de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.71])
AC_INIT([WendzelNNTPd],[2.2-alpha1], [steffen@wendzel.de])
releasename="Bad Woerishofen"
AC_SUBST([RELEASENAME], [$releasename])
AC_DEFINE_UNQUOTED([RELEASENAME], ["$releasename"]) # should not include white-spaces!
AC_COPYRIGHT([WendzelNNTPd is distributed under the GPLv3 license you can find a copy of the license in the 'LICENSE' file.])
AC_CONFIG_MACRO_DIR([m4])

# Checks for enabled features
AC_ARG_ENABLE([mysql], [AS_HELP_STRING([--disable-mysql], [Disables support for MySQL])])
AC_ARG_ENABLE([sqlite], [AS_HELP_STRING([--disable-sqlite], [Disables support for SQLite])])
AC_ARG_ENABLE([postgres], [AS_HELP_STRING([--enable-postgres], [Enables support for PostgreSQL])])
AC_ARG_ENABLE([tls], [AS_HELP_STRING([--disable-tls], [Disables support for TLS encryption])])

# Check if at least one of the databases is enabled
AS_IF([test "x$enable_mysql" = "xno" && test "x$enable_sqlite" = "xno" && test "x$enable_postgres" != "xyes"], [
	AC_MSG_ERROR([You cannot deactivate support for MySQL, PostgreSQL *and* SQLite as you need to integrate support for at least one database!])
])


AS_IF([test "x$enable_mysql" != "xno"], [
	AC_MSG_NOTICE([with MySQL support])
	AX_LIB_MYSQL
	AS_IF([test "x$MYSQL_VERSION" = "x"], [
		AC_MSG_ERROR([You need the MySQL client library and their development files (or disable MySQL support)])
	])
	AC_SUBST([MYSQLOBJ], [db_mysql.o])
], [AC_DEFINE([NOMYSQL])])

AS_IF([test "x$enable_sqlite" != "xno"], [
	AC_MSG_NOTICE([with SQLite support])
	AX_LIB_SQLITE3
	AS_IF([test "x$SQLITE3_VERSION" = "x"], [
		AC_MSG_ERROR([You need libSQLite3 development files and library (or disable SQLite3 support)])
	])
	AC_CHECK_PROGS([SQLITE], [sqlite3])
	AS_IF([test "x$SQLITE" = "x"], [
		AC_MSG_ERROR([You need sqlite3 in your PATH])
	])
	AC_SUBST([SQLITEOBJ], [db_sqlite3.o])
], [
	AC_DEFINE([NOSQLITE])
	AC_SUBST([SQLITEINST], [NO])
])

AS_IF([test "x$enable_postgres" = "xyes"], [
	AC_MSG_NOTICE([with PostgreSQL support])
	AX_LIB_POSTGRESQL
	AC_SUBST([POSTGRESOBJ], [db_postgres.o])
], [AC_DEFINE([NOPOSTGRES])])

AS_IF([test "x$enable_tls" != "xno"], [
	AC_MSG_NOTICE([with TLS support])
	AC_DEFINE([USE_TLS])
	AX_CHECK_OPENSSL([], [AC_MSG_ERROR([You need libssl])])
	AC_CHECK_PROGS([OPENSSL], [openssl])
	AS_IF([test "x$OPENSSL" = "x"], [
		AC_MSG_ERROR([You need openssl in your PATH])
	])
	AC_SUBST([OPENSSLOBJ], [libssl.o])
], [
	AC_SUBST([TLSINST], [NO])
])

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LEX(noyywrap)
AS_IF([test "x$LEX" != "xflex"], [
	AC_MSG_ERROR([You need flex])
])
AX_PROG_BISON([], [AC_MSG_ERROR([You need bison])])

# Checks for libraries.
AX_LIB_SOCKET_NSL
AX_PTHREAD([], [Could not find pthread support])
AC_CHECK_LIB([mhash], [mhash_init], [], [AC_MSG_ERROR([Could not find libmhash])])

AC_MSG_CHECKING([for SHA-256 support in libmhash])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([[#include <mhash.h>]], [[MHASH td = mhash_init(MHASH_SHA256);]])],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no); AC_MSG_ERROR([You need a version of libmhash that supports SHA-256.])]
)

# Checks for header files.
AC_CHECK_HEADER([stdio.h], [], [AC_MSG_ERROR([Could not find stdio.h header file])])

# Checks for library functions.
AC_CHECK_FUNC([strndup], [], AC_DEFINE([NOSUPPORT_STRNDUP]))
AC_CHECK_FUNC([localtime], [], [AC_MSG_ERROR([Could not find localtime])])
AC_CHECK_FUNC([strftime], [], [AC_MSG_ERROR([Could not find strftime])])
AC_CHECK_FUNC([time], [], [AC_MSG_ERROR([Could not find time])])

AC_MSG_CHECKING([for %z support in strftime])
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]],
[[char unused[0xff] = {'\0'};
time_t ltime;

ltime = time(NULL);
if (ltime == (time_t) - 1) {
	return 1;
}
strftime(unused, 0xff-1, "%z", localtime(&ltime));
]])], [AC_MSG_RESULT(yes)], [AC_MSG_RESULT(no); AC_DEFINE([NOSUPPORT_STRFTIME_z_FLAG])])

# Check for additional compile flags
AX_APPEND_COMPILE_FLAGS([-fstack-protector])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

AC_MSG_NOTICE([done (you can now run make (or gmake on OpenSolaris))])
