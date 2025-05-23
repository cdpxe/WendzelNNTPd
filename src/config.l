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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylval;
extern char *yytext;

#undef YY_INPUT

#include "config.tab.h"

%}

%%
verbose-mode                        return TOK_VERBOSE_MODE;
database-engine                     return TOK_DB_ENGINE;
database-server                     return TOK_DB_SERVER;
database-username                   return TOK_DB_USER;
database-password                   return TOK_DB_PASS;
database-port                       return TOK_DB_PORT;
use-authentication                  return TOK_USE_AUTH;
use-acl                             return TOK_USE_ACL;
message-body-in-db                  return TOK_MESSAGE_BODY_IN_DB;
message-count-in-db                 return TOK_MESSAGE_COUNT_IN_DB;
enable-anonym-mids                  return TOK_ANONYM_MIDS;
listen                              return TOK_LISTEN_ON;
port                                return TOK_PORT;
max-size-of-postings                return TOK_MAX_POST_SIZE;
dynamic-ip-device                   return TOK_DYNIPDEV;
client-limit                        return TOK_CLIENT_LIMIT;
change-uid                          return TOK_CHANGEUID;
change-gid                          return TOK_CHANGEGID;
hash-salt                           return TOK_HASHSALT;
tls-is-mandatory                    return TOK_TLS_MANDATORY;
\<connector\>			  			return TOK_CONN_BEGIN;
enable-tls			    			return TOK_TLS;
enable-starttls			    		return TOK_STARTTLS;
tls-ciphers			    		    return TOK_TLS_CIPHERS;
tls-cipher-suites			 	    return TOK_TLS_CIPHER_SUITES;
tls-server-certificate	    	    return TOK_TLS_CERT;
tls-server-key		    		    return TOK_TLS_KEY;
tls-ca-certificate				    return TOK_TLS_CA_CERT;
tls-version                         return TOK_TLS_VERSION;
1\.[0-3]-1\.[0-3]                   return TOK_TLS_VERSION_STRING;
tls-verify-client                   return TOK_TLS_VERIFY_CLIENT;
tls-verify-client-depth             return TOK_TLS_VERIFY_CLIENT_DEPTH;
tls-crl                             return TOK_TLS_CRL;
tls-crl-file                        return TOK_TLS_CRL_FILE;
\<\/connector\>			    		return TOK_CONN_END;
;.*\n                               /* ignore comment */
\|.*\n                              /*   -"-    -"-   */
[ \t\n]+                            /* ignore whitespace and newline */
[a-zA-Z0-9\.\:\\\/\-\_!]+           return TOK_NAME;
\0                                  return TOK_EOF;
%%
