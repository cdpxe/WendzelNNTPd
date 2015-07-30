%{

/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2009 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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

sockinfo_t *sockinfo=0;
int peak=0;
int port=DEFAULTPORT;
fd_set fds;
char *xml_outfile = XML_OUTFILE;

unsigned short use_auth=0;	/* global var i check in server.c while socket-loop */
unsigned short be_verbose=0;	/* for debugging reasons */
unsigned short anonym_message_ids=0; /* if eq 1: do not set IP or hostname within message IDs */
int use_xml_output=0;

#define LF_ANY_IP	0x01
#define	LF_SPEC_IP	0x02
int listenflag=0;

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

void
basic_setup(void)
{
#ifdef __WIN32__
	WSADATA wsa_dat;
#endif

	close(0);
	if((yyin=fopen(CONFIGFILE, "r"))==NULL) {
		perror("Unable to open " CONFIGFILE);
		exit(ERR_EXIT);
	}
	if (chk_file_sec(CONFIGFILE) != 0) {
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
}

%}

%token TOK_VERBOSE_MODE
%token TOK_USE_AUTH
%token TOK_USE_XML_OUTPUT
%token TOK_XMLFILE
%token TOK_LISTEN_ON
%token TOK_PORT
%token TOK_DYNIPDEV
%token TOK_CLIENT_LIMIT
%token TOK_DATABASE_DIR
%token TOK_CHANGEUID
%token TOK_CHANGEGID
%token TOK_ANONYM_MIDS
%token TOK_NAME
%token TOK_EOF

%%

commands: /**/ | commands command;

command:  beVerbose | anonMessageIDs | useAuth | useXML | usePort | xmlFile | listenonSpec | eof;

beVerbose:
	TOK_VERBOSE_MODE
	{
		be_verbose=1;
	}

anonMessageIDs:
	TOK_ANONYM_MIDS
	{
		anonym_message_ids=1;
	}

useAuth:
	TOK_USE_AUTH
	{
		use_auth=1;
	};

useXML:
	TOK_USE_XML_OUTPUT
	{
		use_xml_output=1;
	};

usePort:
	TOK_PORT TOK_NAME
	{
		port = atoi(yytext);
		if (!port) {
			fprintf(stderr, "Port %s is not valid.\n", yytext);
			exit(1);
		}
	};

xmlFile:
	TOK_XMLFILE TOK_NAME
	{
		xml_outfile = (char *) calloc(strlen(yytext) + 1, sizeof(char));
		if (!xml_outfile) {
			perror("calloc");
			exit(1);
		}
		strncpy(xml_outfile, yytext, strlen(yytext));
	}

listenonSpec:  /* done */
	TOK_LISTEN_ON TOK_NAME
	{
		int size=0;
		int salen, sa6len;
		int yup=1;
		struct sockaddr_in sa;
		struct sockaddr_in6 sa6;
		char *yytext_=NULL;

		CALLOC(yytext_, (char *), strlen(yytext)+1, sizeof(char))
		strncpy(yytext_, yytext, strlen(yytext));
		
		if(listenflag==LF_ANY_IP) {
			fprintf(stderr, "error: you have to choose between ANY ip address or some specific\n"
							"but you cannot use both features at the same time.\n");
			exit(0);
		}
		listenflag=LF_SPEC_IP;
		
		if(!sockinfo) {
			CALLOC(sockinfo, (sockinfo_t *), 1, sizeof(sockinfo_t))
		} else {
			size=size_sockinfo_t;
			if((sockinfo=realloc(sockinfo, (size+1)*sizeof(sockinfo_t)))==NULL) {
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
		if(inet_pton(AF_INET, yytext_, &sa.sin_addr))
#endif
		{
			sa.sin_port=htons(port);
			sa.sin_family=AF_INET;
			salen=sizeof(struct sockaddr_in);
			
			if(((sockinfo+size)->sockfd=socket(AF_INET, SOCK_STREAM, 0))<0) {
				fprintf(stderr, "cannot do socket() on %s\n", yytext_);
				exit(ERR_EXIT);
			}
			
			setsockopt((sockinfo+size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));
			
			if(bind((sockinfo+size)->sockfd, (struct sockaddr *)&sa, salen)<0) {
				perror("bind");
				fprintf(stderr, "bind() for %s failed.\n", yytext_);
				exit(ERR_EXIT);
			}
			
			if(listen((sockinfo+size)->sockfd, 5)<0) {
				fprintf(stderr, "listen() for %s failed.\n", yytext_);
				exit(ERR_EXIT);
			}
			peak=max((sockinfo+size)->sockfd, peak);
			(sockinfo+size)->family=AF_INET;
#ifndef __WIN32__ /* IPv6-ready systems */
		} else if(inet_pton(AF_INET6, yytext_, &sa6.sin6_addr)) {
			sa6.sin6_port=htons(port);
			sa6.sin6_family=AF_INET6;
			sa6len=sizeof(struct sockaddr_in6);
			
			if(((sockinfo+size)->sockfd=socket(AF_INET6, SOCK_STREAM, 0))<0) {
				fprintf(stderr, "cannot do socket() on %s\n", yytext_);
				exit(ERR_EXIT);
			}
			
			setsockopt((sockinfo+size)->sockfd, SOL_SOCKET, SO_REUSEADDR, &yup, sizeof(yup));
			
			if(bind((sockinfo+size)->sockfd, (struct sockaddr *)&sa6, sa6len)<0) {
				fprintf(stderr, "bind() for %s failed.\n", yytext_);
				exit(ERR_EXIT);
			}
			
			if(listen((sockinfo+size)->sockfd, 5)<0) {
				fprintf(stderr, "listen() for %s failed.\n", yytext_);
				exit(ERR_EXIT);
			}
			peak=max((sockinfo+size)->sockfd, peak);
			(sockinfo+size)->family=AF_INET6;
#endif
		} else {
			fprintf(stderr, "Invalid address: %s\n", yytext_);
			exit(ERR_EXIT);
		}
		free(yytext_);
		size_sockinfo_t++;
	};

eof:
	TOK_EOF
	{
	};

%%

