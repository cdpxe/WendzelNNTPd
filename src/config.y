%{

/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2010 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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
 
#include "main.h"

extern char *yytext;
extern FILE *yyin;
extern int size_sockinfo_t; /* is set to 0 on startup in main.c */

short parser_mode = PARSER_MODE_SERVER;

sockinfo_t *sockinfo = 0;
int peak = 0;
int port = DEFAULTPORT;
int max_post_size = MAX_POSTSIZE_DEFAULT;
fd_set fds;

unsigned short use_auth = 0;	/* global var i check in server.c while socket-loop */
unsigned short use_acl = 0;	/* do we use access control lists? */
unsigned short be_verbose = 0;	/* for debugging reasons */
unsigned short anonym_message_ids = 0; /* if eq 1: do not set IP or hostname within message IDs */
unsigned short dbase = DBASE_NONE;/* specifies the database engine to use */

char *db_server = NULL;
char *db_user = NULL;
char *db_pass = NULL;
unsigned short db_port = 0;
char *hash_salt = "default-hash-salt-0_----3331";

#define LF_ANY_IP	0x01
#define	LF_SPEC_IP	0x02
int listenflag = 0;

int is_tls_port = 0;
unsigned short use_tls = 0;	/* do we use TLS at all? */
unsigned short tls_mutual_auth = 0; /* check client cert */
char *tls_ca_file = NULL;
char *tls_cert_file = NULL;
char *tls_key_file = NULL;
char *tls_crl_file = NULL;
char *tls_cipher_prio = NULL;

unsigned short tls_is_mandatory = 0; /* force TLS on commands */

void
yyerror(const char *str)
{
   fprintf(stderr, "error: %s\n", str);
}

int
yywrap()
{
   return 1;
}

/* Wendzelnntpadm: set the global dbase value */
void
basic_setup_admtool()
{
	extern short parser_mode;
	FILE *fp;
	
	/* define the special parser mode */
	parser_mode = PARSER_MODE_ADMTOOL;
	
	close(0);
	if ((fp = fopen(CONFIGFILE, "r")) == NULL) {
		perror("Unable to open " CONFIGFILE);
		exit(ERR_EXIT);
	}
	if (chk_file_sec(CONFIGFILE) == INSECURE_RETURN) {
		fprintf(stderr, "aborting startup because of insecure "
				"file permissions for " CONFIGFILE "!\n");
		exit(ERR_EXIT);
	}
	
	/* find 'database-engine' string */
	yyparse();
	fclose(fp);
}

/* wendzelnntpd: read whole config file */
void
basic_setup_server(void)
{
#ifdef __WIN32__
	WSADATA wsa_dat;
#endif

	close(0);
	if((yyin = fopen(CONFIGFILE, "r")) == NULL) {
		perror("Unable to open " CONFIGFILE);
		exit(ERR_EXIT);
	}
	if (chk_file_sec(CONFIGFILE) == INSECURE_RETURN) {
		fprintf(stderr, "aborting startup because of insecure "
				"file permissions for " CONFIGFILE "!\n");
		exit(ERR_EXIT);
	}
	
	FD_ZERO(&fds);
	
#ifdef __WIN32__
	/* init damn winsock */
	if (WSAStartup(0x101, &wsa_dat) != 0) {
		fprintf(stderr,
			"Unable to initialize Windows Sockets (WSAStartUp(0x101,...))\n");
		exit(1);
	}
#endif
	
	yyparse();
	
	/* check if all needed values are included in the config struct */
	 
	if(!sockinfo) {
		fprintf(stderr, "There are no sockets to use!\n"
				"Use the 'listen' command in the config file to fix this.\n");
		exit(ERR_EXIT);
	}
#ifdef DEBUG
	printf(PR_STRING "peak: %i, size_sockinfo: %i\n", peak, size_sockinfo_t);
#endif

	/* Check database information */
	/* MySQL needs user+pass+server */
	if (dbase == DBASE_MYSQL) {
		if (!db_server || !db_user | !db_pass) {
			DO_SYSL("You need to specify a database server, username and password. Exiting.")
			err(1, "Need username, password and server for accessing database");
		}
	}
	
	/* If no port was set: use the default port */
	if (db_port == 0) {
		switch (dbase) {
		case DBASE_SQLITE3:
			/* Need no port */
			break;
		case DBASE_MYSQL:
			db_port = 3306;
			break;
		default:
			err(1, "Please specify a 'database-port' in your config file.\n");
		}
	}

  /* we need at minimum these 3 files if we want to use TLS */
  if (use_tls) {
    if (!tls_ca_file || !tls_cert_file || !tls_key_file) {
			DO_SYSL("You need to specify CA file, server cert and server key files to use TLS. Exiting.");
			fprintf(stderr, "You need to specify CA file, server cert and server key files to use TLS. Exiting.\n");
		  exit(ERR_EXIT);
    }
  }
}

%}

%token TOK_VERBOSE_MODE
%token TOK_USE_AUTH
%token TOK_USE_ACL
%token TOK_LISTEN_ON
%token TOK_PORT
%token TOK_MAX_POST_SIZE;
%token TOK_DYNIPDEV
%token TOK_CLIENT_LIMIT
%token TOK_CHANGEUID
%token TOK_CHANGEGID
%token TOK_ANONYM_MIDS
%token TOK_NAME
%token TOK_DB_ENGINE
%token TOK_DB_SERVER
%token TOK_DB_USER
%token TOK_DB_PASS
%token TOK_DB_PORT
%token TOK_HASHSALT
%token TOK_USE_TLS
%token TOK_TLS_MANDATORY
%token TOK_IS_TLS_PORT
%token TOK_TLS_CA_FILE
%token TOK_TLS_CERT_FILE
%token TOK_TLS_KEY_FILE
%token TOK_TLS_CRL_FILE
%token TOK_TLS_CIPHER_PRIO
%token TOK_TLS_MUTUAL_AUTH
%token TOK_EOF

%%

commands: /**/ | commands command;

command:  beVerbose | anonMessageIDs | useAuth | useACL | usePort | maxPostSize | listenonSpec | dbEngine | dbServer | dbUser | dbPass | dbPort | hashSalt | useTLS | tlsMandatory | tlsPort | tlsCAFile | tlsCertFile | tlsKeyFile | tlsCrlFile | tlsCipherPrio | tlsMutualAuth | eof;

beVerbose:
	TOK_VERBOSE_MODE
	{
		be_verbose=1;
	}

anonMessageIDs:
	TOK_ANONYM_MIDS
	{
		if (parser_mode == PARSER_MODE_SERVER)
			anonym_message_ids=1;
	}

useAuth:
	TOK_USE_AUTH
	{
		if (parser_mode == PARSER_MODE_SERVER)
			use_auth=1;
	};

useACL:
	TOK_USE_ACL
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (!use_auth) {
				fprintf(stderr,
					"You need to enable authentication before enabling ACL!\n"
					"Shutting down.\n");
				DO_SYSL("ACL enabling failed since authentication is disabled!\n"
					"Shutting down.\n");
				exit(1);
			}
			use_acl=1;
		}
	}

usePort:
	TOK_PORT TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			port = atoi(yytext);
			if (!port) {
				fprintf(stderr, "Port '%s' is not valid.\n", yytext);
				exit(1);
			}
      is_tls_port = 0;
		}
	};

maxPostSize:
	TOK_MAX_POST_SIZE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			max_post_size = atoi(yytext);
			if (!max_post_size) {
				fprintf(stderr, "Max. posting size value '%s' is not valid.\n", yytext);
				exit(1);
			} /*else {
				fprintf(stderr, "Max. posting size set to %s bytes\n", yytext);
			}*/
		}
	};


listenonSpec:  /* done */
	TOK_LISTEN_ON TOK_NAME
	{
		int size=0;
		int salen, sa6len;
		int yup=1;
		struct sockaddr_in sa;
		struct sockaddr_in6 sa6;
		char *yytext_ = NULL;
		
		if (parser_mode == PARSER_MODE_SERVER) {

			CALLOC(yytext_, (char *), strlen(yytext) + 1, sizeof(char))
			strncpy(yytext_, yytext, strlen(yytext));
		
			if (listenflag == LF_ANY_IP) {
				fprintf(stderr,
					"error: you have to choose between ANY IP address or some specific\n"
					"IP address but you cannot use both features at the same time.\n");
				exit(0);
			}
			listenflag = LF_SPEC_IP;
		
			if (!sockinfo) {
				CALLOC(sockinfo, (sockinfo_t *), 1, sizeof(sockinfo_t))
			} else {
				size = size_sockinfo_t;
				if ((sockinfo=realloc(sockinfo, (size + 1) * sizeof(sockinfo_t))) == NULL) {
					fprintf(stderr, "cannot allocate memory.\n");
					exit(ERR_EXIT);
				}
			}
		
			bzero(&sa, sizeof(sa));
			bzero(&sa6, sizeof(sa6));
#ifdef __WIN32__ /* lol */
			sa.sin_addr.s_addr = inet_addr(yytext);
			/* Warning: This 32bit only! */
			if (sa.sin_addr.s_addr != 0xffffffff)
#else
			if (inet_pton(AF_INET, yytext_, &sa.sin_addr))
#endif
			{
				sa.sin_port = htons(port);
				sa.sin_family = AF_INET;
				salen = sizeof(struct sockaddr_in);
			
				if (((sockinfo+size)->sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					fprintf(stderr, "cannot do socket() on %s\n", yytext_);
					exit(ERR_EXIT);
				}
			
				setsockopt((sockinfo + size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));
			
				if (bind((sockinfo + size)->sockfd, (struct sockaddr *)&sa, salen) < 0) {
					perror("bind");
					fprintf(stderr, "bind() for %s failed.\n", yytext_);
					exit(ERR_EXIT);
				}
			
				if (listen((sockinfo + size)->sockfd, 5) < 0) {
					fprintf(stderr, "listen() for %s failed.\n", yytext_);
					exit(ERR_EXIT);
				}
        if (is_tls_port) {
          (sockinfo+size)->is_tls = 1;
        }
				peak = max((sockinfo + size)->sockfd, peak);
				(sockinfo + size)->family=AF_INET;
#ifndef __WIN32__ /* IPv6-ready systems */
			} else if (inet_pton(AF_INET6, yytext_, &sa6.sin6_addr)) {
				sa6.sin6_port = htons(port);
				sa6.sin6_family = AF_INET6;
				sa6len = sizeof(struct sockaddr_in6);
			
				if (((sockinfo + size)->sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
					fprintf(stderr, "cannot do socket() on %s\n", yytext_);
					exit(ERR_EXIT);
				}
			
				setsockopt((sockinfo + size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));
			
				if (bind((sockinfo + size)->sockfd, (struct sockaddr *)&sa6, sa6len) < 0) {
					fprintf(stderr, "bind() for %s failed.\n", yytext_);
					exit(ERR_EXIT);
				}
			
				if (listen((sockinfo + size)->sockfd, 5) < 0) {
					fprintf(stderr, "listen() for %s failed.\n", yytext_);
					exit(ERR_EXIT);
				}
        if (is_tls_port) {
          (sockinfo+size)->is_tls = 1;
        }
				peak = max((sockinfo+size)->sockfd, peak);
				(sockinfo + size)->family = AF_INET6;
#endif
			} else {
				fprintf(stderr, "Invalid address: %s\n", yytext_);
				exit(ERR_EXIT);
			}
			free(yytext_);
			size_sockinfo_t++;
		}
	};

dbEngine:
	TOK_DB_ENGINE TOK_NAME
	{
		/* check the database engine string */
		if (strncmp("sqlite3", yytext, strlen(yytext)) == 0) {
			dbase = DBASE_SQLITE3;
		} else if (strncmp("mysql", yytext, strlen(yytext)) == 0) {
			dbase = DBASE_MYSQL;
		} else {
			fprintf(stderr, "Database engine %s not supported.\n", yytext);
			DO_SYSL("Unknown database engine specified in config file")
			exit(1);
		}
	}

dbServer:
	TOK_DB_SERVER TOK_NAME
	{
		if (!(db_server = strdup(yytext))) {
			DO_SYSL("strdup() error (database-server)")
			err(1, "strdup() error (database-server)");
		}
	}

dbUser:
	TOK_DB_USER TOK_NAME
	{
		if (!(db_user = strdup(yytext))) {
			DO_SYSL("strdup() error (database-username)")
			err(1, "strdup() error (database-username)");
		}
	}

dbPass:
	TOK_DB_PASS TOK_NAME
	{
		if (!(db_pass = strdup(yytext))) {
			DO_SYSL("strdup() error (database-password)")
			err(1, "strdup() error (database-password)");
		}
	}

dbPort:
	TOK_DB_PORT TOK_NAME
	{
		if (!(db_port = atoi(yytext))) {
			DO_SYSL("atoi() error (database-port)")
			err(1, "atoi() error (database-port)");
		}
	}

hashSalt:
	TOK_HASHSALT TOK_NAME
	{
		if (!(hash_salt = strdup(yytext))) {
			DO_SYSL("strdup() error (hash_salt)")
			err(1, "strdup() error (hash_salt)");
		}
	}

useTLS:
	TOK_USE_TLS
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			use_tls=1;
    }
	};

tlsMandatory:
	TOK_TLS_MANDATORY
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      tls_is_mandatory = 1;
		}
	}

tlsPort:
	TOK_IS_TLS_PORT
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      is_tls_port = 1;
		}
	}

tlsCAFile:
	TOK_TLS_CA_FILE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      if (!(tls_ca_file = strdup(yytext))) {
        DO_SYSL("strdup() error (tls-ca-file)")
        err(1, "strdup() error (tls-ca-file)");
      }
		}
	}

tlsCertFile:
	TOK_TLS_CERT_FILE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      if (!(tls_cert_file = strdup(yytext))) {
        DO_SYSL("strdup() error (tls-cert-file)")
        err(1, "strdup() error (tls-cert-file)");
      }
		}
	}

tlsKeyFile:
	TOK_TLS_KEY_FILE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      if (!(tls_key_file = strdup(yytext))) {
        DO_SYSL("strdup() error (tls-key-file)")
        err(1, "strdup() error (tls-key-file)");
      }
		}
	}

tlsCrlFile:
	TOK_TLS_CRL_FILE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      if (!(tls_crl_file = strdup(yytext))) {
        DO_SYSL("strdup() error (tls-crl-file)")
        err(1, "strdup() error (tls-crl-file)");
      }
		}
	}

tlsCipherPrio:
	TOK_TLS_CIPHER_PRIO TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      if (!(tls_cipher_prio = strdup(yytext))) {
        DO_SYSL("strdup() error (tls-cipher-prio)")
        err(1, "strdup() error (tls-cipher-prio)");
      }
		}
	}

tlsMutualAuth:
	TOK_TLS_MUTUAL_AUTH
	{
		if (parser_mode == PARSER_MODE_SERVER) {
      tls_mutual_auth = 1;
		}
	}

eof:
	TOK_EOF
	{
	};

%%

