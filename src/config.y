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
connectorinfo_t *connectorinfo = NULL;
int peak = 0;
//int port = DEFAULTPORT;
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

//#define LF_ANY_IP	0x01
//#define	LF_SPEC_IP	0x02
//int listenflag = 0;

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
}

/* start listener for each connector */
static void
start_listener(void) {

int size=0;
int salen, sa6len;
int yup=1;
struct sockaddr_in sa;
struct sockaddr_in6 sa6;

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

	(sockinfo + size)->ssl_active=FALSE;			
	(sockinfo + size)->connectorinfo=connectorinfo;					//Link sockinfo to connectorinfo

	if (inet_pton(AF_INET, connectorinfo->listen, &sa.sin_addr)) {					//IPv4 Listenet
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
		fprintf(stderr, "Created IPv4 listener on %s:%d\n",connectorinfo->listen,connectorinfo->port);
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
		fprintf(stderr, "Created IPv6 listener on %s:%d\n",connectorinfo->listen,connectorinfo->port);
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
%token TOK_HASHSALT
%token TOK_CONN_BEGIN
%token TOK_SSL
%token TOK_STARTTLS
%token TOK_SSL_CIPHERS
%token TOK_SSL_CIPHER_SUITES;
%token TOK_SSL_VERSION
%token TOK_SSL_CERT
%token TOK_SSL_KEY
%token TOK_SSL_CIPHERS_PREF
%token TOK_SSL_VERIFY
%token TOK_SSL_VERIFY_DEPTH
%token TOK_SSL_CN_AUTH
%token TOK_SSL_CRL_PATH
%token TOK_SSL_CRL_FILE
%token TOK_SSL_CRL_CHECK
%token TOK_SSL_CA_PATH
%token TOK_SSL_CA_FILE
%token TOK_CONN_END
%token TOK_EOF

%%

commands: /**/ | commands command | commands connector;

command:  beVerbose | anonMessageIDs | useAuth | useACL |  maxPostSize | dbEngine | dbServer | dbUser | dbPass | dbPort | hashSalt | eof ;

connector: connectorBegin connectorCmds connectorEnd;

connectorCmds: /**/ | connectorCmds connectorCmd;

connectorCmd:  usePort | listenonSpec | enableSSL | enableSTARTTLS | SSLCiphers | SSLCipherSuites | SSLVersion | SSLCert | SSLKey | SSLCipherPref | SSLVerify | SSLVerifyDepth | SSLCNAuth | SSLCRLPath | SSLCRLFile | SSLCRLCheck | SSLCAPath | SSLCAFile ;

connectorBegin:
	TOK_CONN_BEGIN
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			CALLOC(connectorinfo, (connectorinfo_t *), 1, sizeof(connectorinfo_t));
			connectorinfo->port=0;
			connectorinfo->listen=NULL;
			connectorinfo->enable_ssl=FALSE;							// TRUE, FALSE
			connectorinfo->enable_starttls=FALSE;					// TRUE, FALSE
			connectorinfo->ciphers=NULL;								// Cipher TLS1.0 - 1.2
			connectorinfo->cipher_suites=NULL;						// Cipher Suites TLS1.3
#ifdef USE_SSL
			connectorinfo->tlsversion_min=TLS1_VERSION;			// SSL3_VERSION, TLS1_VERSION, TLS1_1_VERSION, TLS1_2_VERSION, TLS1_3_VERSION
			connectorinfo->tlsversion_max=TLS1_3_VERSION;		// SSL3_VERSION, TLS1_VERSION, TLS1_1_VERSION, TLS1_2_VERSION, TLS1_3_VERSION
#else
			connectorinfo->tlsversion_min=0;
			connectorinfo->tlsversion_max=0;
#endif
			connectorinfo->server_cert_file=NULL;
			connectorinfo->server_key_file=NULL;
			connectorinfo->server_cipher_preference=FALSE;		// TRUE, FALSE
			connectorinfo->verify_client=VERIFY_UNDEV;				// VERIFY_NONE, VERIFY_OPTIONAL, VERIFY_REQUIRE
			connectorinfo->verify_depth=10;
			connectorinfo->CN_authentication=FALSE;				// TRUE, FALSE
			connectorinfo->CRL_path=NULL;
			connectorinfo->CRL_file=NULL;
			connectorinfo->CRL_check=CRL_UNDEV;						// CRL_NONE, CRL_LEAF, CRL_CHAIN
			connectorinfo->CA_path=NULL;
			connectorinfo->CA_file=NULL;
//      	fprintf(stderr,"Connector Begin\n");
		}
	};

connectorEnd:
	TOK_CONN_END
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->port == 0) {					//Port was not set in config-File
				if (connectorinfo->enable_ssl)				//SSL enabled
					connectorinfo->port=DEFAULTSSLPORT;
				else
					connectorinfo->port=DEFAULTPORT;
			}
			if (connectorinfo->listen != NULL) {																													//Listener has been set in connector
				if (connectorinfo->enable_ssl || connectorinfo->enable_starttls) {																		//Check SSL parameter in connector
					if ((connectorinfo->server_cert_file != NULL) && (connectorinfo->server_key_file != NULL)) {									//Cert and Key has been defined
						if ((access(connectorinfo->server_cert_file,R_OK) == 0) && (access(connectorinfo->server_key_file,R_OK) == 0))	{	//Cert and Key files exist
#ifdef USE_SSL
							connectorinfo->ctx=SSL_CTX_new(TLS_server_method());
							if (!connectorinfo->ctx) {
        						fprintf(stderr,"Error creating SSL Context!\n");
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							if (!SSL_CTX_set_min_proto_version(connectorinfo->ctx,connectorinfo->tlsversion_min)) {
        						fprintf(stderr,"Error setting min TLS version in SSL Context!!\n");
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							if (!SSL_CTX_set_max_proto_version(connectorinfo->ctx,connectorinfo->tlsversion_max)) {
        						fprintf(stderr,"Error setting max TLS version in SSL Context!!\n");
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							if (!SSL_CTX_use_PrivateKey_file(connectorinfo->ctx,connectorinfo->server_key_file,SSL_FILETYPE_PEM)) {
        						fprintf(stderr,"Error loading private key file \"%s\" in SSL Context!!\n",connectorinfo->server_key_file);
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							if (!SSL_CTX_use_certificate_file(connectorinfo->ctx,connectorinfo->server_cert_file,SSL_FILETYPE_PEM)) {
        						fprintf(stderr,"Error loading certificate \"%s\" in SSL Context!!\n",connectorinfo->server_cert_file);
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							// verify private key with certificate
							if (!SSL_CTX_check_private_key(connectorinfo->ctx)) {
        						fprintf(stderr,"Private key in \"%s\" does not match Certificate in \"%s\" !\n",connectorinfo->server_key_file,connectorinfo->server_cert_file);
								ERR_print_errors_fp(stderr);
								exit(ERR_EXIT);
							}
							if (connectorinfo->ciphers != NULL)
								if (!SSL_CTX_set_cipher_list(connectorinfo->ctx,connectorinfo->ciphers)) {
        							fprintf(stderr,"Error setting ciphers \"%s\" in SSL Context!!\n",connectorinfo->ciphers);
									ERR_print_errors_fp(stderr);
									exit(ERR_EXIT);
								}
							if (connectorinfo->cipher_suites != NULL)
								if (!SSL_CTX_set_ciphersuites(connectorinfo->ctx,connectorinfo->cipher_suites)) {
        							fprintf(stderr,"Error setting TLS1.3 ciphers suites \"%s\" in SSL Context!!\n",connectorinfo->cipher_suites);
									ERR_print_errors_fp(stderr);
									exit(ERR_EXIT);
								}
							if (connectorinfo->server_cipher_preference == TRUE) 			// User server Cipher order
								if(!SSL_CTX_set_options(connectorinfo->ctx, SSL_OP_CIPHER_SERVER_PREFERENCE)){
        							fprintf(stderr,"Error setting option SSL_OP_CIPHER_SERVER_PREFERENCE in SSL Context!\n");
									ERR_print_errors_fp(stderr);
									exit(ERR_EXIT);
								}
							if (connectorinfo->verify_client != VERIFY_UNDEV) {			//Verify option
								switch (connectorinfo->verify_client) {
									case VERIFY_OPTIONAL : SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_PEER, NULL);
//																  fprintf(stderr,"VERIFY_OPTIONAL\n");
																  break;
									case VERIFY_REQUIRE  : SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, NULL);
//																  fprintf(stderr,"VERIFY_REQUIRE\n");
																  break;
									case VERIFY_NONE     : 
									default					: connectorinfo->verify_client=VERIFY_NONE;
																  SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_NONE, NULL);
//																  fprintf(stderr,"VERIFY_NONE\n");
																  break;
								}
								SSL_CTX_set_verify_depth(connectorinfo->ctx, connectorinfo->verify_depth);		//Verify depth
							}
							if ((connectorinfo->verify_client != VERIFY_UNDEV) && (connectorinfo->verify_client != VERIFY_NONE)) {		//Load CA Cert for verify
								if ((connectorinfo->CA_file != NULL) || (connectorinfo->CA_path != NULL)) {
									if (!SSL_CTX_load_verify_locations(connectorinfo->ctx, connectorinfo->CA_file, connectorinfo->CA_path)){
        								fprintf(stderr,"Error setting verify location in SSL Context!\n");
										ERR_print_errors_fp(stderr);
										exit(ERR_EXIT);
									} else {																	// Certificates in Verify location loaded OK
										STACK_OF(X509_NAME) *cert_names=SSL_CTX_get_client_CA_list(connectorinfo->ctx);		//Send List of CA-Names to Client
										if (cert_names != NULL) {
											if (connectorinfo->CA_path != NULL)
												SSL_add_dir_cert_subjects_to_stack(cert_names,connectorinfo->CA_path);
											if (connectorinfo->CA_file != NULL)
												SSL_add_file_cert_subjects_to_stack(cert_names,connectorinfo->CA_file);
										}
									}
								}
							}
							if ((connectorinfo->CRL_check == CRL_LEAF) || (connectorinfo->CRL_check == CRL_CHAIN)) {	//CRL check
								X509_VERIFY_PARAM *param=X509_VERIFY_PARAM_new();
								switch (connectorinfo->CRL_check) {
									case CRL_LEAF	: X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
														  break;
									case CRL_CHAIN	: X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL| X509_V_FLAG_TRUSTED_FIRST); 
														  break;
								}
								SSL_CTX_set1_param(connectorinfo->ctx,param);
								X509_VERIFY_PARAM_free(param);

								if ((connectorinfo->CRL_file != NULL) || (connectorinfo->CRL_path != NULL)) {	//Load CRL File or Path
									X509_STORE *cert_store=SSL_CTX_get_cert_store(connectorinfo->ctx);
									if (cert_store != NULL) {																	// Load Store from Context
										if (!X509_STORE_load_locations(cert_store,connectorinfo->CRL_file,connectorinfo->CRL_path)) {
        									fprintf(stderr,"Error setting CRL location in SSL Context!\n");
											ERR_print_errors_fp(stderr);
											exit(ERR_EXIT);
										}
									} else {
        								fprintf(stderr,"X509_STORE could not be loaded from SSL Context!\n");
									}
								}
							}
							start_listener();																																
#endif
						}
						else {
        					fprintf(stderr,"Cert or Key file could not be read!\n");
						}
					}
					else {
        				fprintf(stderr,"Cert or Key file has not been set in connector!\n");
					}
				} else {																																						//SSL note enabled on connector
					start_listener();
				}
			}
			else {
        		fprintf(stderr,"Listen was not defined in connector!\n");
			}
//        	fprintf(stderr,"Connector End\n");
		}
	};

enableSSL:
	TOK_SSL
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			connectorinfo->enable_ssl=TRUE;
//        	fprintf(stderr,"Enable SSL\n");
		}
	};

enableSTARTTLS:
	TOK_STARTTLS
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			connectorinfo->enable_starttls=TRUE;
//        	fprintf(stderr,"Enable STARTTLS\n");
		}
	};

SSLCiphers:
	TOK_SSL_CIPHERS TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->ciphers == NULL) {			//Ciphers have not been defined in connector
				CALLOC(connectorinfo->ciphers, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->ciphers, yytext, strlen(yytext));
			} else {													//Cipher already defined in connector
				DO_SYSL("Ciphers already defined in Connector - ignoring");
				fprintf(stderr,"Ciphers already defined in Connector - ignoring: %s \n",yytext);
			}
		}
	};

SSLCipherSuites:
	TOK_SSL_CIPHER_SUITES TOK_NAME
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

SSLVersion:
	TOK_SSL_VERSION TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
#ifdef USE_SSL
			int max=0,min=0;
			sscanf(yytext,"1.%d-1.%d",&min,&max);
			if (max < min) {		//Switch Values
				int tmp=max;
				max=min;
				min=tmp;
			}
			switch (min) {
				case 0 : connectorinfo->tlsversion_min=TLS1_VERSION;
							break;
				case 1 : connectorinfo->tlsversion_min=TLS1_1_VERSION;
							break;
				case 2 : connectorinfo->tlsversion_min=TLS1_2_VERSION;
							break;
				case 3 : connectorinfo->tlsversion_min=TLS1_3_VERSION;
							break;
				default : connectorinfo->tlsversion_min=TLS1_VERSION;
							 fprintf(stderr,"Error TLS Version String \"%s\"! Min could not be recognized - setting default TLS1.0!\n",yytext);
							 break;
			}
			switch (max) {
				case 0 : connectorinfo->tlsversion_max=TLS1_VERSION;
							break;
				case 1 : connectorinfo->tlsversion_max=TLS1_1_VERSION;
							break;
				case 2 : connectorinfo->tlsversion_max=TLS1_2_VERSION;
							break;
				case 3 : connectorinfo->tlsversion_max=TLS1_3_VERSION;
							break;
				default : connectorinfo->tlsversion_max=TLS1_VERSION;
							 fprintf(stderr,"Error TLS Version String \"%s\"! Max could not be recognized - setting default TLS1.3!",yytext);
							 break;
			}
#endif
		}
//        	fprintf(stderr,"TLS Version String: %s %d-%d\n",yytext,connectorinfo->tlsversion_min,connectorinfo->tlsversion_max);
	};

SSLCert:
	TOK_SSL_CERT TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->server_cert_file == NULL)	{
				CALLOC(connectorinfo->server_cert_file, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->server_cert_file, yytext, strlen(yytext));
			} else {
				DO_SYSL("Config-File: More than one openssl-servercertificate statement in connector");
			}
		}
	};

SSLKey:
	TOK_SSL_KEY TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->server_key_file == NULL)	{
				CALLOC(connectorinfo->server_key_file, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->server_key_file, yytext, strlen(yytext));
			} else {
				DO_SYSL("Config-File: More than one openssl-serverkey statement in connector");
			}
		}
	};

SSLCipherPref:
	TOK_SSL_CIPHERS_PREF
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			connectorinfo->server_cipher_preference=TRUE;
		}
	};

SSLVerify:
	TOK_SSL_VERIFY TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->verify_client == VERIFY_UNDEV)	{		// First in connector
				if (strncmp(yytext,"none",4) == 0) {						// NONE
					connectorinfo->verify_client=VERIFY_NONE;
				} else if (strncmp(yytext,"optional",8) == 0) {			//OPTIONAL
					connectorinfo->verify_client=VERIFY_OPTIONAL;
				} else if (strncmp(yytext,"require",7) == 0) {			//REQUIRE
					connectorinfo->verify_client=VERIFY_REQUIRE;
				} else {
					DO_SYSL("Config-File: openssl-verifyclient must be [none | optional | require]. Ignoring");
				}
			} else {
				DO_SYSL("Config-File: More than one openssl-verifyclient statement in connector");
			}
		}
	};

SSLVerifyDepth:
   TOK_SSL_VERIFY_DEPTH TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         connectorinfo->verify_depth = atoi(yytext);
			if (connectorinfo->verify_depth > 128) {
				DO_SYSL("Config-File: openssl-verifydepth too large [0-128]. Ignoring - using Default (10)");
				connectorinfo->verify_depth=10;
         }
      }
   };

SSLCNAuth:
   TOK_SSL_CN_AUTH
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         connectorinfo->CN_authentication=TRUE;
      }
   };

SSLCRLPath:
   TOK_SSL_CRL_PATH TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         CALLOC(connectorinfo->CRL_path, (char *), strlen(yytext) + 1, sizeof(char));
         strncpy(connectorinfo->CRL_path, yytext, strlen(yytext));
      }
   };

SSLCRLFile:
   TOK_SSL_CRL_FILE TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         CALLOC(connectorinfo->CRL_file, (char *), strlen(yytext) + 1, sizeof(char));
         strncpy(connectorinfo->CRL_file, yytext, strlen(yytext));
      }
   };

SSLCRLCheck:
   TOK_SSL_CRL_CHECK TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         if (connectorinfo->CRL_check == CRL_UNDEV)  {     			// First in connector
            if (strncmp(yytext,"none",4) == 0) {                  // NONE
               connectorinfo->CRL_check=CRL_NONE;
            } else if (strncmp(yytext,"leaf",4) == 0) {       		// LEAF
               connectorinfo->CRL_check=CRL_LEAF;
            } else if (strncmp(yytext,"chain",5) == 0) {        	// CHAIN
               connectorinfo->CRL_check=CRL_CHAIN;
            } else {
               DO_SYSL("Config-File: openssl-CRLcheck must be [none | leaf | chain]. Ignoring");
            }
         } else {
            DO_SYSL("Config-File: More than one openssl-CRLcheck statement in connector");
         }
      }
   };

SSLCAPath:
   TOK_SSL_CA_PATH TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         CALLOC(connectorinfo->CA_path, (char *), strlen(yytext) + 1, sizeof(char));
         strncpy(connectorinfo->CA_path, yytext, strlen(yytext));
      }
   };

SSLCAFile:
   TOK_SSL_CA_FILE TOK_NAME
   {
      if (parser_mode == PARSER_MODE_SERVER) {
         CALLOC(connectorinfo->CA_file, (char *), strlen(yytext) + 1, sizeof(char));
         strncpy(connectorinfo->CA_file, yytext, strlen(yytext));
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
				exit(1);
			}
			use_acl=1;
		}
	}

usePort:
	TOK_PORT TOK_NAME
	{
		if (parser_mode == PARSER_MODE_SERVER) {
			if (connectorinfo->port == 0) {
				connectorinfo->port = atoi(yytext);
				if (!connectorinfo->port) {
					fprintf(stderr, "Port '%s' is not a valid port.\n", yytext);
					exit(1);
				}
//        		fprintf(stderr,"Port: %d\n",connectorinfo->port);
			}else {																		//more than one port definition in connector
				fprintf(stderr, "More than one port statement in connector\n" );
				exit(1);
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
				exit(1);
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
				CALLOC(connectorinfo->listen, (char *), strlen(yytext) + 1, sizeof(char));
				strncpy(connectorinfo->listen, yytext, strlen(yytext));
//		      fprintf(stderr,"Listen on: %s\n",yytext);
			} else {
				fprintf(stderr, "More than one listen statement in connector\n" );
				exit(1);
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

eof:
	TOK_EOF
	{
	};

%%

