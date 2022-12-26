/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2021 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
 * https://www.wendzel.de
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
#include <dirent.h>

#include "cdpstrings.h"

#define MAX_CMDLEN		8196 /* this was much smaller, but Xana-News sends multiple
				      * requests within ONE packet. */
#define NNTP_HDR_NG_SEP_STR	","

/* OK global vars */
extern char lowercase[256];
extern int daemon_mode;		/* main.c */
extern unsigned short use_auth;	/* config.y */
extern unsigned short use_acl; /* config.y */
extern unsigned short use_tls; /* config.y */
extern unsigned short tls_is_mandatory; /* config.y */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	NNTP Messages
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
char helpstring[]=            "100 help text follows\r\n"
                                 "--\r\n"
                                 "\tarticle [number|<message-id>]\r\n"
				 "\tauthinfo <user|pass> <username|password>\r\n"
                                 "\tbody [number|<message-id>]\r\n"
                                 "\tcapabilities\r\n"
                                 "\tdate\r\n"
                                 "\tgroup <group>\r\n"
                                 "\thead [number|<message-id>]\r\n"
                                 "\thelp\r\n"
                                 "\tlist [overview.fmt|newsgroups [wildmat*]]\r\n"
                                 "\tlistgroup [group]\r\n"
                                 "\tmode reader (always returns 200)\r\n"
                                 "\tpost\r\n"
                                 "\tquit\r\n"
                                 "\tstarttls\r\n"
                                 "\tstat [number|<message-id>]\r\n"
                                 "\txhdr <from|date|newsgroups|subject|lines> <number[-[endnum]]|msgid>\r\n"
				"\txover <from[-[to]]>\r\n"
                                 "\txgtitle [wildmat*] (equals LIST NEWSGROUPS but return code differs)\r\n"
                                 "--\r\n"
                                 "Send questions and problems to <wendzel [at] hs-worms [dot] de>\r\n"
                                 "Website: https://www.wendzel.de\r\n"
                                 "\r\n"
                                 "Notes:\r\n"
                                 "* Wildmat format is based on the used regex library and is not 100%\r\n"
                                 "  wildmat format compatible (XGTITLE *x -> XGTITLE .*x)!\r\n"
                                 ".\r\n";
char list_overview_fmt_info[]="215 Order of fields in overview database.\r\n"
				"Subject:\r\n"
				"From:\r\n"
				"Date:\r\n"
				"Message-ID:\r\n"
				"References:\r\n"
				"Bytes:\r\n"
				"Lines:\r\n"
				"Xref:full\r\n"
				".\r\n";
char capabilites_string[]=    "101 capability list follows\r\n";
char welcomestring[]=         "200 WendzelNNTPd " WELCOMEVERSION " ready (posting ok).\r\n";
char mode_reader_ok[]=        "200 hello, you can post\r\n";
char quitstring[]=            "205 closing connection - goodbye!\r\n";
char article_list_follows[]=  "211 Article list follows\r\n";
char list[]=                  "215 list of newsgroups follows\r\n";
char list_newsgroups[]=       "215 information follows\r\n";
char xhdr[]=                  "221 Header follows\r\n";
char xhdr_hdrnotpresent[]=    "221 Header follows\r\n.\r\n";
char xover[]=                 "224 overview information follows\r\n";
char postdone[]=              "240 article posted\r\n";
char xgtitle[]=               "282 list of groups and descriptions follows\r\n";
char postok[]=                "340 send article to be posted. End with <CR-LF>.<CR-LF>\r\n";
char tls_connect[]=           "382 continue with TLS negotiation\r\n";
char tls_failed[]=            "400 Connection closing due to lack of security\r\n";
char nosuchgroup[]=           "411 no such group\r\n";
char nogroupselected[]=       "412 no news group current selected\r\n";
char noarticleselected[]=     "420 no (current) article selected\r\n";
char nosucharticle[]=         "430 no such article found\r\n";
char hdrerror_subject[]=      "441 'subject' line needed or incorrect.\r\n";
char hdrerror_from[]=         "441 'from' line needed or incorrect.\r\n";
char hdrerror_newsgroup[]=    "441 'newsgroups' line needed or incorrect.\r\n";
char posterr_posttoobig[]=    "441 posting too huge.\r\n";
char posterror_notallowed[]=  "441 posting failed (you selected a newsgroup in which posting is not permitted).\r\n";
char auth_req[]=              "480 authentication required.\r\n";
char tls_req[]=               "483 TLS encryption required.\r\n";
char unknown_cmd[]=           "500 unknown command\r\n";
char parameter_miss[]=        "501 missing a parameter, see 'help'\r\n";
char cmd_not_supported[]=     "502 command not implemented\r\n";
char progerr503[]=            "503 program error, function not performed\r\n";
char post_too_big[]=          "503 posting size too big (administratively prohibited)\r\n";
char tls_error[]=             "580 can not initiate TLS negotiation\r\n";
char period_end[]=            ".\r\n";

static char *get_slinearg(char *, int);
static void Send(server_cb_inf *, char *, int);
static int Receive(server_cb_inf *, char *, int);
static void do_command(char *, server_cb_inf *);
static void docmd_list(char *, server_cb_inf *, int);
static void docmd_authinfo_user(char *, server_cb_inf *);
static void docmd_authinfo_pass(char *, server_cb_inf *);
static void docmd_xover(char *, server_cb_inf *);
static void docmd_article(char *, server_cb_inf *);
static void docmd_group(char *, server_cb_inf *);
static void docmd_listgroup(char *, server_cb_inf *);
static int docmd_post_chk_ng_name_correctness(char *, server_cb_inf *);
static int docmd_post_chk_required_hdr_lines(char *, server_cb_inf *);
static void docmd_post(server_cb_inf *);
static void docmd_mode_reader(server_cb_inf *);
static void docmd_capabilities(server_cb_inf *);
static unsigned short check_tls_mandatory(server_cb_inf *);

/* this function returns a command line argv[]. counting (=num) starts
 * by 0 */

static char *
get_slinearg(char *cmdstring, int num)
{
	char *ptr;
	char *tmp;
	int len;
	int cnt;

	cnt = 0;

	for (ptr = cmdstring; ptr[0] != '\0'; ptr++) {
		if (ptr[0] != ' ' && ptr[0] != '\r') {
			if (cnt == num) {
				/* copy the word in tmp and return it */
				tmp = ptr;
				len = 0;
				while (ptr[0] != ' ' && ptr[0] != '\0' && ptr[0] != '\r') {
					ptr++;
					len++;
				}
				tmp = strndup(tmp, len);
				if (!tmp) {
					DO_SYSL("get_slinearg strndup error.")
					fprintf(stderr, "out of mem in " __FILE__ ", line %i "
							"(strndup())", __LINE__);
					return NULL;
				}
				return tmp;
			} else {
				/* jump to the next word */
				while (ptr[0] != ' ' && ptr[0] != '\0')
					ptr++;
				/* break up, if this was the last word */
				if (ptr[0] == '\0')
					return NULL;
			}
			cnt++;
		}
	}
	return NULL;
}

static void
Send(server_cb_inf *inf, char *str, int len)
{
	if (inf->servinf->tls_is_there) {
#ifndef NOGNUTLS
		if(gnutls_record_send(inf->servinf->tls_session, str, len)<0) {
			if (daemon_mode) {
				DO_SYSL("gnutls_record_send() returned <0 -- killing connection.")
			} else {
				perror("gnutls_record_send");
			}
			pthread_exit(NULL);
		}
#endif
#ifndef NOOPENSSL
		int return_code;
		return_code = SSL_write(inf->servinf->tls_session, str, len);
		if (return_code <= 0) {
			if (daemon_mode) {
				DO_SYSL("SSL_write() returned <= 0 -- killing connection.")
			} else {
				DO_SYSL("SSL_write() returned <= 0 -- killing connection.")
				int err = SSL_get_error(inf->servinf->tls_session, return_code);
				if (err == SSL_ERROR_SSL) {
					fprintf(stderr, "SSL error: ");
					ERR_print_errors_fp(stderr);
				}
			}
			pthread_exit(NULL);
		}
#endif
	} else {
		if(send(inf->sockinf->sockfd, str, len, MSG_NOSIGNAL)<0) {
			if (daemon_mode) {
				DO_SYSL("send() returned <0 -- killing connection.")
			} else {
				perror("send");
			}
			pthread_exit(NULL);
		}
	}
}

void
ToSend(char *str, int len, server_cb_inf *inf)
{
	int curlen;

	if (inf->servinf->curstring == NULL) {
		inf->servinf->curstring=(char *)calloc(len+1, sizeof(char));
		if(inf->servinf->curstring==NULL) {
#ifdef DEBUG
			perror("malloc");
#endif
			DO_SYSL("not enough memory!")
			kill_thread(inf);
			/*NOTREACHED*/
		}
		strncpy(inf->servinf->curstring, str, len);
	} else {
		curlen=strlen(inf->servinf->curstring);
		inf->servinf->curstring=(char *)realloc(inf->servinf->curstring, curlen+len+1);
		if(inf->servinf->curstring==NULL) {
			DO_SYSL("not enough memory!")
			onxxdebug("memory low, disconnecting child and killing the child process.\n");
			kill_thread(inf);
			/* NOTREACHED */
		}
		strncpy(inf->servinf->curstring + curlen, str, len);
		inf->servinf->curstring[curlen+len]='\0';
	}
}

int
Receive(server_cb_inf *inf, char *str, int len)
{
	int recv_bytes = -1;
	if (inf->servinf->tls_is_there) {
#ifndef NOGNUTLS
		do {
			recv_bytes = gnutls_record_recv(inf->servinf->tls_session, str, len);
		} while (recv_bytes == GNUTLS_E_AGAIN || recv_bytes == GNUTLS_E_INTERRUPTED);
#endif
#ifndef NOOPENSSL
		recv_bytes = SSL_read(inf->servinf->tls_session, str, len);
#endif
	} else {
		recv_bytes = recv(inf->sockinf->sockfd, str, len, 0);
	}

	return recv_bytes;
}

void
nntp_localtime_to_str(char tbuf[40], time_t ltime)
{
	/* RFC-850-Format: Wdy, DD Mon YY HH:MM:SS TIMEZONE */
#if defined(NOSUPPORT_STRFTIME_z_FLAG) && !defined(__WIN32__)
	/* Solaris 8 makes some problems here. Why didn't they implement the damn '%z'?
	 * A good thing I noticed in Feb-2008: OpenSolaris 2008-11 _has_ %z -- good!
	 * Note: Make special Win32 check here too because Win32 uses no configure script!
	 */
	strftime(tbuf, 39, "%a, %d %b %Y %H:%M:%S %Z", localtime(&ltime));
#else
	strftime(tbuf, 39, "%a, %d %b %Y %H:%M:%S %z", localtime(&ltime));
#endif
}

static unsigned short
check_tls_mandatory(server_cb_inf *inf)
{
	if (use_tls) {
		if (!inf->servinf->tls_is_there && tls_is_mandatory) {
			ToSend(tls_req, strlen(tls_req), inf);
			return FALSE;
		}
	}
	return TRUE;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	CAPABILITES
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_capabilities(server_cb_inf *inf)
{
	char cap_nntp_version[]="VERSION 2\r\n";

	char cap_authinfo[]="AUTHINFO\r\n";
	char cap_list[]="LIST NEWSGROUPS OVERVIEW.FMT\r\n";
	char cap_mode_reader[]="MODE-READER\r\n";
	char cap_post[]="POST\r\n";
	char cap_starttls[]="STARTTLS\r\n";

	/* XOVER and XHDR can not be advertized via CAPABILITES */

	ToSend(capabilites_string, strlen(capabilites_string), inf);

	/* VERSION MUST be first line */
	ToSend(cap_nntp_version, strlen(cap_nntp_version), inf);

	ToSend(cap_authinfo, strlen(cap_authinfo), inf);
	ToSend(cap_list, strlen(cap_list), inf);

	/* MODE READER can NOT be switched when TLS active */
	if (!inf->servinf->tls_is_there) {
		ToSend(cap_mode_reader, strlen(cap_mode_reader), inf);
	}

	ToSend(cap_post, strlen(cap_post), inf);

	/* STARTTLS only if not already on TLS */
	if (!inf->servinf->tls_is_there) {
		ToSend(cap_starttls, strlen(cap_starttls), inf);
	}

	ToSend(period_end, strlen(period_end), inf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	STARTTLS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_starttls(server_cb_inf *inf)
{
	if (inf->servinf->tls_is_there) {
		ToSend(cmd_not_supported, strlen(cmd_not_supported), inf);
	} else {
		ToSend(tls_connect, strlen(tls_connect), inf);
		inf->servinf->switch_to_tls = 1;
	}
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	MODE READER
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_mode_reader(server_cb_inf *inf)
{
	/* MODE READER must NOT be available on TLS connections */
	if (inf->servinf->tls_is_there) {
		ToSend(unknown_cmd, strlen(unknown_cmd), inf);
	} else {
		ToSend(mode_reader_ok, strlen(mode_reader_ok), inf);
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUTHINFO USER / AUTHINFO PASS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* Do not check if a user really exists in docmd_authinfo_user because this would make it
 * easier for attackers to get access!
 */

static void
docmd_authinfo_user(char *cmdstring, server_cb_inf *inf)
{
	char need_more_inf[]="381 More authentication information required.\r\n";
	char *user;

	inf->servinf->auth_is_there=0;
	if (!(user = get_slinearg(cmdstring, 2))) {
		ToSend(parameter_miss, strlen(parameter_miss), inf);
		return;
	}
	if (inf->servinf->cur_auth_user)
		free(inf->servinf->cur_auth_user);

	inf->servinf->cur_auth_user = user;
	ToSend(need_more_inf, strlen(need_more_inf), inf);
}

static void
docmd_authinfo_pass(char *cmdstring, server_cb_inf *inf)
{
	char need_more_inf[]="381 More authentication information required.\r\n";
	char auth_accept[]="281 Authentication accepted.\r\n";
	char auth_reject[]="482 Authentication rejected.\r\n";
	char *pass, *pass_hash = NULL;
	char *log_str = NULL;

	inf->servinf->auth_is_there=0;

	if (inf->servinf->cur_auth_user) {
		if (!(pass = get_slinearg(cmdstring, 2))) {
			ToSend(parameter_miss, strlen(parameter_miss), inf);
			return;
		}
		if (inf->servinf->cur_auth_pass)
			free(inf->servinf->cur_auth_pass);
		pass_hash = get_sha256_hash_from_str(inf->servinf->cur_auth_user, pass);
		if (!pass_hash) {
			DO_SYSL("Internal error: get_sha256_from_str() returned an error, probably out of memory. Cannot authenticate user.")
			kill_thread(inf);
			return;
		}
		inf->servinf->cur_auth_pass = pass_hash;

		/* do the whole authentication check on DB */
		db_authinfo_check(inf);

		if(inf->servinf->auth_is_there==0) {
			ToSend(auth_reject, strlen(auth_reject), inf);
			log_str = str_concat("Authentication REJECTED for user ",
						inf->servinf->cur_auth_user,
						" from IP ", inf->sockinf->ip, NULL);

		} else {
			ToSend(auth_accept, strlen(auth_accept), inf);
			log_str = str_concat("Authentication accepted for user ",
						inf->servinf->cur_auth_user,
						" from IP ", inf->sockinf->ip, NULL);
		}
		if (log_str) {
			DO_SYSL(log_str)
			free(log_str);
		}

		/* delete the username to prevent password brute-force attacks */
		if (use_acl != 1) {
			free(inf->servinf->cur_auth_user); /* do this for security reasons! */
			inf->servinf->cur_auth_user = NULL;
		}
		/* overwrite password to prevent memory leaks */
		memset(pass, 0x0, strlen(pass));
		memset(pass_hash, 0x0, strlen(pass_hash));
		free(inf->servinf->cur_auth_pass);
		inf->servinf->cur_auth_pass = NULL;
	} else {
		/* the client first has to send a username */
		ToSend(need_more_inf, strlen(need_more_inf), inf);
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	DATE
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_date(server_cb_inf *inf)
{
	char datestr[50]={'\0'};
	time_t ltime;

	ltime = time(NULL);
	strftime(datestr, sizeof(datestr), "111 %Y%m%d%H%M%S\r\n", localtime(&ltime));
	ToSend(datestr, strlen(datestr), inf);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	LIST, LIST NEWSGROUPS [wildmat], XGTITLE [wildmat]
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_list(char *cmdstring, server_cb_inf *inf, int cmdtyp)
{
	char *wildmat = NULL;

	switch (cmdtyp) {
	case CMDTYP_LIST:
		ToSend(list, strlen(list), inf);
		break;
	case CMDTYP_XGTITLE:
		if (!(wildmat = get_slinearg(cmdstring, 1 /* 1 less than list newsgroups! */))) {
			ToSend(parameter_miss, strlen(parameter_miss), inf);
			return;
		}
		ToSend(xgtitle, strlen(xgtitle), inf);
		break;
	case CMDTYP_LIST_NEWSGROUPS:
		if (!(wildmat = get_slinearg(cmdstring, 2))) {
			/* if no wildmat is given, we list all newsgroups, according to RFC 3977
			 * therefore, set wildmat to '.*'. */
			 wildmat = ".*";
		}
		ToSend(list_newsgroups, strlen(list_newsgroups), inf);
		break;
	default:
		fprintf(stderr, "internal error");
		ToSend(progerr503, strlen(progerr503), inf);
		exit(1);
		break;
	}
	db_list(inf, cmdtyp, wildmat);
	ToSend(period_end, strlen(period_end), inf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	XHDR
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* possible: 'xhdr hdr <msgid>'
 *            'xhdr hdr num'
 *            'xhdr hdr num-'
 *            'xhdr hdr num-num'
 *
 * returns:	"%s %s\r\n", articlenum, hdrline
 */

static void
docmd_xhdr(char *cmdstring, server_cb_inf *inf)
{
	short req_message_id = 0;
	u_int32_t min, max;
	u_int32_t REALmax;
	char *ptr;
	short xhdr_part;

	/* if (no newsgroup is selected) -> return a 412 error */
	if (inf->servinf->selected_group == NULL) {
		ToSend(nogroupselected, strlen(nogroupselected), inf);
		return;
	}

	/* get the REALmax value (max-num in the newsgroups table) */
	REALmax = db_get_high_value(inf, inf->servinf->selected_group);

	/* get the command */
	ptr = get_slinearg(cmdstring, 1);
	if (!ptr) {
		DO_SYSL("unable to parse command line")
		ToSend(noarticleselected, strlen(noarticleselected), inf);
		return;
	}
	if (strncasecmp(ptr, "from", 4) == 0) {
		xhdr_part = XHDR_FROM;
	} else if (strncasecmp(ptr, "date", 4) == 0) {
		xhdr_part = XHDR_DATE;
		/* we must convert the time() returned value stored in the db to a
		 * real standard conform string */
		inf->speccmd = SPECCMD_DATE;
	} else if (strncasecmp(ptr, "newsgroups", 10) == 0) {
		xhdr_part = XHDR_NEWSGROUPS;
	} else if (strncasecmp(ptr, "subject", 7) == 0) {
		xhdr_part = XHDR_SUBJECT;
	} else if (strncasecmp(ptr, "lines", 5) == 0) {
		xhdr_part = XHDR_LINES;
	} else {
		/* Say that header does not exist; RFC says '(none)' is
		 * valid response. However, we also do not check for 'to'
		 * and other hdr lines. Quote:
		 * >>... return the 221 response code followed by a period on a line by
		 * itself..<< */
		ToSend(xhdr_hdrnotpresent, strlen(xhdr_hdrnotpresent), inf);
		return;
	}
	free(ptr);

	/* get the min + max values */
	ptr = get_slinearg(cmdstring, 2);
	if (ptr == NULL) { /* no value is given by the client => use current article */
		if (inf->servinf->selected_article == NULL) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		min = (u_int32_t) atoi(inf->servinf->selected_article);
		max = min;
	} else { /* at least a min-value is given */
		/* now check if the client requests either a message number
		 * (num[-[to]]) or a message id (<message-id>) */
		if (ptr[0] == '<') { /* message ID */
			req_message_id = 1;
		} else {
			char *range_ptr = ptr;

			req_message_id = 0;
			min = atoi(range_ptr);
			max = min; /* This is correct; if we find a '-',
			* we will change it in a moment */

			/* check for the '-' character */
			while (range_ptr[0] != '\0' && max != REALmax) {
				if(range_ptr[0] == '-') {
					/* all articles from min to the end */
					max = REALmax;
				}
				range_ptr++;
			}
			if (max == REALmax) {
				/* look if there is an end-value for max */
				if (range_ptr[0] != '\0') {
					max = atoi(range_ptr);
					/* If the client sent us bullshit then
					 * max could be zero ... */
					if (!max) {
						/* ... in this case, we set it
						 * back to REALmax ;-) */
						max = REALmax;
					}
				}
			}
		}
	}
	/*onxxdebugm("%s%i%s%i%s%i%s",
	           "min: ", min, " max: ", max, " REALmax: ", REALmax, "\n");*/
	/* check if the requested message ID exists */
	if (req_message_id == 1) {
		if (db_chk_if_msgid_exists(inf, inf->servinf->selected_group, ptr) == 0) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
	}
	/* if no articles are in the range -> return a 420 error */
	else if (min > REALmax || max > REALmax || min > max) {
		ToSend(noarticleselected, strlen(noarticleselected), inf);
		return;
	}

	/* now send the list */
	ToSend(xhdr, strlen(xhdr), inf);

	/* now let the DB to the Scheissjob :-) */
	db_xhdr(inf, req_message_id, xhdr_part, ptr, min, max);
	if (ptr)
		free(ptr);
	ToSend(period_end, strlen(period_end), inf);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	XOVER
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* possible: 'xover'
 *           'xover num'
 *           'xover num-'
 *           'xover num-num'
 */

static void
docmd_xover(char *cmdstring, server_cb_inf *inf)
{
	u_int32_t min, max;
	u_int32_t REALmax;
	char *ptr;

	/* if (no newsgroup is selected) -> return a 412 error */
	if (inf->servinf->selected_group == NULL) {
		ToSend(nogroupselected, strlen(nogroupselected), inf);
		return;
	}

	/* get the REALmax value (max-num in the newsgroups table) */
	REALmax = db_get_high_value(inf, inf->servinf->selected_group);

	/* get the min + max values */
	ptr = get_slinearg(cmdstring, 1);
	if (ptr == NULL) { /* no value is given by the client => use current article */
		if (inf->servinf->selected_article == NULL) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		min = (u_int32_t) atoi(inf->servinf->selected_article);
		max = min;
	} else { /* at least a min-value is given */
		char *ptr_orig = ptr;

		min = atoi(ptr); /* atoi(), by definition, detects no errors, e.g. maybe
					* we convert a string like 'cat' to some number here, but
					* then we simply cannot return the posting as it will not
					* exist. Some plausibility checks are done later in this
					* function. */

		max = min; /* This is correct. If we find a '-', we will change it in a moment */

		/* check for the '-' character. */
		while (ptr[0] != '\0' && max != REALmax) {
			if (ptr[0] == '-') { /* all articles from min to the end */
				max = REALmax;
			}
			ptr++;
		}
		if (max == REALmax) { /* look if there is an end-value for max */
			if (ptr[0] != '\0') {
				max = atoi(ptr);
				/* if the client sent us bullshit then max could be <=0 */
				if (!max) {
					/* ... in this case, we set it back to REALmax ;-) */
					max = REALmax;
				}
			}
		}

		free(ptr_orig);
	}

	onxxdebugm("%s%i%s%i%s%i%s",
	           "min: ", min, " max: ", max, " REALmax: ", REALmax, "\n");

	/* if no articles are in the range -> return a 420 error */
	if ( min > REALmax
		/*|| (max > REALmax) <-- this was actually a good thing, but
		 * the crappy usenet clients, like pan, just simply request
		 * independently of the real max values sent to them! Result:
		 * they do not display anything! */
		|| min > max) {
		ToSend(noarticleselected, strlen(noarticleselected), inf);
		return;
	}

	/* now send the list */
	ToSend(xover, strlen(xover), inf);

	db_xover(inf, min, max);
	ToSend(period_end, strlen(period_end), inf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ARTICLE
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_article(char *cmdstring, server_cb_inf *inf)
{
	int type=0;
	char *param;
	int i;
	int found=0;

	/* the cb func will set this to '1' if we found an article */
	inf->servinf->found_article = 0;

	if (cmdstring[0]=='A' || cmdstring[0]=='a') inf->cmdtype = CMDTYP_ARTICLE;
	else if(cmdstring[0]=='H' || cmdstring[0]=='h') inf->cmdtype = CMDTYP_HEAD;
	else if(cmdstring[0]=='B' || cmdstring[0]=='b') inf->cmdtype = CMDTYP_BODY;
	else if(cmdstring[0]=='S' || cmdstring[0]=='s') inf->cmdtype = CMDTYP_STAT;

	/* if(no newsgroup is selected) -> return a 412 error */
	if(inf->servinf->selected_group == NULL) {
		ToSend(nogroupselected, strlen(nogroupselected), inf);
		return;
	}

	/* point to the parameter-substring of the cmd-string */
	param = get_slinearg(cmdstring, 1);
	if (param == NULL) {
		type = ARTCLTYP_CURRENT;
	} else {
		/* okay, not the case. but shall we use the message-id or the news-id? */
		if (param[0] == '<') {
			type = ARTCLTYP_MESSAGEID;
		} else if (param[0] >= '0' && param[0] <= '9') {
			type = ARTCLTYP_NUMBER;
		} else {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
	}

	/* create first response line (code + info + message like "230 3 38 head follows") */
	switch(type) {
	case ARTCLTYP_MESSAGEID:
		// get the message-id-string
		for (i = 0; i < (int)strlen(param); i++) {
			if (param[i] == '>') {
				found = 1;
				break;
			}
		}
		if (!found) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		param[i+1] = '\0';

		db_article(inf, type, param);
		break;

	case ARTCLTYP_NUMBER:
		db_article(inf, type, param);
		break;

	case ARTCLTYP_CURRENT:
		/* first check whether an article is selected */
		if (inf->servinf->selected_article == NULL) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		db_article(inf, type, NULL /* no article identifier */);
		break;
	}

	if (param) /* can be NULL due to ARTCLTYP_CURRENT case */
		free(param);

	/* if this is still zero, no cb function was called -> no article was found */
	if (inf->servinf->found_article == 0) {
		ToSend(nosucharticle, strlen(nosucharticle), inf);
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	GROUP
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* From RFC 977:
	 * If an invalid group is specified, the
	 previously selected group and article remain selected. CHK

	 * If an empty
	 newsgroup is selected, the "current article pointer" is in an
	 indeterminate state and should not be used.
*/

static void
docmd_group(char *cmdstring, server_cb_inf *inf)
{
	char *ptr;
	int old_foundgroup = inf->servinf->found_group;

	ptr = get_slinearg(cmdstring, 1);
	if (!ptr) {
		ToSend(parameter_miss, strlen(parameter_miss), inf);
		return; /* let him try again */
	}

	/* Check newsgroup existence here */
	inf->servinf->chkname = ptr;
	inf->servinf->found_group = 0;
	db_check_newsgroup_existence(inf);

	/* if the group was not found (=the cb function was not called) ... */
	if (inf->servinf->found_group == 0) {
		/* send "no such group" */
		ToSend(nosuchgroup, strlen(nosuchgroup), inf);
		free(ptr);
		/* Restore */
		inf->servinf->found_group = old_foundgroup;
		/* Return here and leave the selected_{group,article} untouched -> RFC 977 */
		return;
	}

	if (use_auth && use_acl) {
		/* If the user is not allowed to access this group: shadow the group! */
		if (acl_check_user_group(inf, inf->servinf->cur_auth_user, ptr) == FALSE) {
			ToSend(nosuchgroup, strlen(nosuchgroup), inf);
			free(ptr);
			return;
		}
	}
	db_group(inf, ptr);
	free(ptr);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	LISTGROUP [ggg]
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
docmd_listgroup(char *cmdstring, server_cb_inf *inf)
{
	char *ptr;

	ptr = get_slinearg(cmdstring, 1);
	/* If no argv was given, we need a currently selected group */
	if (ptr == NULL) {
		if (inf->servinf->selected_group == NULL) {
			ToSend(nogroupselected, strlen(nogroupselected), inf);
			return; /* let him try again */
		}
		/* db_listgroup() is used with ptr, so we need to set it here! */
		ptr = strdup(inf->servinf->selected_group);
		if (!ptr) {
			DO_SYSL("dup() ret NULL")
			fprintf(stderr, "dup() returned NULL (see logfile)");
			return;
		}
	} else {
		int old_foundgroup = inf->servinf->found_group;

		/* Check newsgroup existence */
		inf->servinf->chkname = ptr;
		db_check_newsgroup_existence(inf);

		/* if the group was not found ... */
		if (inf->servinf->found_group == 0) {
			/* send "no such group" */
			/* restore old found_group value */
			inf->servinf->found_group = old_foundgroup;
			ToSend(nosuchgroup, strlen(nosuchgroup), inf);
			return;
		}

		/* Okay, group exists; make it the currently selected group (RFC need!) */
		if (inf->servinf->selected_group)
			free(inf->servinf->selected_group);
		inf->servinf->selected_group = strdup(ptr);
		if (!inf->servinf->selected_group) {
			DO_SYSL("strdup() error!")
			fprintf(stderr, "strdup() error!");
			kill_thread(inf);
			/* NOTREACHED */
		}
	}

	/* does the user (still) has access to the group? */
	if (use_auth && use_acl) {
		/* If the user is not allowed to access this group: shadow the group! */
		if (acl_check_user_group(inf, inf->servinf->cur_auth_user, ptr) == FALSE) {
			ToSend(nosuchgroup, strlen(nosuchgroup), inf);
			free(ptr);
			return;
		}
	}
	ToSend(article_list_follows, strlen(article_list_follows), inf);
	db_listgroup(inf, ptr);
	free(ptr);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	POST
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define SEND_441ERR(s) {			\
	if(!correctline) {			\
		ToSend(s, strlen(s), inf);	\
		return FALSE;			\
	}					\
	correctline=FALSE;			\
}

static int
docmd_post_chk_required_hdr_lines(char *header, server_cb_inf *inf)
{
	#define DOMAIN_NAME "[a-zA-Z0-9.-]+"
	#define EMAIL_NAME "[a-zA-Z0-9._+-]+"
	int correctline=FALSE;

	/* Newsgroups: */
	if (wnntpd_rx_contain("^[nN][eE][wW][sS][gG][rR][oO][uU][pP][sS]: [a-zA-Z0-9.-]+(,[a-zA-Z0-9.-]+)*\r\n", header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_newsgroup)

	/* From: */
	/* "blah@blah.com" */
	/* Comments:
	 * - This allows "max....mustermann@muster.com", but we let it pass here.
	 */
	if (wnntpd_rx_contain("^[fF][rR][oO][mM]: "EMAIL_NAME"@"DOMAIN_NAME"\r\n", header) == 0)
		correctline = TRUE;
	/* blah@blah.com (Name Name) */
	if (wnntpd_rx_contain("^[fF][rR][oO][mM]: "EMAIL_NAME"@"DOMAIN_NAME" ([^\r\n\t]+)\r\n", header) == 0)
		correctline=TRUE;
	/* Name [... [Name]] <blah@blah.com> */
	if (wnntpd_rx_contain("^[fF][rR][oO][mM]: [^\r\n\t]* <"EMAIL_NAME"@"DOMAIN_NAME">\r\n",
				header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_from)

	/* Subject: */
	if (wnntpd_rx_contain("^[sS][uU][bB][jJ][eE][cC][tT]: [^\r\n\t]*\r\n", header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_subject)

	return TRUE;
}

static int
docmd_post_chk_ng_name_correctness(char *ngstrp_in, server_cb_inf *inf)
{
	char *ngstrp = NULL;
	char *newsgroup;
	const char sep[]=NNTP_HDR_NG_SEP_STR;
#ifndef __WIN32__
	char *saveptr; /* both for strtok_r() */
#endif

	/* work only with a copy of the string because we need the original later */
	ngstrp = strdup(ngstrp_in);
	if (!ngstrp) {
		DO_SYSL("Not enough mem for strdup() in docmd_post_chk_ng_name_correctness()")
		return FALSE;
	}

	/* get the first newsgroup + check if all newsgroups exists */
	for (newsgroup =
#ifndef __WIN32__
	strtok_r(ngstrp, sep, &saveptr)
#else
	strtok(ngstrp, sep)
#endif
	; newsgroup; ) {
		/* 1. check whether group exists */
	 	inf->servinf->chkname = db_secure_sqlbuffer(inf, newsgroup);
		db_check_newsgroup_existence(inf);
		db_secure_sqlbuffer_free(inf->servinf->chkname);
		inf->servinf->chkname = NULL;
		if (!inf->servinf->found_group) {
			ToSend(nosuchgroup, strlen(nosuchgroup), inf);
			return FALSE;
		}

		/* 2. check whether posting is allowed */
	 	inf->servinf->chkname = db_secure_sqlbuffer(inf, newsgroup);
		db_check_newsgroup_posting_allowed(inf);
		db_secure_sqlbuffer_free(inf->servinf->chkname);
		inf->servinf->chkname = NULL;
		if (!inf->servinf->found_group) {
			ToSend(posterror_notallowed, strlen(posterror_notallowed), inf);
			return FALSE;
		}

		/* 3. reset for next group */
		inf->servinf->found_group = 0;

		/* get the next group */
#ifndef __WIN32__
		newsgroup = strtok_r(NULL, sep, &saveptr);
#else
		newsgroup = strtok(NULL, sep);
#endif
	}
	return TRUE;
}

static void
docmd_post(server_cb_inf *inf)
{
	extern unsigned short anonym_message_ids; /* config.y */
	extern int max_post_size; /* config.y */
#define MAX_IPLEN 49
	int len=0;
	size_t lenb = 0;
	size_t e;
	int i;
	u_int32_t cur_high;
	size_t s;
	const char sep[]=NNTP_HDR_NG_SEP_STR;
	char *newid;
	struct hostent *hostinfo = NULL;
	char *addr;
	char *from    = NULL,
	     *subj    = NULL,
	     *body    = NULL,
	     *header  = NULL,
	     *add_to_hdr = NULL,
	     *buf     = NULL,
	     *ptr     = NULL;
	char *message_id = NULL;
	char *ngstrp;
	char *newsgroup;
	char *remove_lines[] = { "Message-ID: ", "Date: ", "Lines: ", NULL };
	char tbuf[40]={'\0'}; /* strftime() */
#ifndef __WIN32__
	char *saveptr; /* both for strtok_r() */
#endif
	int linecount=0;
	time_t ltime;
	charstack_t *stackp = NULL;

	Send(inf, postok, strlen(postok));

	{
		size_t recv_bytes = 0;
		int finished = 0;
		fd_set fds_post;
		int peak = 0;
		struct timeval tv;

		if ((buf = (char *) calloc(max_post_size, sizeof(char))) == NULL) {
			perror("malloc");
			DO_SYSL("memory low. killing child process.")
			kill_thread(inf);
			/* NOTREACHED */
		}
		while (!finished) {
			bzero(&tv, sizeof(struct timeval));
			tv.tv_sec = 1;

			FD_ZERO(&fds_post);
			FD_SET(inf->sockinf->sockfd, &fds_post);
			peak = inf->sockinf->sockfd;

			if (select(peak + 1, &fds_post, NULL, NULL, &tv) == -1) {
				if (errno == EINTR) {
					continue;
				} else {
					perror("select() in docmd_post()");
					free(buf);
					kill_thread(inf);
					/* NOTREACHED */
				}
			}
			if (FD_ISSET(inf->sockinf->sockfd, &fds_post)) {
				ssize_t recv_ret;
				u_int32_t offset;
				char *haystack_ptr;

				/* check if already the max. number of allowed bytes
				 * were received. As FD_ISSET() returned true, there
				 * are bytes left nevertheless, i.e. posting is too
				 * big.
				 */
				if (max_post_size - recv_bytes <= 1) {
					Send(inf, post_too_big, strlen(post_too_big));
					fprintf(stderr, "Posting is larger than allowed max (%i Bytes) "
						"in docmd_post()\n", max_post_size);
					DO_SYSL("posting from client larger than allowed max. value. "
						"Please check the documentation if you want to allow "
						"larger postings.")
					free(buf);
					kill_thread(inf);
					/* NOTREACHED */
				}

				recv_ret = Receive(inf, buf + recv_bytes, max_post_size - recv_bytes - 1);
				if ((int)recv_ret == -1) {
					perror("recv()");
					DO_SYSL("posting recv() error!");
					free(buf);
					kill_thread(inf);
					/* NOTREACHED */
				} else if ((int)recv_ret == 0) {
					/* Manpage says: 0 is returned when peer
					 * performs an ordinary shutdown.
					 */
					perror("Client disconnected in docmd_post()");
					free(buf);
					kill_thread(inf);
					/* NOTREACHED */
				}
				recv_bytes += recv_ret;

				/* BEGIN: check whether \r\n.\r\n is included */
				/* 1) calculate where we need to look within the
				 *    buf[fer] */
				if (recv_bytes > 5) {
					/* to catch \r\n.\r\n even if it is
					 * partly located in previously recv()'ed
					 * data */
					offset = recv_bytes - 5;
				} else {
					offset = 0;
				}
				/* 2) try to find \r\n.\r\n */
				if ((haystack_ptr = strstr(buf+offset, "\r\n.\r\n")) != NULL) {
					/* we found the end of the posting.
					 * for stability and security reasons, all
					 * content behind \r\n.\r\n will be cut. */
					haystack_ptr[5] = '\0';
					/* since found: end while() loop */
					finished = 1;
				}
				/* END: Check for \r\n.\r\n */

			}
		}
	}

	/*onxxdebugm("%s%s%s", "buffer: '", buf, "'\n");*/
	/*fprintf(stderr, "buffer: '%s'\n", buf);*/

	/*
	 * create a buffer called 'header' that only contains the header part of 'buf'
	 */
	for (i = 0; buf[i + 4] != '\0' && header == NULL; i++) {
		if (strncmp(buf + i, "\r\n\r\n", 4) == 0) {
			/* the end of the header */
			if ((header = (char *) calloc(sizeof(char), i
					+ 2/*add the \r\n for the regex checks */
					+ 1/*\0*/)) == NULL) {
				perror("calloc");
				DO_SYSL("mem low. killing child process.")
				free(buf);
				kill_thread(inf);
				/* NOTREACHED */
			}
			strncpy(header, buf, i + 2);
		}
	}
	onxxdebugm("%s%s%s", "header: '", header, "'\n");
	if (!header) {
		DO_SYSL("client sent garbage posting. terminating connection.")
		free(buf);
		kill_thread(inf);
		/* NOTREACHED */
	}

	body = strstr(buf, "\r\n\r\n") + 4;
	if (body[0] == '\0') {
		free(buf);
		free(header);
		ToSend(progerr503, sizeof(progerr503), inf);
		return;
	}

	/*
	 * different header checks
	 */

	if (docmd_post_chk_required_hdr_lines(header, inf) == FALSE) {
		free(buf);
		free(header);
		return;
	}

	/*
	 * Check if the newsgroup-names are valid
	 */

	/* get the newsgroups-string value (func is NOT case sensitive) */
	if (!(ngstrp = CDP_return_linevalue(header, "Newsgroups:"))) {
		free(buf);
		free(header);
		ToSend(hdrerror_newsgroup, strlen(hdrerror_newsgroup), inf);
		return;
	}
	/* now do the first checks */
	if (docmd_post_chk_ng_name_correctness(ngstrp /* creates a copy of ngstrp, original stays untouched!*/, inf) == FALSE) {
		free(buf);
		free(header);
		return;
	}

	/*
	 * Generate a Message ID
	 */

	/* get a uniq number */
	if (!(newid = get_uniqnum())) {
		DO_SYSL("I/O error in file DB -> can't create a new msg-id. Terminating child connection.")
		free(buf);
		free(header);
		kill_thread(inf);
		/* NOTREACHED */
	}
	/*printf("new uniq-id: %s\n", newid);*/


	/* If the admin wants no anonymous message IDs: Find out hostname or use IP */
	if (anonym_message_ids == 0) {
		/* get the host of the sender */
		if (!(addr = (char *) calloc(MAX_IPLEN, sizeof(char)))) {
			DO_SYSL("not enough memory")
			free(buf);
			free(header);
			kill_thread(inf);
			/* NOTREACHED */
		}

		if (inf->sockinf->family == FAM_4) {
	#ifdef __WIN32__ /* win32 sux */
			free(addr);
			addr = inet_ntoa(inf->sockinf->sa.sin_addr);
			if(!addr)
	#else
			if (!inet_ntop(AF_INET, &inf->sockinf->sa.sin_addr, addr, MAX_IPLEN))
	#endif
			{
				DO_SYSL("Can't get address of socket! Terminating child connection.")
				free(buf);
				free(header);
				kill_thread(inf);
				/* NOTREACHED */
			}
			hostinfo = gethostbyaddr((char *)&inf->sockinf->sa.sin_addr,
				sizeof(inf->sockinf->sa.sin_addr), AF_INET);
	#ifndef __WIN32__
		} else { /* IPnG */
			inet_ntop(AF_INET6, &inf->sockinf->sa6.sin6_addr, addr, MAX_IPLEN);
			if(!addr) {
				DO_SYSL("Can't get address of socket! Terminating child connection.")
				free(buf);
				free(header);
				kill_thread(inf);
				/* NOTREACHED */
			}
			hostinfo = gethostbyaddr((char *)&inf->sockinf->sa6.sin6_addr,
				sizeof(inf->sockinf->sa6.sin6_addr), AF_INET6);
	#endif
		}

		if(!hostinfo) {
			/* Hostname is not present. Use "addr" instead! */
			len=MAX_IDNUM_LEN+3+strlen(addr) + 1;
			if (!(message_id = (char *) calloc(len, sizeof(char)))) {
				DO_SYSL("Not enough memory")
				free(buf);
				free(header);
				kill_thread(inf);
				/* NOTREACHED */
			}
			snprintf(message_id, MAX_IDNUM_LEN+3+strlen(addr), "%s%s>", newid, addr);
		} else {
			len=MAX_IDNUM_LEN+3+strlen(hostinfo->h_name) + 1;
			if (!(message_id = (char *) calloc(len, sizeof(char)))) {
				DO_SYSL("Not enough memory")
				free(buf);
				free(header);
				kill_thread(inf);
				/* NOTREACHED */
			}
			snprintf(message_id, MAX_IDNUM_LEN+3+strlen(addr), "%s%s>", newid, hostinfo->h_name);
			//free(hostinfo);
		}
		free(addr);/* NEW */
	} else {
		/* Create an anonymous Message-ID */
		len = MAX_IDNUM_LEN + 3 + strlen(NNTPD_ANONYM_HOST) + 1;
		if (!(message_id = (char *) calloc(len, sizeof(char)))) {
			DO_SYSL("not enough memory")
			free(buf);
			free(header);
			kill_thread(inf);
			/* NOTREACHED */
		}
		snprintf(message_id, len - 1, "%s" NNTPD_ANONYM_HOST ">", newid);
	}
	/* Free the newly created Message-ID since we finally built the message_id */
	free(newid);

	/*
	 * Manipulate the Header (Do not trust the Message-ID, Line-Count and Date
	 * sent by the Client)
	 */

	/* if there is a message ID, remove it (new is inserted by the write_posting() func */
	if ((ptr = strstr(buf, "\r\n\r\n")) == 0) {
		DO_SYSL("(internal) parsing error #1")
		free(buf);
		free(header);
		kill_thread(inf);
		/* NOTREACHED */
	}
	len = ptr - buf;

	/* Remove 'Message-ID/Date/Lines';
	   Note: header is != NULL here. Otherwise function would have terminated earlier */
	for (e = 0; remove_lines[e] != NULL;  e++) {
		for (i = 0; i < len; i++) {
			if (strncasecmp(header+i, remove_lines[e], strlen(remove_lines[e])) == 0) {
#ifdef DEBUG
				fprintf(stderr, "'remove_lines' Field %s found! Removing...\n", remove_lines[e]);
#endif
				// find the end of the line
				for (lenb = i + strlen(remove_lines[e]) - 1; header[lenb] != '\r'
				   && header[lenb + 1] != '\n'; lenb++) {
					/* do nothing */
				}
				lenb += 2;
				/* now add the rest of the header from position i on */
				while (lenb <= strlen(header)) {
					header[i] = header[lenb];
					i++;
					lenb++;
				}
				header[i] = '\0';
				break;
			}
		}
	}

	/*
	 * Post it!
	 */

	/*
	 * add message in the Postings table
	 */

	/* Get the current time */
	ltime = time(NULL);
	if (ltime == (time_t) - 1) {
		free(buf);
		free(header);
		DO_SYSL("time(NULL) returned (time_t)-1")
		kill_thread(inf);
		/* NOTREACHED */
	}

	nntp_localtime_to_str(tbuf, ltime);

	/* Get the # of lines */
	if ((ptr=strstr(buf, "\r\n\r\n"))==NULL) {
		DO_SYSL("posting aborted (no end of header found, client sent garbage).")
		free(buf);
		free(header);
		kill_thread(inf);
		/* NOTREACHED */
	}

	{
		/* WendzelNNTPd-2.0.1:
		 * strlen(ptr) takes multiple seconds if posting >100k!
		 * Thus, I added a new var strlen_ptr used in the loop to
		 * safe MUCH time. However, it is still too slow :/ */
		size_t strlen_ptr = strlen(ptr);
		for (s=4, linecount=-1; s<strlen_ptr; s++) {
			if(ptr[s]=='\n') {
				linecount++;
			} else if(ptr[s]=='.' && s==strlen_ptr-2) {
				continue;
			}
		}
	}

	/* now, with the new values, generate the new part of the header ... */
	if (!(add_to_hdr = (char *) calloc(0x7ff + strlen(message_id) + 2*127 /* 127 chars for
			the hostname + 127 chars for the domain name /should/ be enough */
			+ strlen(header) /* for strcat() */, sizeof(char)))) {
		DO_SYSL("not enough memory.")
		free(buf);
		free(header);
		kill_thread(inf);
		/* NOTREACHED */
	}

	{
		/* Try to set the FQDN here */
		char hostname[128] = { '\0' };
		char domainname[128] = { '\0' };
		char fqdn[256] = { '\0' };
		char unknown[] = "unknown\0";

		if (gethostname(hostname, 127) == 0) {
			strncpy(fqdn, hostname, strlen(hostname));
		} else {
			strncpy(fqdn, unknown, strlen(unknown));
		}
		/* now also get the domain name */
#ifdef __WIN32__ /* ... but not on Win32 */
		fqdn[strlen(fqdn)] = '.';
		strncpy(fqdn + strlen(fqdn), "win32", strlen("win32"));
#else /* okay, here we really get the domain name */
		if (getdomainname(domainname, 127) == 0 && strncmp(domainname, "(none)", 6) != 0) {
			fqdn[strlen(fqdn)] = '.';
			strncpy(fqdn + strlen(fqdn), domainname, 127);
		} else {
			fqdn[strlen(fqdn)] = '.';
			strncpy(fqdn + strlen(fqdn), unknown, strlen(unknown));
		}
#endif
		/* Add FQDN + other important headers */
		snprintf(add_to_hdr, 0x7fe,
			"Path: %s\r\nMessage-ID: %s\r\nDate: %s\r\nLines: %i\r\nX-WendzelNNTPdBodysize: %u\r\n",
			fqdn, message_id, tbuf, linecount, (unsigned int)strlen(body));
	}

	/*
	 * add the posting to the database
	 */

	subj = CDP_return_linevalue(header, "Subject:");
	from = CDP_return_linevalue(header, "From:");

	/* ... and make the header complete */
	strcat(add_to_hdr, header);
	free(header);

	/* ALERT: this is the last occurrence of body; don't free it, since it
	 * is part of 'buf'! */
	filebackend_savebody(message_id, body);
	free(buf);

	/* SQL safe execution of the user submitted buffer parts */
	{
		char *from_sec,
		     *subj_sec;

		from_sec = db_secure_sqlbuffer(inf, from);
		subj_sec = db_secure_sqlbuffer(inf, subj);

		db_post_insert_into_postings(inf, message_id, ltime, from_sec,
			ngstrp, subj_sec, linecount, add_to_hdr);

		db_secure_sqlbuffer_free(from_sec);
		db_secure_sqlbuffer_free(subj_sec);
	}

	free(from);
	free(subj);
	free(add_to_hdr);

	/*
	 * and add one entry for every group in ngposts table and increment their
	 * groups 'high' value.
	 */
#ifndef __WIN32__
	newsgroup = strtok_r(ngstrp, sep, &saveptr);
#else
	newsgroup = strtok(ngstrp, sep);
#endif

	if (!(stackp = (charstack_t *) calloc(1, sizeof(charstack_t)))) {
		DO_SYSL("Not enough mem -- posting aborted.")
		kill_thread(inf);
		/* NOTREACHED */
	}
	stackp->state = STACK_EMPTY;

	while (newsgroup != NULL) {

		if ((use_auth && use_acl) &&
			(acl_check_user_group(inf, inf->servinf->cur_auth_user, newsgroup) == FALSE)) {
			char err_msg[512] = {'\0'};
			snprintf(err_msg, sizeof(err_msg) - 1,
				"User %s tried to post to newsgroup %s what is denied by ACL. "
				"Skipping this entry.", inf->servinf->cur_auth_user, newsgroup);
			DO_SYSL(err_msg);
		} else {

			cur_high = db_get_high_value(inf, newsgroup);
			if (cur_high > 0x7ffffffe) { /* :-) */
				DO_SYSL("Warning: This server only can handle 0x7fffffff "
					"(= 2.147.483.646) articles!")
				DO_SYSL("Warning: Article limit exeeded.")
				kill_thread(inf);
				/* NOTREACHED */

			}

			// Only do INSERT, if this 'newsgroup' name occurs the first time in this posting
			// since a 'Newsgroups: my.group,my.group' (a double post) would lead to a doubled
			// msgid in the table, what sqlite3 will not accept since this is the primary key!
			// This also prevents double-postings to a newsgroup.
			if (charstack_check_for(stackp, newsgroup) == STACK_NOTFOUND) {
				db_post_insert_into_ngposts(inf, message_id, newsgroup, cur_high + 1);
				/* update high value */
				db_post_update_high_value(inf, cur_high + 1, newsgroup);

				if (charstack_push_on(stackp, newsgroup) != OK_RETURN) {
					DO_SYSL("charstack_push_on() returned an error. This can lead to a closed "
						"client connection if one newsgroup was selected multiple times "
						"for a single posting (what shouldn't be the normal case)")
					/* this kill prevents while-true-loops */
					kill_thread(inf);
					/* NOTREACHED */
				}
			} else {
				DO_SYSL("Catched a duplicated newsgroup posting (posted it only once to the group).")
			}
		}
#ifndef __WIN32__
		newsgroup = strtok_r(NULL, sep, &saveptr);
#else
		newsgroup = strtok(NULL, sep);
#endif
	}

	charstack_free(stackp);
	/* don't free() 'ngstrp' earlier since strtok(NULL, ...) uses the
	 * address of it! */
	free(ngstrp);

	ToSend(postdone, strlen(postdone), inf);
	free(message_id);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	COMMAND HANDLER
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
do_command(char *recvbuf, server_cb_inf *inf)
{

#define QUESTION(cmd, len)	strncasecmp(recvbuf, cmd, len)==0
#define QUESTION_AUTH(cmd, len)	( (inf->servinf->auth_is_there || use_auth == 0) && strncasecmp(recvbuf, cmd, len)==0)

	/* COMMANDS THAT NEED _NO_ AUTHENTICATION */
	/* Check "AAAA" before "AAA" to make sure we match the correct command here! */
	if (QUESTION("quit", 4)) {
	  Send(inf, quitstring, strlen(quitstring));
		kill_thread(inf);
		/* NOTREACHED */
	} else if (QUESTION("authinfo user ", 14)) {
		if (check_tls_mandatory(inf)) {
			docmd_authinfo_user(recvbuf, inf);
		}
	} else if (QUESTION("authinfo pass ", 14)) {
		if (check_tls_mandatory(inf)) {
			docmd_authinfo_pass(recvbuf, inf);
		}
	} else if (QUESTION("capabilities", 12)) {
		docmd_capabilities(inf);
	} else if (QUESTION("starttls", 8)) {
		docmd_starttls(inf);
	}

	/* COMMANDS THAT NEED AUTHENTICATION */
	/* Check "AAAA" before "AAA" to make sure we match the correct command here! */

	else if (QUESTION_AUTH("list newsgroups", 15)) {
		if (check_tls_mandatory(inf)) {
			docmd_list(recvbuf, inf, CMDTYP_LIST_NEWSGROUPS);
		}
	} else if (QUESTION_AUTH("list overview.fmt", 17)) {
			ToSend(list_overview_fmt_info, strlen(list_overview_fmt_info), inf);
	} else if (QUESTION_AUTH("listgroup", 9)) {
		if (check_tls_mandatory(inf)) {
			docmd_listgroup(recvbuf, inf);
		}
	} else if (QUESTION_AUTH("list", 4)) {
		if (check_tls_mandatory(inf)) {
			docmd_list(recvbuf, inf, CMDTYP_LIST);
		}
	} else if (QUESTION_AUTH("xgtitle", 7)) {
		if (check_tls_mandatory(inf)) {
			docmd_list(recvbuf, inf, CMDTYP_XGTITLE);
		}
	} else if (QUESTION_AUTH("help", 4)) {
		ToSend(helpstring, strlen(helpstring), inf);
	} else if (QUESTION_AUTH("group", 5)) {
		if (check_tls_mandatory(inf)) {
			docmd_group(recvbuf, inf);
		}
	} else if (QUESTION_AUTH("xover", 5)) {
		if (check_tls_mandatory(inf)) {
			docmd_xover(recvbuf, inf);
		}
	} else if (QUESTION_AUTH("xhdr", 4)) {
		if (check_tls_mandatory(inf)) {
			docmd_xhdr(recvbuf, inf);
		}
	} else if (QUESTION_AUTH("article", 7) || QUESTION_AUTH("head", 4)
		|| QUESTION_AUTH("body", 4) || QUESTION_AUTH("stat", 4)) {
		if (check_tls_mandatory(inf)) {
			docmd_article(recvbuf, inf);
		}
	} else if (QUESTION_AUTH("mode reader", 11)) {
		docmd_mode_reader(inf);
	} else if (QUESTION_AUTH("post", 4)) {
		if (check_tls_mandatory(inf)) {
			docmd_post(inf);
		}
	} else if (QUESTION_AUTH("date", 4)) {
		docmd_date(inf);
	} else if (QUESTION_AUTH("NEWNEWS", 7)) {
		ToSend(cmd_not_supported, strlen(cmd_not_supported), inf);
	} else {

		if(inf->servinf->auth_is_there || (use_auth == 0)) {
			ToSend(unknown_cmd, strlen(unknown_cmd), inf);
#ifdef DEBUG
			printf("cmd '%s' not implemented.\n", recvbuf);
#endif
		} else {
			ToSend(auth_req, strlen(auth_req), inf);
		}
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	MAINLOOP
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void *
do_server(void *socket_info_ptr)
{
	char recvbuf[MAX_CMDLEN+1];
	char *sec_cmd;
	int len=0;
	sockinfo_t *sockinf;
	serverinfo_t servinf;
	server_cb_inf inf;
	extern short be_verbose;
	int i;

	/* This is _SERV_-inf */
	bzero(&servinf, sizeof(serverinfo_t));
	/* This is _SOCK_-inf */
	sockinf = (sockinfo_t *)socket_info_ptr;
	/* This is 'inf' containing everything */
	inf.sockinf = sockinf;
	inf.servinf = &servinf;

	db_open_connection(&inf);

	if (inf.sockinf->is_tls) {
		if (!tls_session_init(&inf.servinf->tls_session, inf.sockinf->sockfd)) {
			DO_SYSL("Could not init TLS session. Exiting.");
			fprintf(stderr, "Could not init TLS session. Exiting\n");
			kill_thread(&inf);
		} else {
			inf.servinf->tls_is_there = 1;
			Send(&inf, welcomestring, strlen(welcomestring));
		}
	} else {
		Send(&inf, welcomestring, strlen(welcomestring));
	}

	if(use_auth==1) {
		servinf.auth_is_there=0;
	} else {
		servinf.auth_is_there=1;
	}

	while(1) {
		if (len == 0) {
			bzero(recvbuf, MAX_CMDLEN);
		}

		/* receive only one byte each time; not good for performance but allows to deal
		 * with crappy clients who send multiple requests within one request. */
		/* 1. kill connection if the client sends more bytes than allowed */
		if (len == MAX_CMDLEN) {
			kill_thread(&inf);
		}

		/* switch to TLS in server code, must be done outside docmd_ */
		static unsigned short tls_switch_fails = 0;
		if (inf.servinf->switch_to_tls) {
			inf.servinf->switch_to_tls = 0;
			if (!tls_session_init(&inf.servinf->tls_session, inf.sockinf->sockfd)) {
				if (++tls_switch_fails < 3) {
					Send(&inf, tls_error, strlen(tls_error));
				} else {
					Send(&inf, tls_failed, strlen(tls_failed));
					kill_thread(&inf);
				}
			} else {
				inf.servinf->tls_is_there = 1;

				/* Reset all data of the user when switchting to TLS (RFC 4642 requires this) */
				if (inf.servinf->selected_group) {
					free(inf.servinf->selected_group);
					inf.servinf->selected_group = NULL;
				}
				if (inf.servinf->selected_article) {
					free(inf.servinf->selected_article);
					inf.servinf->selected_article = NULL;
				}
				if (inf.servinf->cur_auth_user) {
					free(inf.servinf->cur_auth_user);
					inf.servinf->cur_auth_user = NULL;
				}
				if (inf.servinf->cur_auth_pass) {
					free(inf.servinf->cur_auth_pass);
					inf.servinf->cur_auth_pass = NULL;
				}
#ifdef DEBUG
				fprintf(stderr, "client switched to TLS.\n");
				FFLUSH
#endif
			}
		}

		/* 2. receive byte-wise */
		int return_val = -1;
		return_val = Receive(&inf, recvbuf+len, 1);
		if (return_val <= 0) {
			/* kill connection in problem case */
			kill_thread(&inf);
			/* NOTREACHED */
		}

		if (strstr(recvbuf, "\r\n") != NULL) {
			if (be_verbose) {
				if (strncasecmp(recvbuf, "authinfo pass", 13) == 0) {
					fprintf(stderr, "client sent 'authinfo pass 'xxxxx'\n");
				} else if (strncasecmp(recvbuf, "authinfo user", 13) == 0) {
					fprintf(stderr, "client sent 'authinfo user 'xxxxx'\n");
				} else {
					fprintf(stderr, "client sent '%s'\n", recvbuf);
				}
			}

			/* make the buffer more secure before going on */
			/* 1. remove trailing \r\n by replacing \r with \0 */
			for (i = (strlen(recvbuf) - 1); i > 0; i--) {
				if (recvbuf[i] == '\r') {
					recvbuf[i] = '\0';
					i = 0; /* = break */
				}
			}
			/* 2. run db-library security functions */
			sec_cmd = db_secure_sqlbuffer(&inf, recvbuf);

			/* now proceed */
			do_command(sec_cmd, &inf);
			db_secure_sqlbuffer_free(sec_cmd);

			Send(&inf, servinf.curstring, strlen(servinf.curstring));

			free(servinf.curstring);
			servinf.curstring=NULL;

			len=0;
		} else {
			len = strlen(recvbuf);
		}
	}
	/* NOTREACHED */
	kill_thread(&inf);
	/* NOTREACHED */
	return NULL;
}

void
kill_thread(server_cb_inf *inf)
{
	char *conn_logstr = NULL;

	/* TODO:  if there's still something in the send() queue, then run Send() */
	// Run send() manually here and do under NO CIRCUMSTANCES use the function defined for that
	// because the function can run kill_thread() on error what could result in a recursive calling
	// what again could result in a DoS!
	// => run only 'send()' here.


	/* Log the ended connection */
	conn_logstr = str_concat("Closed connection from ", inf->sockinf->ip, NULL, NULL, NULL);
	if (conn_logstr) {
		DO_SYSL(conn_logstr)
		free(conn_logstr);
	}

	/* close db connection */
	db_close_connection(inf);

	/* shutdown TLS */
	if (inf->servinf->tls_is_there) {
		tls_session_close(inf->servinf->tls_session);
	}

#ifdef __WIN32__
	closesocket(inf->sockinf->sockfd);
#else
	close(inf->sockinf->sockfd);
#endif

#ifdef DEBUG
	fprintf(stderr, "client connection closed.\n");
	FFLUSH
#endif
	/* TODO: free some mem from buffers of sockinf/servinf here ... */
	/* ... */
	//free(inf);
	if (inf->servinf->cur_auth_pass)
		free(inf->servinf->cur_auth_pass);
	if (inf->servinf->cur_auth_user)
		free(inf->servinf->cur_auth_user);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

