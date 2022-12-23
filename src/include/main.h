/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2021 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
 * http://www.wendzel.de
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WENDZELNNTPD_H
   #error "main.h already included."
#else
   #define WENDZELNNTPD_H
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef __WIN32__
   #include <sys/wait.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
#endif
#include <signal.h>
#ifndef __WIN32__
   #include <sys/select.h>
#endif
#include <sys/time.h>
#include <pthread.h>
#include <stdarg.h>
#ifndef __WIN32__
   #include <syslog.h>
#endif
#include <errno.h>
#ifdef __linux__
#include <err.h>
#endif
#include <sys/stat.h> /* umask() */

#ifdef __svr4__
   #include <strings.h>	
#endif
#ifndef __WIN32__
   #include <netdb.h>
#endif
#ifdef __WIN32__
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <assert.h>

/* precondition */

#if defined(NOSQLITE) && defined(NOMYSQL)
	#error "You need at least one database but excluded MySQL *and* SQlite support!"
#endif

/* SQlite3 */
#ifndef NOSQLITE
	#include <sqlite3.h>
#endif

/* MySQL */
#ifndef NOMYSQL
	#include <mysql/mysql.h>
#endif

/* GnuTLS */
#include <gnutls/gnutls.h>

/* Own files */
#include "wendzelnntpdpath.h"

#if defined(__svr4__) || defined(__WIN32__)
   typedef uint8_t u_int8_t;
   typedef uint16_t u_int16_t;
   typedef uint32_t u_int32_t;

   #define BYTE_ORDER		ORDER
   #define BIG_ENDIAN		0
   #define LITTLE_ENDIAN	1
#endif

#ifdef __WIN32__
   #define bzero(a,b)		memset(a, 0x0, b)
#endif

/* debugging stuff */
#ifdef DEBUG
    #define DBMODE		" in DEBUG mode"
#else
    #define DBMODE		""
#endif

#define VERSION			"2.1.4-unstable"
#define RELEASENAME		"'St.-Peter-Ording'" /* should not include white-spaces! */

#ifndef BUILD /* Win32 */
   #define BUILD		"win"
#endif

#define DTB			" - (" __DATE__ " " __TIME__ " #" BUILD ")"
#define WELCOMEVERSION		VERSION " " RELEASENAME " " DTB

#define TRUE			1
#define FALSE			0

#define OK_EXIT			0x00
#define OK_RETURN		OK_EXIT
#define ERR_EXIT		0x01
#define ERR_RETURN		ERR_EXIT
#define NOSUCHFILE_RETURN	0x02
#define INSECURE_RETURN		0x03


/* for timestamp in database.c */
#define MAX_IDNUM_LEN      160

/* The "hostname" (it not really is one) to set in the message-ID if the
 * server runs in anonymous-message-ID-mode */
#define NNTPD_ANONYM_HOST	"WendzelNNTPdAnonymous"

/* Should yacc code parse everything needed for the server or just the
 * parts needed by the admin tool?
 */
#define PARSER_MODE_SERVER	0
#define PARSER_MODE_ADMTOOL	1

/* for select */
#ifndef __WIN32__
   #define max(a, b)		(a > b ? a : b)
#endif
#define FAM_4			1
#define FAM_6			0

#define CONN_LOGSTR_LEN		128
#define IPv6ADDRLEN		48 /* 40 should be enough */

#define DEFAULTPORT		119

#define DEFAULT_TLS_PORT 563

#define STACK_FOUND		0x00
#define STACK_NOTFOUND		0x01

#define MODE_PROCESS		0x01
#define MODE_THREAD		0x02

#define DBASE_NONE		0
#define DBASE_SQLITE3		1
#define DBASE_MYSQL		2
/*#define DBASE_POSTGRES		3*/

#define XHDR_FROM		0x01
#define XHDR_DATE		0x02
#define XHDR_NEWSGROUPS		0x03
#define XHDR_SUBJECT		0x04
#define XHDR_LINES		0x05

#define CMDTYP_ARTICLE		0x01
#define CMDTYP_HEAD		0x02
#define CMDTYP_BODY		0x03
#define CMDTYP_STAT		0x04

#define CMDTYP_LIST		0x11
#define CMDTYP_XGTITLE		0x12
#define CMDTYP_LIST_NEWSGROUPS	0x13

#define ARTCLTYP_MESSAGEID	0x01
#define ARTCLTYP_NUMBER		0x02
#define ARTCLTYP_CURRENT	0x03  /* > ARTICLE\r\n -> return the currently selected article */

#define PR_STRING		"WendzelNNTPd: "
#define PROMPT(x)		printf(PR_STRING x "\n")

/* rename it since we don't use syslog now: DO_LOGSTR */
#define DO_SYSL(fs)		{						\
				 char __q__[1024] = { '\0' };			\
				 strncpy(__q__, __func__, sizeof(__q__) - 1);	\
				 logstr(__FILE__, __LINE__, __q__, fs);		\
				}

#define SWITCHIP(i, v4, v6)	((sockinfo+i)->family == AF_INET ? v4 : v6)

#define FFLUSH			{ fflush(stdout);fflush(stdin);fflush(stderr); }

#define DEBUG_OUT(__stri_)	{						\
				 int __si;					\
				 for (__si = 0; __si < strlen(__stri_); __si++){\
					if (__stri_[__si] == '\r') putchar('R');\
					else putchar(__stri_[__si]);		\
				 }						\
				}

/* syslog stuff */
/*#define XY_SYSL_NOTICE	LOG_NOTICE|LOG_DAEMON*/
#define XY_SYSL_ERR	LOG_ERR|LOG_DAEMON
#define XY_SYSL_NOTICE	XY_SYSL_ERR           /* log ERR instead */
#define XY_SYSL		XY_SYSL_ERR
#define XY_SYSL_WARN	LOG_WARNING|LOG_DAEMON

/* This CALLOC exits (!!) on error. Don't use this with threads! */
#define CALLOC(var, cast, num, size_of)					\
	if((var= cast calloc(num, size_of))==NULL) {			\
		DO_SYSL("couldn't allocate a datatype! mem-low.\n");	\
      		sig_handler(0);					      	\
	}
/* This CALLOC only kills the thread (!) */
#define CALLOC_Thread(_inf, var, cast, num, size_of)			\
	if((var= cast calloc(num, size_of))==NULL) {			\
		DO_SYSL("couldn't allocate a datatype! mem-low.\n");	\
      		kill_thread(_inf);					\
	}

#define CALLOC_Process(var, cast, num, size_of)				\
	if((var= cast calloc(num, size_of))==NULL) {			\
		fprintf(stderr, "couldn't allocate a datatype! mem-low.\n");\
		exit(ERR_EXIT);						\
	}

#ifndef BYTE_ORDER
   #error no BYTE_ORDER defined.
#endif

/*******************************************************************/

typedef struct {
	int		sockfd;
	int		family;
	struct sockaddr_in  sa;
	struct sockaddr_in6 sa6;
	char		ip[IPv6ADDRLEN];
} sockinfo_t;

typedef struct {
	int		auth_is_there;	/* is the client already authenticated? */
	int		tls_is_there;	/* do we have active TLS encryption? */
  int   switch_to_tls; /* started migration to TLS */
  gnutls_session_t tls_session; /* saves the current TLS session */
	char		*cur_auth_user;
	char		*cur_auth_pass;
	
	char		*selected_group;
	char		*selected_article;
	
	char		*curstring;
	
	/* for POST */
	char		*chkname;
	int		found_group;
	
	/* for ARTICLE */
	int		found_article;
	
	/* for LIST NEWSGROUPS/XGTITLE in Sqlite3 */
	char		*wildmat;
	
	/* gerneral purpose */
	int		counter;
	
	/* SQLite stuff */
#ifndef NOSQLITE
	sqlite3		*db;
	char		*sqlite_err_msg;
#endif
#ifndef NOMYSQL	
	/* MySQL stuff */
	MYSQL		*myhndl;
#endif
} serverinfo_t;

typedef struct {
	sockinfo_t	*sockinf;
	serverinfo_t	*servinf;
	/* used for some cb functions */
	int		cmdtype;	/* article */
#define SPECCMD_DATE	0x0001
	int		speccmd;	/* special cmd for callback functions */

} server_cb_inf;

struct charstack {
	char		*value;
	struct charstack*next;
#define STACK_EMPTY	0x0000
#define STACK_HASDATA	0x0001
	u_int8_t	state;
};
typedef struct charstack charstack_t;

/***************************** **************************************/

/* global functions */

/* config.y */
void basic_setup_server(void);
void basic_setup_admtool(void);

/* database.c */
int chk_file_sec(char *);
int get_openfilelen(FILE *);
char *get_uniqnum(void);
short int filebackend_savebody(char *, char *);
char *filebackend_retrbody(char *);

/* globals.c */
void sig_handler(int);

/* server.c */
void ToSend(char *, int, server_cb_inf *);
void Recv(int sockfd, char *buf, int len);
void *do_server(void *);
void kill_thread(server_cb_inf *);
void nntp_localtime_to_str(char [40], time_t);

/* tls.c */
int tls_global_init();
void tls_global_close();
int tls_session_init(gnutls_session_t *session, int sockfd);
void tls_session_close(gnutls_session_t session);

/* db_abstraction.c */
void db_open_connection(server_cb_inf *);
void db_close_connection(server_cb_inf *);
char *db_secure_sqlbuffer(server_cb_inf *, char *);
void db_secure_sqlbuffer_free(char *);
short db_acl_check_user_group(server_cb_inf *, char *, char *);
void db_authinfo_check(server_cb_inf *);
void db_list(server_cb_inf *, int, char *);
void db_xhdr(server_cb_inf *, short, int, char *, u_int32_t,
	u_int32_t);
void db_article(server_cb_inf *, int, char *);
void db_group(server_cb_inf *, char *);
void db_listgroup(server_cb_inf *, char *);
void db_xover(server_cb_inf *, u_int32_t, u_int32_t);
u_int32_t db_get_high_value(server_cb_inf *, char *);
int db_chk_if_msgid_exists(server_cb_inf *, char *, char *);
void db_check_newsgroup_posting_allowed(server_cb_inf *);
void db_check_newsgroup_existence(server_cb_inf *);
void db_check_user_existence(server_cb_inf *);
void db_check_role_existence(server_cb_inf *);
void db_post_insert_into_postings(server_cb_inf *, char *, time_t,
	char *, char *, char *, int, char *);
void db_post_update_high_value(server_cb_inf *, u_int32_t, char *);
void db_post_insert_into_ngposts(server_cb_inf *, char *, char *,
	u_int32_t);
void db_list_users(server_cb_inf *);
void db_list_acl_tables(server_cb_inf *);
void db_create_newsgroup(server_cb_inf *, char *, char);
void db_delete_newsgroup(server_cb_inf *, char *);
void db_modify_newsgroup(server_cb_inf *, char *, char);
void db_add_user(server_cb_inf *, char *, char *);
void db_del_user(server_cb_inf *, char *);
void db_acl_add_user(server_cb_inf *, char *, char *);
void db_acl_del_user(server_cb_inf *, char *, char *);
void db_acl_add_role(server_cb_inf *, char *);
void db_acl_del_role(server_cb_inf *, char *);
void db_acl_role_connect_group(server_cb_inf *, char *, char *);
void db_acl_role_disconnect_group(server_cb_inf *, char *, char *);
void db_acl_role_disconnect_user(server_cb_inf *, char *, char *);
void db_acl_role_connect_user(server_cb_inf *, char *, char *);

/* db_sqlite3.c */
#ifndef NOSQLITE
void db_sqlite3_open_connection(server_cb_inf *);
void db_sqlite3_close_connection(server_cb_inf *);
void sqlite3_secexec(server_cb_inf *, char *,
	int (*)(void *, int, char **, char **), void *);
void db_sqlite3_authinfo_check(server_cb_inf *);
void db_sqlite3_list(server_cb_inf *, int, char *);
void db_sqlite3_xhdr(server_cb_inf *, short, int, char *, u_int32_t,
	u_int32_t);
void db_sqlite3_article(server_cb_inf *, int, char *);
void db_sqlite3_group(server_cb_inf *, char *);
void db_sqlite3_listgroup(server_cb_inf *, char *);
void db_sqlite3_xover(server_cb_inf *, u_int32_t, u_int32_t);
u_int32_t db_sqlite3_get_high_value(server_cb_inf *, char *);
int db_sqlite3_chk_if_msgid_exists(server_cb_inf *, char *, char *);
void db_sqlite3_chk_newsgroup_posting_allowed(server_cb_inf *);
void db_sqlite3_chk_newsgroup_existence(server_cb_inf *); /* for POST AND for admin tool */
void db_sqlite3_chk_user_existence(server_cb_inf *);
void db_sqlite3_chk_role_existence(server_cb_inf *);
void db_sqlite3_post_insert_into_postings(server_cb_inf *, char *,
	time_t, char *, char *, char *, int , char *);
void db_sqlite3_post_update_high_value(server_cb_inf *, u_int32_t,
	char *);
void db_sqlite3_post_insert_into_ngposts(server_cb_inf *, char *,
	char *, u_int32_t);
short db_sqlite3_acl_check_user_group(server_cb_inf *, char *, char *);
void db_sqlite3_list_users(server_cb_inf *);
void db_sqlite3_list_acl_tables(server_cb_inf *);
void db_sqlite3_create_newsgroup(server_cb_inf *, char *, char);
void db_sqlite3_delete_newsgroup(server_cb_inf *, char *);
void db_sqlite3_modify_newsgroup(server_cb_inf *, char *, char);
void db_sqlite3_add_user(server_cb_inf *, char *, char *);
void db_sqlite3_del_user(server_cb_inf *, char *);
void db_sqlite3_acl_add_user(server_cb_inf *, char *, char *);
void db_sqlite3_acl_del_user(server_cb_inf *, char *, char *);
void db_sqlite3_acl_add_role(server_cb_inf *, char *);
void db_sqlite3_acl_del_role(server_cb_inf *, char *);
void db_sqlite3_acl_role_connect_group(server_cb_inf *, char *, char *);
void db_sqlite3_acl_role_disconnect_group(server_cb_inf *, char *, char *);
void db_sqlite3_acl_role_connect_user(server_cb_inf *, char *, char *);
void db_sqlite3_acl_role_disconnect_user(server_cb_inf *, char *, char *);
#endif

/* db_mysql.c */
#ifndef NOMYSQL
void db_mysql_open_connection(server_cb_inf *);
void db_mysql_close_connection(server_cb_inf *);
void db_mysql_authinfo_check(server_cb_inf *);
u_int32_t db_mysql_get_high_value(server_cb_inf *, char *);
int db_mysql_chk_if_msgid_exists(server_cb_inf *, char *, char *);
void db_mysql_chk_newsgroup_posting_allowed(server_cb_inf *);
void db_mysql_chk_newsgroup_existence(server_cb_inf *);
void db_mysql_create_newsgroup(server_cb_inf *, char *, char);
void db_mysql_delete_newsgroup(server_cb_inf *, char *);
void db_mysql_modify_newsgroup(server_cb_inf *, char *, char);
void db_mysql_list(server_cb_inf *, int, char *);
void db_mysql_xhdr(server_cb_inf *, short, int, char *, u_int32_t,
		u_int32_t);
void db_mysql_article(server_cb_inf *, int, char *);
void db_mysql_group(server_cb_inf *, char *);
void db_mysql_listgroup(server_cb_inf *, char *);
void db_mysql_xover(server_cb_inf *, u_int32_t, u_int32_t);
void db_mysql_post_insert_into_postings(server_cb_inf *, char *,
	time_t, char *, char *, char *, int , char *);
void db_mysql_post_update_high_value(server_cb_inf *, u_int32_t,
	char *);
void db_mysql_post_insert_into_ngposts(server_cb_inf *, char *,
	char *, u_int32_t);
void db_mysql_list_users(server_cb_inf *);
void db_mysql_chk_user_existence(server_cb_inf *);
void db_mysql_chk_role_existence(server_cb_inf *);
void db_mysql_add_user(server_cb_inf *, char *, char *);
void db_mysql_del_user(server_cb_inf *, char *);
void db_mysql_acl_add_user(server_cb_inf *, char *, char *);
void db_mysql_acl_del_user(server_cb_inf *, char *, char *);
short db_mysql_acl_check_user_group(server_cb_inf *, char *, char *);
void db_mysql_list_acl_tables(server_cb_inf *);
void db_mysql_chk_role_existence(server_cb_inf *);
void db_mysql_acl_add_role(server_cb_inf *, char *);
void db_mysql_acl_del_role(server_cb_inf *, char *);
void db_mysql_acl_role_connect_group(server_cb_inf *, char *, char *);
void db_mysql_acl_role_disconnect_group(server_cb_inf *, char *, char *);
void db_mysql_acl_role_connect_user(server_cb_inf *, char *, char *);
void db_mysql_acl_role_disconnect_user(server_cb_inf *, char *, char *);
#endif


/* log.c */
void onxxdebug(char *str);
void onxxdebugm(char *str, ...);
void logstr(char *, int, char *, char *);

/* db_rawcheck.c */
void check_db(void);

/* libfunc.c */
#if defined(__WIN32__) || defined(NOSUPPORT_STRNDUP)
char *strndup(char *, int);
#endif
char *str_concat(char *, char *, char *, char *, char *);
int wnntpd_rx_contain(char *, char *);

/* charstack.c */
int charstack_check_for(charstack_t *, char *);
int charstack_push_on(charstack_t *, char *);
void charstack_free(charstack_t *);

/* acl.c */
short acl_check_user_group(server_cb_inf *, char *, char *);

/* hash.c */
char *get_sha256_hash_from_str(char *, char *);
