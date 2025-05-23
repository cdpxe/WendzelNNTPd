%{

/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2010 Steffen Wendzel <steffen (at) wendzel (dot) de>
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

/* NetBSD compatibility; taken from patch by Michael Baeuerle
 * see: https://github.com/jgoerzen/libnbcompat */
#if defined(HAVE_NBCOMPAT_H)
	#include <nbcompat/config.h>    /* Needed for the other headers */
	#include <nbcompat/cdefs.h>     /* Needed for the other headers */
	#include <nbcompat/err.h>
#else
	#include <err.h>
#endif

#include "main.h"

/* Lexer generated by flex */
extern int yylex(void);
/* Parser generated by bison */
int yyparse(void);

extern char *yytext;
extern FILE *yyin;
extern int size_sockinfo_t; /* is set to 0 on startup in main.c */

short parser_mode = PARSER_MODE_SERVER;

sockinfo_t *sockinfo = 0;
connectorinfo_t *connectorinfo = NULL; /* used for multiple connectors */
int peak = 0;
int max_post_size = MAX_POSTSIZE_DEFAULT;
fd_set fds;

unsigned short use_auth = 0;	/* global var i check in server.c while socket-loop */
unsigned short use_acl = 0;	/* do we use access control lists? */
unsigned short be_verbose = 0;	/* for debugging reasons */
unsigned short anonym_message_ids = 0; /* if eq 1: do not set IP or hostname within message IDs */
unsigned short dbase = DBASE_NONE;/* specifies the database engine to use */
unsigned short message_body_in_db = 0; /* load/store the body from/into DB */
unsigned short message_count_in_db = 0; /* load/store message counter from/into DB */

char *db_server = NULL;
char *db_user = NULL;
char *db_pass = NULL;
unsigned short db_port = 0;
char *hash_salt = "default-hash-salt-0_----3331";
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
		exit(ERR_EXIT);
	}
#endif

	if (yyparse() != 0){		//Error parsing config file
		DO_SYSL("Error parsing config file!");
	   	fprintf(stderr,"Error parsing config file!\n");
		exit(ERR_EXIT);
	}

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
	/* MySQL/Postgres needs user+pass+server */
	if (dbase == DBASE_MYSQL || dbase == DBASE_POSTGRES) {
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
		case DBASE_POSTGRES:
			db_port = 5432;
			break;
		default:
			err(1, "Please specify a 'database-port' in your config file.\n");
		}
	}
}

static void
start_listeners(void) {

	int size = 0;
	int salen, sa6len;
	int yup = 1;
	struct sockaddr_in sa;
	struct sockaddr_in6 sa6;

	if (!sockinfo) {
		// allocate memory for socket
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

#ifdef USE_TLS
	// set tls_active default for socket
	(sockinfo + size)->tls_active=FALSE;			
#endif
	(sockinfo + size)->connectorinfo=connectorinfo;					//Link sockinfo to connectorinfo

	if (inet_pton(AF_INET, connectorinfo->listen, &sa.sin_addr)) {					//IPv4 Listener
		sa.sin_port = htons(connectorinfo->port);
		sa.sin_family = AF_INET;
		salen = sizeof(struct sockaddr_in);

		if (((sockinfo+size)->sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {							//Socket create
			fprintf(stderr, "cannot do socket() on %s\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}	

		setsockopt((sockinfo + size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));		//Socket Options

		if (bind((sockinfo + size)->sockfd, (struct sockaddr *)&sa, salen) < 0) {					//Socket bind
			perror("bind");
			fprintf(stderr, "bind() for %s failed.\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}

		if (listen((sockinfo + size)->sockfd, 5) < 0) {														//Socket listen
			fprintf(stderr, "listen() for %s failed.\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}

		peak = max((sockinfo + size)->sockfd, peak);
		(sockinfo + size)->family=AF_INET;
		fprintf(stdout, "Created IPv4 listener on %s:%d\n",connectorinfo->listen,connectorinfo->port);
	} else if (inet_pton(AF_INET6, connectorinfo->listen, &sa6.sin6_addr)) {	//IPv6 Listener
		sa6.sin6_port = htons(connectorinfo->port);
		sa6.sin6_family = AF_INET6;
		sa6len = sizeof(struct sockaddr_in6);

		if (((sockinfo + size)->sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {					//Socket create
			fprintf(stderr, "cannot do socket() on %s\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}

		setsockopt((sockinfo + size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));
		setsockopt((sockinfo + size)->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &yup, sizeof(yup));	//Do not bind IPv4 port with IPv6

		if (bind((sockinfo + size)->sockfd, (struct sockaddr *)&sa6, sa6len) < 0) {				//Socket bind
			perror("bind");
			fprintf(stderr, "bind() for %s failed.\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}

		if (listen((sockinfo + size)->sockfd, 5) < 0) {														//Socket listen
			fprintf(stderr, "listen() for %s failed.\n", connectorinfo->listen);
			exit(ERR_EXIT);
		}
	
		peak = max((sockinfo+size)->sockfd, peak);
		(sockinfo + size)->family = AF_INET6;	
		fprintf(stdout, "Created IPv6 listener on %s:%d\n",connectorinfo->listen,connectorinfo->port);
	} else {																							//Error Listener not IPv4 or IPv6
		fprintf(stderr, "Invalid address: %s\n", connectorinfo->listen);
		exit(ERR_EXIT);
	}
	size_sockinfo_t++;

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
%token TOK_MESSAGE_COUNT_IN_DB
%token TOK_MESSAGE_BODY_IN_DB
%token TOK_HASHSALT
%token TOK_TLS_MANDATORY
%token TOK_CONN_BEGIN
%token TOK_TLS
%token TOK_TLS_CIPHERS
%token TOK_TLS_CIPHER_SUITES;
%token TOK_TLS_CERT
%token TOK_TLS_KEY
%token TOK_STARTTLS
%token TOK_TLS_CA_CERT
%token TOK_TLS_VERSION
%token TOK_TLS_VERSION_STRING
%token TOK_TLS_VERIFY_CLIENT
%token TOK_TLS_VERIFY_CLIENT_DEPTH
%token TOK_TLS_CRL
%token TOK_TLS_CRL_FILE
%token TOK_CONN_END
%token TOK_EOF

%%

commands: /**/ | commands command | commands connector;

command:  beVerbose | anonMessageIDs | useAuth | useACL | maxPostSize | dbEngine | dbServer | dbUser | dbPass | dbPort | hashSalt | messageBodyInDb | messageCountInDb | TLSMandatory | eof;

connector: connectorBegin connectorCmds connectorEnd;

connectorCmds: /**/ | connectorCmds connectorCmd;

connectorCmd:  usePort | listenonSpec | enableTLS | enableSTARTTLS | TLSCiphers | TLSCipherSuites | TLSCert | TLSKey | TLSCACert | TLSVersion | TLSVerifyClient | TLSVerifyClientDepth | TLSCrl | TLSCrlFile;

connectorBegin:
	TOK_CONN_BEGIN
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			CALLOC(connectorinfo, (connectorinfo_t *), 1, sizeof(connectorinfo_t));
			connectorinfo->port=0;
			connectorinfo->listen=NULL;
			connectorinfo->enable_tls=FALSE;
			connectorinfo->enable_starttls=FALSE;
			connectorinfo->ciphers=NULL;	// Cipher TLS1.0 - 1.2
			connectorinfo->cipher_suites=NULL; // Cipher Suites TLS1.3

			connectorinfo->server_cert_file=NULL;	// Server Cert
			connectorinfo->server_key_file=NULL; // Server Key
			connectorinfo->ca_cert_file=NULL; // CA File
			connectorinfo->tls_verify_client = VERIFY_UNDEV; // mTLS: VERIFY_NONE, VERIFY_OPTIONAL, VERIFY_REQUIRE
			connectorinfo->tls_verify_client_depth = 10;
			connectorinfo->tls_crl = CRL_UNDEV; // Certificate Revocation List (CRL)
			connectorinfo->tls_crl_file = NULL;

#ifdef USE_TLS
			connectorinfo->tls_minimum_version = TLS1_2_VERSION;
			connectorinfo->tls_maximum_version = TLS1_3_VERSION;
#else
			connectorinfo->tls_minimum_version = 0;
			connectorinfo->tls_maximum_version = 0;
#endif
		}
	};

connectorEnd:
	TOK_CONN_END
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->port == 0) {					//Port was not set in config-File -> set default port
				initialize_connector_ports(connectorinfo);
			}

			// early return if no listener was defined for connector
			if (connectorinfo->listen == NULL) {
				fprintf(stderr,"Listen was not defined in connector!\n");
#ifdef USE_TLS
				ERR_print_errors_fp(stderr);
#endif
				exit(ERR_EXIT);
			}

#ifdef USE_TLS
			// enable TLS if enable-tls or enable-starttls isset in configuration for connector
			if (connectorinfo->enable_tls || connectorinfo->enable_starttls) {
				// check needed prerequisites -> if successful, then init TLS implementation
				if (check_tls_prerequisites(connectorinfo)) {
					tls_global_init(connectorinfo);
				}
			}
#endif
			start_listeners();
		}
	};

enableTLS:
	TOK_TLS
	{
		if (parser_mode == PARSER_MODE_SERVER) {
#ifdef USE_TLS
			connectorinfo->enable_tls=TRUE;
#else
			connectorinfo->enable_tls=FALSE;			//enabled on Connector but no TLS support
#endif
		}
	};

enableSTARTTLS:
	TOK_STARTTLS
	{
		if (parser_mode == PARSER_MODE_SERVER) {
#ifdef USE_TLS
			connectorinfo->enable_starttls=TRUE;
#else
			connectorinfo->enable_starttls=FALSE;		//STARTTLS enabled on Connector but no TLS support
#endif
		}
	};

TLSCiphers:
	TOK_TLS_CIPHERS TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->ciphers == NULL) { //Ciphers have not been defined in connector
				CALLOC(connectorinfo->ciphers, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->ciphers, yytext, strlen(yytext));
			} else {													//Cipher already defined in connector
				DO_SYSL("Ciphers already defined in Connector - ignoring");
				fprintf(stderr,"Ciphers already defined in Connector - ignoring: %s \n",yytext);
			}
		}
	};

TLSCipherSuites:
	TOK_TLS_CIPHER_SUITES TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->cipher_suites == NULL) {			//Cipher suites have not been defined in connector
				CALLOC(connectorinfo->cipher_suites, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->cipher_suites, yytext, strlen(yytext));
			} else {														//Cipher suites already defined in connector
				DO_SYSL("TLS1.3 Cipher suites already defined in Connector - ignoring");
				fprintf(stderr,"TLS1.3 Cipher suites already defined in Connector - ignoring: %s \n",yytext);
			}
		}
	};

TLSMandatory:
	TOK_TLS_MANDATORY
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			// set TLS mandatory
			tls_is_mandatory = 1;
		}
	};

TLSVersion:
	TOK_TLS_VERSION TOK_TLS_VERSION_STRING
	{
		if (parser_mode == PARSER_MODE_SERVER) {
#ifdef USE_TLS
			int maximum = 3;
			int minimum = 0;

			sscanf(yytext, "1.%d-1.%d", &minimum, &maximum);

			// change maximum and minimum, if minimum is greater than maximum
			if (maximum < minimum) {
				int tmp = maximum;
				maximum = minimum;
				minimum = tmp;
			}

			switch (minimum) {
				case 0: 	connectorinfo->tls_minimum_version = TLS1_VERSION;
							break;
				case 1: 	connectorinfo->tls_minimum_version = TLS1_1_VERSION;
							break;
				case 2: 	connectorinfo->tls_minimum_version = TLS1_2_VERSION;
							break;
				case 3: 	connectorinfo->tls_minimum_version = TLS1_3_VERSION;
							break;
				default: 	connectorinfo->tls_minimum_version = TLS1_2_VERSION;
							fprintf(stderr,"Error TLS Version String \"%s\"! Minimum could not be recognized - setting default TLS1.2!\n",yytext);
							break;
			}

			switch (maximum) {
				case 0: 	connectorinfo->tls_maximum_version = TLS1_VERSION;
							break;
				case 1: 	connectorinfo->tls_maximum_version = TLS1_1_VERSION;
							break;
				case 2: 	connectorinfo->tls_maximum_version = TLS1_2_VERSION;
							break;
				case 3: 	connectorinfo->tls_maximum_version = TLS1_3_VERSION;
							break;
				default: 	connectorinfo->tls_maximum_version = TLS1_3_VERSION;
							fprintf(stderr,"Error TLS Version String \"%s\"! Maximum could not be recognized - setting default TLS1.3!\n",yytext);
							break;
			}
#endif
		}
	};

TLSCert:
	TOK_TLS_CERT TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->server_cert_file == NULL)	{
				CALLOC(connectorinfo->server_cert_file, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->server_cert_file, yytext, strlen(yytext));
			} else {
				DO_SYSL("Config-File: More than one tls-server-certificate statement in connector");
			}
		}
	};

TLSKey:
	TOK_TLS_KEY TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->server_key_file == NULL)	{
				CALLOC(connectorinfo->server_key_file, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->server_key_file, yytext, strlen(yytext));
			} else {
				DO_SYSL("Config-File: More than one tls-server-key statement in connector");
			}
		}
	};

TLSCACert:
   TOK_TLS_CA_CERT TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->ca_cert_file == NULL) {
         	CALLOC(connectorinfo->ca_cert_file, (char *), strlen(yytext) + 1, sizeof(char));
         	strncpy(connectorinfo->ca_cert_file, yytext, strlen(yytext));
			} else {
				DO_SYSL("Config-File: More than one tls-ca-certificate statement in connector. Ignoring");
			}
      }
   };

TLSVerifyClient:
	TOK_TLS_VERIFY_CLIENT TOK_NAME
	{
		// set mTLS mode
		if (parser_mode == PARSER_MODE_SERVER) {
			if (strncmp(yytext, "none", 4) == 0) {
				connectorinfo->tls_verify_client = VERIFY_NONE;
			} else if (strncmp(yytext, "optional", 8) == 0) {
				connectorinfo->tls_verify_client = VERIFY_OPTIONAL;
			} else if (strncmp(yytext, "require", 7) == 0) {
				connectorinfo->tls_verify_client = VERIFY_REQUIRE;
			} else {
				DO_SYSL("Config-File: tls-verify-client must be [none | optional | require]. Ignoring");
			}
		}
	};

TLSVerifyClientDepth:
	TOK_TLS_VERIFY_CLIENT_DEPTH TOK_NAME {
		if (parser_mode == PARSER_MODE_SERVER) {
			connectorinfo->tls_verify_client_depth = atoi(yytext);
			if (atoi(yytext) > 128) {
				DO_SYSL("Config-File: tls-verify-client-depth too large [0-128]. Ignoring - using Default (10)");
				connectorinfo->tls_verify_client_depth=10;
			}
		}
	};

TLSCrl:
	TOK_TLS_CRL TOK_NAME {
		if (parser_mode == PARSER_MODE_SERVER) {
			if (strncmp(yytext, "none", 4) == 0) {
				connectorinfo->tls_crl = CRL_NONE;
			} else if (strncmp(yytext, "leaf", 4) == 0) {
				connectorinfo->tls_crl = CRL_LEAF;
			} else if (strncmp(yytext, "chain", 5) == 0) {
				connectorinfo->tls_crl = CRL_CHAIN;
			} else {
				 DO_SYSL("Config-File: tls-crl must be [none | leaf | chain]. Ignoring");
			}
		}
	};

TLSCrlFile:
	TOK_TLS_CRL_FILE TOK_NAME {
		if (parser_mode == PARSER_MODE_SERVER) {
			CALLOC(connectorinfo->tls_crl_file, (char *), strlen(yytext) + 1, sizeof(char));
         	strncpy(connectorinfo->tls_crl_file, yytext, strlen(yytext));
		}
	};

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
				exit(ERR_EXIT);
			}
			use_acl=1;
		}
	}

messageBodyInDb:
	TOK_MESSAGE_BODY_IN_DB
	{
		if (parser_mode == PARSER_MODE_SERVER) {
		    if (dbase != DBASE_POSTGRES) {
			fprintf(stderr, "Storing message body within the"
				" database is only supported for Postgres.\n");
			exit(ERR_EXIT);
		    }
		    message_body_in_db = 1;
		}
	}

messageCountInDb:
	TOK_MESSAGE_COUNT_IN_DB
	{
		if (parser_mode == PARSER_MODE_SERVER) {
		    if (dbase != DBASE_POSTGRES) {
			fprintf(stderr, "Storing next-message ID within the"
				" database is only supported for Postgres.\n");
			exit(ERR_EXIT);
		    }
		    message_count_in_db = 1;
		}
	}

usePort:
	TOK_PORT TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->port == 0) {
				connectorinfo->port = atoi(yytext);
				// check if port isset and is an integer
				if (!connectorinfo->port) {
					fprintf(stderr, "Port '%s' is not a valid port.\n", yytext);
					exit(ERR_EXIT);
				}
			} else {																		//more than one port definition in connector
				fprintf(stderr, "More than one port statement in connector\n" );
				exit(ERR_EXIT);
			}
		}
	};

maxPostSize:
	TOK_MAX_POST_SIZE TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			max_post_size = atoi(yytext);
			if (!max_post_size) {
				fprintf(stderr, "Max. posting size value '%s' is not valid.\n", yytext);
				exit(ERR_EXIT);
			} /*else {
				fprintf(stderr, "Max. posting size set to %s bytes\n", yytext);
			}*/
		}
	};


listenonSpec:
	TOK_LISTEN_ON TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->listen == NULL) {
				CALLOC(connectorinfo->listen, (char *), strlen(yytext) + 1, sizeof(char))
				strncpy(connectorinfo->listen, yytext, strlen(yytext));
			} else {
				fprintf(stderr, "More than one listen statement in connector\n" );
				exit(ERR_EXIT);
			}
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
		} else if (strncmp("postgres", yytext, strlen(yytext)) == 0) {
			dbase = DBASE_POSTGRES;
		} else {
			fprintf(stderr, "Database engine %s not supported.\n", yytext);
			DO_SYSL("Unknown database engine specified in config file")
			exit(ERR_EXIT);
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

eof:
	TOK_EOF
	{
	};

%%
