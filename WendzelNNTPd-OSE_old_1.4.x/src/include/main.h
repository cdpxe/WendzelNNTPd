/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2010 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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

#include <sqlite3.h>

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

#define VERSION			"1.4.7"
#define RELEASENAME		"'Pluto'"

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


/* for timestamp in database.c */
#define MAX_IDNUM_LEN      160

/* The "hostname" (it not really is one) to set in the message-ID if the
 * server runs in anonymous-message-ID-mode */
#define NNTPD_ANONYM_HOST	"WendzelNNTPdAnonymous"

/* for select */
#ifndef __WIN32__
   #define max(a, b)		(a > b ? a : b)
#endif
#define FAM_4			1
#define FAM_6			0

#define CONN_LOGSTR_LEN		128
#define IPv6ADDRLEN		48 /* 40 should be enough */

#define DEFAULTPORT		119

#define STACK_FOUND		0x0000
#define STACK_NOTFOUND		0x0001

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
#define CALLOC_Thread(var, cast, num, size_of)				\
	if((var= cast calloc(num, size_of))==NULL) {			\
		DO_SYSL("couldn't allocate a datatype! mem-low.\n");	\
      		/* TODO: exit thread here */exit(1);			\
	}

#ifndef BYTE_ORDER
   #error no BYTE_ORDER defined.
#endif

/*******************************************************************/

typedef struct {
	int		 sockfd;
	int		 family;
	struct sockaddr_in  sa;
	struct sockaddr_in6 sa6;
	char		 ip[IPv6ADDRLEN];
} sockinfo_t;

typedef struct {
	int		 auth_is_there;	/* is the client already authenticated? */
	char		*cur_auth_user;
	char		*cur_auth_pass;
	
	char		*selected_group;
	char		*selected_article;
	
	char		*curstring;
	
	/* for POST */
	char		*group_chkname;
	int		 found_group;
	
	/* for ARTICLE */
	int		 found_article;
	
	/* SQLite stuff */
	sqlite3		*db;
	char		*sqlite_err_msg;
} serverinfo_t;

typedef struct {
	sockinfo_t	*sockinf;
	serverinfo_t	*servinf;
	/* used for some cb functions */
	int		 cmdtype;	/* article */
#define SPECCMD_DATE	0x0001
	int		 speccmd;	/* special cmd for callback functions */

} server_cb_inf;

struct charstack {
	char		*value;
	struct charstack*next;
#define STACK_EMPTY	0x0000
#define STACK_HASDATA	0x0001
	u_int8_t	 state;
};
typedef struct charstack charstack_t;

/***************************** **************************************/

/* global functions */

/* config.y */
void basic_setup(void);

/* database.c */
int chk_file_sec(char *);
char *get_uniqnum(void);
u_int32_t DB_get_high_value(char *, server_cb_inf *);
int DB_chk_if_msgid_exists(char *, char *, server_cb_inf *);
short int filebackend_savebody(char *, char *);
char *filebackend_retrbody(char *);

/* main.c */
void sig_handler(int);

/* server.c */
void ToSend(char *, int, server_cb_inf *);
void Recv(int sockfd, char *buf, int len);
void *do_server(void *);
void kill_thread(server_cb_inf *);
void sqlite3_secexec(server_cb_inf *, char *, int (*)(void *, int, char **, char **), void *);

/* log.c */
void onxxdebug(char *str);
void onxxdebugm(char *str, ...);
void logstr(char *, int, char *, char *);

/* w32trial.c */
void check_db(void);

/* xml_out.c */
void write_xml(void);

/* libfunc.c */
#if defined(__WIN32__) || defined(NOSUPPORT_STRNDUP)
char *strndup(char *, int);
#endif
char *str_concat(char *, char *, char *, char *, char *);

/* charstack.c */
int charstack_check_for(charstack_t *, char *);
int charstack_push_on(charstack_t *, char *);
void charstack_free(charstack_t *);


