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
 
/* basic setup */
#ifdef __WIN32__
   #define SPOOLFOLDER		"."
   #define CONFIGFILE		"wendzelnntpd.conf"
   #define DBFILE		"usenet.db"	/* SQlite3 */
   #define LOGFILE		"log.txt"
   #define MSGID_FILE		"nextmsgid"
#else
   #define CONFIGFILE		CONFDIR "/wendzelnntpd.conf"
   #define SPOOLFOLDER		"/var/spool/news/wendzelnntpd"
   #define DBFILE		SPOOLFOLDER "/usenet.db" /* SQlite3 */
   #define MSGID_FILE		SPOOLFOLDER "/nextmsgid"
   #define LOGFILE		"/var/log/wendzelnntpd"
#endif

/* The maximum size of a NNTP posting */
#define MAX_POSTSIZE_DEFAULT	20*1024*1024	/* 20M */


