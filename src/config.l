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
enable-anonym-mids                  return TOK_ANONYM_MIDS;
listen                              return TOK_LISTEN_ON;
port                                return TOK_PORT;
max-size-of-postings                return TOK_MAX_POST_SIZE;
dynamic-ip-device                   return TOK_DYNIPDEV;
client-limit                        return TOK_CLIENT_LIMIT;
change-uid                          return TOK_CHANGEUID;
change-gid                          return TOK_CHANGEGID;
hash-salt                           return TOK_HASHSALT;
\<connector\>			  					return TOK_CONN_BEGIN;
enable-ssl			    					return TOK_SSL;
enable-starttls			    			return TOK_STARTTLS;
openssl-ciphers			    			return TOK_SSL_CIPHERS;
openssl-tlsversion		    			return TOK_SSL_VERSION;
openssl-servercertificate	    		return TOK_SSL_CERT;
openssl-serverkey		    				return TOK_SSL_KEY;
openssl-server-cipher-preference		return TOK_SSL_CIPHERS_PREF;
openssl-verifyclient						return TOK_SSL_VERIFY;
openssl-verifydepth						return TOK_SSL_VERIFY_DEPTH;
openssl-CNauthentication				return TOK_SSL_CN_AUTH;
openssl-CRLpath							return TOK_SSL_CRL_PATH;
openssl-CRLfile							return TOK_SSL_CRL_FILE;
openssl-CRLcheck							return TOK_SSL_CRL_CHECK;
openssl-CApath								return TOK_SSL_CA_PATH;
openssl-CAfile								return TOK_SSL_CA_FILE;
\<\/connector\>			    			return TOK_CONN_END;
;.*\n                               /* ignore comment */
\|.*\n                              /*   -"-    -"-   */
[ \t\n]+                            /* ignore whitespace and newline */
[a-zA-Z0-9\.\:\\\/\-\_!]+            return TOK_NAME;
\0                                  return TOK_EOF;
%%
