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

#include "main.h"
#include <dirent.h>

#ifdef __WIN32__
   #include <rxposix.h>
#else
   #include <regex.h>
#endif

#include "cdpstrings.h"

#define MAX_CMDLEN	512

/* OK global vars */
extern char lowercase[256];
extern int daemon_mode;		/* main.c */
extern unsigned short use_auth;	/* config.y */
extern int use_xml_output;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	NNTP Messages 
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
char helpstring[]=            "100 help text follows\r\n"
                                 "--\r\n"
                                 "\tarticle <number | message-id>\r\n"
                                 "\tbody\r\n"
                                 "\tdate\r\n"
                                 "\tgroup <group>\r\n"
                                 "\thead\r\n"
                                 "\thelp\r\n"
                                 "\tlist\r\n"
                                 //"\t$_list newsgroups (not implemented but understood)\r\n"
                                 /*"\tmode reader (always returns 200)\r\n"*/
                                 "\tpost\r\n"
                                 "\tquit\r\n"
                                 "\tstat\r\n"
                                 "\txhdr <from|date|newsgroups|subject|lines> <number[-[endnum]]|msgid>\r\n"
                                 "\txover <from[-[to]]>\r\n"
                                 "--\r\n"
                                 "send questions + problems to <swendzel [at] ploetner-it [dot] de>\r\n"
                                 ".\r\n";
char welcomestring[]=         "200 WendzelNNTPd " WELCOMEVERSION " ready (posting ok).\r\n";
char mode_reader_ok[]=        "200 hello, you can post\r\n";
char quitstring[]=            "205 closing connection - goodbye!\r\n";
char list[]=                  "215 list of newsgroups follows\r\n";
char xhdr[]=                  "221 Header follows\r\n";
char xover[]=                 "224 overview information follows\r\n";
char postdone[]=              "240 article posted\r\n";
char postok[]=                "340 send article to be posted. End with <CR-LF>.<CR-LF>\r\n";
char nosuchgroup[]=           "411 no such group\r\n";
char nogroupselected[]=       "412 no news group current selected\r\n";
char noarticleselected[]=     "420 no (current) article selected\r\n";
char nosucharticle[]=         "430 no such article found\r\n";
char hdrerror_subject[]=      "441 'subject' line needed or incorrect.\r\n";
char hdrerror_from[]=         "441 'from' line needed or incorrect.\r\n";
char hdrerror_newsgroup[]=    "441 'newsgroups' line needed or incorrect.\r\n";
char posterr_posttoobig[]=    "441 posting too huge.\r\n";
char auth_req[]=              "480 authentication required.\r\n";
char unknown_cmd[]=           "500 unknown command\r\n";
char no_group_given[]=        "501 need a newsgroup\r\n";
char progerr503[]=            "503 programm error, function not performed\r\n";
char period_end[]=            ".\r\n";

static void Send(int, char *, int);
void nntp_localtime_to_str(char [40], time_t);
static void do_command(char *, server_cb_inf *);
static void docmd_list_sql(server_cb_inf *);
static void docmd_authinfo_user(char *, server_cb_inf *);
static void docmd_authinfo_pass(char *, server_cb_inf *);
static void docmd_xover(char *, server_cb_inf *);
static void docmd_article(char *, server_cb_inf *);
static void docmd_group(char *, server_cb_inf *);
static void docmd_post_sql(server_cb_inf *);
static char *get_slinearg(char *, int);

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
Send(int lsockfd, char *str, int len)
{
	if(send(lsockfd, str, len, 0)<0) {
		if (daemon_mode)
			DO_SYSL("send() returned  -lt 0. killing connection.")
		else
			perror("send");
		pthread_exit(NULL);
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
			/*NOTREACHED*/
		}
		strncpy(inf->servinf->curstring + curlen, str, len);
		inf->servinf->curstring[curlen+len]='\0';
	}
}

void
nntp_localtime_to_str(char tbuf[40], time_t ltime)
{
	/* RFC-850-Format: Wdy, DD Mon YY HH:MM:SS TIMEZONE */
#if defined(NOSUPPORT_STRFTIME_z_FLAG) && !defined(__WIN32__)
	/* Solaris 8 makes some problemes here. Why didn't they implement the fucking '%z'?
	 * A good thing I noticed in Feb-2008: OpenSolaris 2008-11 _has_ %z -- good!
	 * Note: Make special Win32 check here too because Win32 uses no configure script!
	 */
	strftime(tbuf, 39, "%a, %d %b %y %H:%M:%S %Z", localtime(&ltime));
#else
	strftime(tbuf, 39, "%a, %d %b %y %H:%M:%S %z", localtime(&ltime));
#endif
}

/* this was a good idea but acctually it does nothing more than executing sqlite3_exec. I do
 * all the checks by hand, what is better because it is needed only two times in this whole
 * file! Btw. This is good because I can better add the database abstraction layer later!
 */
void
sqlite3_secexec(server_cb_inf *inf, char *cmd, int (*cb)(void *, int, char **, char **), void *arg)
{
	if (sqlite3_exec(inf->servinf->db, cmd, cb, arg,
			 &inf->servinf->sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "SQLite3 error: %s\n", inf->servinf->sqlite_err_msg);
		sqlite3_free(inf->servinf->sqlite_err_msg);
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("sqlite3_SECexec(): Unable to exec on database.")
		kill_thread(inf);
		/*NOTREACHED*/
	}
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUTHINFO USER / AUTHINFO PASS
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* Do not check if a user realy exists in docmd_authinfo_user because this would make it
 * easier for attackers to get access!
 */
  
static void
docmd_authinfo_user(char *recvbuf, server_cb_inf *inf)
{
	char need_more_inf[]="381 More authentication information required.\r\n";
	char *user;
	
	inf->servinf->auth_is_there=0;
	
	if (!(user = get_slinearg(recvbuf, 2))) {
		/* error msg comes from get_slinearg() */
		kill_thread(inf);
		/*NOTREACHED*/
	}
	
	if (inf->servinf->cur_auth_user)
		free(inf->servinf->cur_auth_user);
	
	inf->servinf->cur_auth_user = user;
	ToSend(need_more_inf, strlen(need_more_inf), inf);
}

static int
docmd_authinfo_pass_cb(void *infp, int argc, char **argv, char **ColName)
{
        ((server_cb_inf *)infp)->servinf->auth_is_there = 1;
	return 0;
}

static void
docmd_authinfo_pass(char *recvbuf, server_cb_inf *inf)
{
	char need_more_inf[]="381 More authentication information required.\r\n";
	char auth_accept[]="281 Authentication accepted.\r\n";
	char auth_reject[]="482 Authentication rejected.\r\n";	
	char *pass;
	int len;
	char *sql_cmd;
	char *log_str = NULL;
	
	inf->servinf->auth_is_there=0;

	if(inf->servinf->cur_auth_user) {
		if (!(pass = get_slinearg(recvbuf, 2))) {
			/* error msg comes from get_slinearg() */
			kill_thread(inf);
			/*NOTREACHED*/
		}
		if (inf->servinf->cur_auth_pass)
			free(inf->servinf->cur_auth_pass);
		inf->servinf->cur_auth_pass = pass;
		
		/* now check if combination of user+pass is valid */
		len = strlen(inf->servinf->cur_auth_user)+strlen(inf->servinf->cur_auth_pass)+0xff;
		if (!(sql_cmd = (char *)calloc(len, sizeof(char)))) {
			kill_thread(inf);
			/*NOTREACHED*/
		}
		snprintf(sql_cmd, len - 1, "select * from users where name='%s' and password='%s';",
			inf->servinf->cur_auth_user, inf->servinf->cur_auth_pass);
		sqlite3_secexec(inf, sql_cmd, docmd_authinfo_pass_cb, inf);
		free(sql_cmd);
		
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
		free(inf->servinf->cur_auth_user); /* do this for security reasons! */
		inf->servinf->cur_auth_user = NULL;
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
	LIST
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* format: <group> <high> <low> <pflag> */
static int
docmd_list_cb(void *infp, int argc, char **argv, char **ColName)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	char addbuf[1024] = { '\0' };
	/* the first message is '1' if we already have an article, else it will be zero */
	int already_posted = atoi(argv[3]);
	
	snprintf(addbuf, 1023, "%s %s %i %s\r\n", argv[1], argv[3], (already_posted ? 1 : 0), argv[2]);
	ToSend(addbuf, strlen(addbuf), inf);
	return 0;
}

static void
docmd_list_sql(server_cb_inf *inf)
{
	char cmd[]= "select * from Newsgroups;";

	ToSend(list, strlen(list), inf);
#ifdef DEBUG
	printf("executing cmd: %s\n", cmd);
#endif
	sqlite3_secexec(inf, cmd, docmd_list_cb, inf);
	
	ToSend(period_end, strlen(period_end), inf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	XHDR
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* possible: 'xhdr hdr <msgid>'
 *           'xhdr hdr num'
 *           'xhdr hdr num-'
 *           'xhdr hdr num-num'
 *
 * returns:  "%s %s\r\n", articlenum, hdrline
 */

/* argv:       0         1
 * select n.postnum, p.<hdrpart>
 */

static int
docmd_xhdr_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int len;
	int articlenum;
	char *add_to_response;
	static char none[] = "(none)";
	char *value;
	char tbuf[40] = { '\0' };
	server_cb_inf *linf;
	
	linf = (server_cb_inf *) infp;
	
	articlenum = atoi(argv[0]);
	
	/* If the requested header-part is supported, we get its value in argv[1],
	 * if not, we get NULL since we did not request a send parameter in the
	 * SQL query.
	 */
	if (argv[1] != NULL)
		value = argv[1];
	else
		value = none;
	
	/* catch date values we need to convert! */
	if (linf->speccmd == SPECCMD_DATE) {
		nntp_localtime_to_str(tbuf, atol(argv[1]));
		value = tbuf;
	}
	
	len = 0xfff + 1 + strlen(value) + 2 + 1;
	
	CALLOC_Thread(add_to_response, (char *), len, sizeof(char))
	snprintf(add_to_response, len - 1, "%i %s\r\n", articlenum, value);
	ToSend(add_to_response, strlen(add_to_response), inf);
	
	free(add_to_response);
	return 0;
}

static void
docmd_xhdr(char *cmdstring, server_cb_inf *inf)
{
	short req_message_id = 0;
	u_int32_t min, max;
	u_int32_t REALmax;
	int len;
	char *ptr;
	char *sql_cmd;
	char *hdr_type_str;
	
	/* if (no newsgroup is selected) -> return a 412 error */
	if (inf->servinf->selected_group == NULL) {
		ToSend(nogroupselected, strlen(nogroupselected), inf);
		return;
	}
	
	/* get the REALmax value (max-num in the newsgroups table) */	
	REALmax = DB_get_high_value(inf->servinf->selected_group, inf);
	
	/* get the command */
	ptr = get_slinearg(cmdstring, 1);
	if (!ptr) {
		DO_SYSL("unable to parse command line")
		ToSend(noarticleselected, strlen(noarticleselected), inf);
		return;
	}
	if (strncasecmp(ptr, "from", 4) == 0) {
		hdr_type_str = ",p.author";
	} else if (strncasecmp(ptr, "date", 4) == 0) {
		hdr_type_str = ",p.date";
		/* we must convert the time() returned value stored in the db to a
		 * real standard conform string */
		inf->speccmd = SPECCMD_DATE;
	} else if (strncasecmp(ptr, "newsgroups", 10) == 0) {
		hdr_type_str = ",p.newsgroups";
	} else if (strncasecmp(ptr, "subject", 7) == 0) {
		hdr_type_str = ",p.subject";
	} else if (strncasecmp(ptr, "lines", 5) == 0) {
		hdr_type_str = ",p.lines";
	} else {
		/* not supported */
		hdr_type_str = " "; /* nothing */
	}
	free(ptr);
	
	/* get the min + max values */
	ptr = get_slinearg(cmdstring, 2);
	if(ptr == NULL) { /* no value is given by the client => use current article */
		if (inf->servinf->selected_article == NULL) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		min = (u_int32_t) atoi(inf->servinf->selected_article);
		max = min;
	} else { /* at least a min-value is given */
		/* now check if the client requests either a message numner (num[-[to]]) or a message id (<message-id>) */
		if (ptr[0] == '<') { /* message ID */
			req_message_id = 1;
		} else {
			char *range_ptr = ptr;
			
			req_message_id = 0;
			min = atoi(range_ptr);
			max = min; /* this is correct. if we find a '-', we will change it in a moment */
			/* check for the '-' character */
			while(range_ptr[0]!='\r' && max!=REALmax) {
				if(range_ptr[0]=='-') { /* all articles from min to the end */
					max=REALmax;
				}
				range_ptr++;
			}
			if(max==REALmax) { /* look if there is an end-value for max */
				if(range_ptr[0]!='\r') {
					max=atoi(range_ptr);
					/* if the client sent us bullshit then max could be zero ... */
					if (!max) {
						/* ... in this case, we set it back to REALmax ;-) */
						max = REALmax;
					}
				}
			}
		}
	}
	/*onxxdebugm("%s%i%s%i%s%i%s",
	           "min: ", min, " max: ", max, " REALmax: ", REALmax, "\n");*/
	// check if the requested message ID exists
	if (req_message_id == 1) {
		if (DB_chk_if_msgid_exists(inf->servinf->selected_group, ptr, inf) == 0) {
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
	 
	/* now let the DB do the scheissjob ;-) */
	len = 0xfff + strlen(inf->servinf->selected_group) + strlen(hdr_type_str);
	if (ptr != NULL)
		len += strlen(ptr);
	CALLOC_Thread(sql_cmd, (char *), len, sizeof(char))
	
	if (req_message_id) {
		snprintf(sql_cmd, len - 1,
			"select n.postnum %s"
			" from ngposts n,postings p"
			" where ng='%s' and p.msgid = '%s' and n.msgid = p.msgid;",
			hdr_type_str, inf->servinf->selected_group, ptr);
	} else {
		snprintf(sql_cmd, len - 1,
			"select n.postnum %s"
			" from ngposts n,postings p"
			" where ng='%s' and (postnum between %i and %i) and n.msgid = p.msgid;",
			hdr_type_str, inf->servinf->selected_group, min, max);
	}
	sqlite3_secexec(inf, sql_cmd, docmd_xhdr_cb, inf);

	free(sql_cmd);
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
 *
 * returns:  "%s\t%s\t%s\t%s\t%s\t%s\t%i\t%s\t%s\r\n",
 *		articlenum, subject, author, date, messageid, (references=='\0' ? " " : references),
 *		get_openfilelen(fp), linecount, (xref=='\0' ? " " : xref)
 */

/* argv:       0         1         2         3        4       5        6
 * select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header
 */

static int
docmd_xover_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int articlenum, lines;
	int postlen;
	int len;
	char *ref, *xref, *bodylen;
	char *add_to_response;
	char tbuf[40] = { '\0' };
	
	articlenum = atoi(argv[0]);
	lines = atoi(argv[5]);
	
	/* Calculate the posting size (byte count) */
	bodylen = CDP_return_linevalue(argv[6], "X-WendzelNNTPdBodysize:");
	if (bodylen) {
		postlen = atoi(bodylen);
		free(bodylen);
	}
	postlen += strlen(argv[6]);
	
	ref = CDP_return_linevalue(argv[6], "References:");
	xref = CDP_return_linevalue(argv[6], "Xref:");
	
	len = 0xff + strlen(argv[1]) + strlen(argv[2]) + 39 /* timestamp len */ + strlen(argv[4]);
	if (ref)
		len += strlen(ref);
	if (xref)
		len += strlen(xref);
	
	nntp_localtime_to_str(tbuf, atol(argv[3]));
	
	CALLOC_Thread(add_to_response, (char *), len, sizeof(char))
	snprintf(add_to_response, len - 1,
		"%i\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\r\n",
		articlenum, argv[1], argv[2], tbuf, argv[4], (ref ? ref : " "), postlen, lines, (xref ? xref : " "));
	ToSend(add_to_response, strlen(add_to_response), inf);
	
	if (ref)
		free(ref);
	if (xref)
		free(xref);
	free(add_to_response);
	return 0;
}

static void
docmd_xover(char *cmdstring, server_cb_inf *inf)
{
	u_int32_t min, max;
	u_int32_t REALmax;
	int len;
	char *ptr;
	char *sql_cmd;
	
	/* if (no newsgroup is selected) -> return a 412 error */
	if (inf->servinf->selected_group == NULL) {
		ToSend(nogroupselected, strlen(nogroupselected), inf);
		return;
	}
	
	/* get the REALmax value (max-num in the newsgroups table) */	
	REALmax = DB_get_high_value(inf->servinf->selected_group, inf);
	
	/* get the min + max values */
	ptr=get_slinearg(cmdstring, 1);
	if(ptr==NULL) { /* no value is given by the client => use current article */
		if (inf->servinf->selected_article == NULL) {
			ToSend(noarticleselected, strlen(noarticleselected), inf);
			return;
		}
		min = (u_int32_t) atoi(inf->servinf->selected_article);
		max = min;
	} else { /* at least a min-value is given */
		char *ptr_orig = ptr;
		
		min = atoi(ptr);
		max = min; /* this is correct. if we find a '-', we will change it in a moment */
		/* check for the '-' character */
		while(ptr[0]!='\0' && max!=REALmax) {
			if(ptr[0]=='-') { /* all articles from min to the end */
				max=REALmax;
			}
			ptr++;
		}
		if(max==REALmax) { /* look if there is an end-value for max */
			if(ptr[0]!='\0') {
				max=atoi(ptr);
				/* if the client sent us bullshit then max could be zero ... */
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
	if (min > REALmax || max > REALmax || min > max) {
		ToSend(noarticleselected, strlen(noarticleselected), inf);
		return;
	}

	/* now send the list */
	ToSend(xover, strlen(xover), inf);
	
	/* now let the DB do the scheissjob  ;-) */
	len = 0xfff + strlen(inf->servinf->selected_group);
	CALLOC_Thread(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header"
		" from ngposts n,postings p"
		" where ng='%s' and (postnum between %i and %i) and n.msgid = p.msgid;",
		inf->servinf->selected_group, min, max);
		
	sqlite3_secexec(inf, sql_cmd, docmd_xover_cb, inf);

	free(sql_cmd);
	ToSend(period_end, strlen(period_end), inf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ARTICLE
	before SQL             : line 494 to 812 -> 319 LOC!
	with SQL+improved code : line 496 to 726 -> 231 LOC!
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define CMDTYP_ARTICLE		0x01
#define CMDTYP_HEAD		0x02
#define CMDTYP_BODY		0x03
#define CMDTYP_STAT		0x04

static int
docmd_article_cb_cb2(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;

	/* add header to the send buffer */
	switch (inf->cmdtype) {
	case CMDTYP_ARTICLE:
		ToSend(argv[0], strlen(argv[0]), inf);
		ToSend("\r\n", 2, inf); /* insert empty line */
		break;
	case CMDTYP_HEAD:
		ToSend(argv[0], strlen(argv[0]), inf);
		break;
	}
	/* add missing .\r\n\r\n on HEAD cmd */
	if (inf->cmdtype == CMDTYP_HEAD)
		ToSend(period_end, strlen(period_end), inf);
	
	return 0;
}

static int
docmd_article_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	char *msgid;
	char *id;
	char *sendbuffer;
	char *sql_cmd = NULL;
	int len, len_sql;
	
	if (argc < 3) {
		DO_SYSL("ARTICLE(andCo)_cb: argc < 3!")
		kill_thread(infp);
	}
	msgid = argv[0];
	id = argv[2];
	
	len = 0xff + strlen(msgid) + strlen(id);
	if ((sendbuffer = (char *) calloc(len + 1, sizeof(char))) == NULL) {
		DO_SYSL("ARTICLE(andCo)_cb: not enough memory.")
		kill_thread(infp);
	}

	/* set the selected article */
	if (inf->servinf->selected_article)
		free(inf->servinf->selected_article);
	
	inf->servinf->selected_article = (char *) calloc(strlen(id) + 1, sizeof(char));
	strncpy(inf->servinf->selected_article, id, strlen(id));

	/* compose return message */
//postings (msgid string primary key, date string, author string, newsgroups string, subject string, lines string, header varchar(16000), body varchar(250000));
	len_sql = 0x7f + strlen(msgid);
	if ((sql_cmd = (char *) calloc(len + 1, sizeof(char))) == NULL) {
		DO_SYSL("ARTICLE(andCo)_cb: not enough memory.")
		kill_thread(infp);
	}
	
	switch (inf->cmdtype) {
	case CMDTYP_ARTICLE:
		snprintf(sendbuffer, len, "220 %s %s article retrieved - head and body follow\r\n", id, msgid);
		snprintf(sql_cmd, len_sql, "select header from postings where msgid='%s';", msgid);
		break;
	case CMDTYP_HEAD:
		snprintf(sendbuffer, len, "221 %s %s article retrieved - head follows\r\n", id, msgid);
		snprintf(sql_cmd, len_sql,  "select header from postings where msgid='%s';", msgid);
		break;
	case CMDTYP_BODY:
		snprintf(sendbuffer, len, "222 %s %s article retrieved - body follows\r\n", id, msgid);
		break;
	case CMDTYP_STAT:
		snprintf(sendbuffer, len, "223 %s %s article retrieved - request text separately\r\n", id, msgid);
		break;
	}
	ToSend(sendbuffer, strlen(sendbuffer), inf);
	free(sendbuffer);
	
	/* send the header, if needed */
	if (inf->cmdtype == CMDTYP_ARTICLE || inf->cmdtype == CMDTYP_HEAD) {
		sqlite3_secexec(inf, sql_cmd, docmd_article_cb_cb2, inf);
	}
	if (sql_cmd)
		free(sql_cmd);
	
	/* send the body, if needed */
	if (inf->cmdtype == CMDTYP_ARTICLE || inf->cmdtype == CMDTYP_BODY) {
		char *msgbody = filebackend_retrbody(msgid);
		if (msgbody != NULL) {
			ToSend(msgbody, strlen(msgbody), inf);
			free(msgbody);
		}
	}
	
	inf->servinf->found_article = 1;
	
	return 0;
}


static void
docmd_article(char *cmdstring, server_cb_inf *inf)
{
#define ARTCLTYP_MESSAGEID	0x01
#define ARTCLTYP_NUMBER		0x02
#define ARTCLTYP_CURRENT	0x03  /* > ARTICLE\r\n -> return the currently selected article */
	int type=0;
	char *param;
	int i;
	int found=0;
	char *sql_cmd = NULL;
	int len;
	
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
	switch(type)
	{
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
			
			/* select * from ngposts where ng='%s' and messageid='%s'; */
			len = 0x7f + strlen(inf->servinf->selected_group) + strlen(param);
			if ((sql_cmd = (char *) calloc(len + 1, sizeof(char))) == NULL) {
				DO_SYSL("ARTICLE(andCo): not enough memory.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and msgid='%s'",
				inf->servinf->selected_group, param);
		break;
	
	case ARTCLTYP_NUMBER:
			/* select * from ngposts where ng='%s' and messageid='%s'; */
			len = 0x7f + strlen(inf->servinf->selected_group) + strlen(param);
			if ((sql_cmd = (char *) calloc(len + 1, sizeof(char))) == NULL) {
				DO_SYSL("ARTICLE(andCo): not enough memory.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and postnum='%s'",
				inf->servinf->selected_group, param);
		break;
						
	case ARTCLTYP_CURRENT:
			// erstmal testen, ob nen article selected ist
			if (inf->servinf->selected_article == NULL) {
				ToSend(noarticleselected, strlen(noarticleselected), inf);
				return;
			}
			/* select * from ngposts where ng='%s' and messageid='%s'; */
			len = 0x7f + strlen(inf->servinf->selected_group) + strlen(inf->servinf->selected_article);
			if ((sql_cmd = (char *) calloc(len + 1, sizeof(char))) == NULL) {
				DO_SYSL("ARTICLE(andCo): not enough memory.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and postnum='%s'",
				inf->servinf->selected_group, inf->servinf->selected_article);
		break;
	}

	/* try to find the article and add header+body if found/needed */
	sqlite3_secexec(inf, sql_cmd, docmd_article_cb, inf);
	free(sql_cmd);
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

static int
docmd_group_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int estimated, first, last;
	char *buf;

	last = atoi(argv[3]);
	if (last)
		first = 1;	/* if there's already a posting: the 1st posting has the num 1 */
	else
		first = 0;	/* else, we display a zero */
	estimated = last;
	
	if ((buf = (char *) calloc(strlen(argv[1]) + 0xff, sizeof(char))) == NULL) {
		DO_SYSL("GROUP_cb: not enough memory.")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	snprintf(buf, strlen(argv[1]) + 0xff - 1, "211 %i %i %i %s group selected\r\n",
		estimated, first, last, argv[1]);
	ToSend(buf, strlen(buf), inf);
	free(buf);
	
	if (inf->servinf->selected_group)
		free(inf->servinf->selected_group);
	
	if ((inf->servinf->selected_group = (char *) calloc(strlen(argv[1]) + 1, sizeof(char))) == NULL) {
		DO_SYSL("GROUP_cb: not enough memory.")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	strncpy(inf->servinf->selected_group, argv[1], strlen(argv[1]));
	
	/* unset the selected article */
	inf->servinf->selected_article = NULL;
	/* we found the group */
	inf->servinf->found_group = 1;
	
	return 0;
}

static void
docmd_group(char *cmdstring, server_cb_inf *inf)
{
	char *ptr;
	char *sql_cmd;
	
	ptr = get_slinearg(cmdstring, 1);
	
	if (!ptr) {
		ToSend(no_group_given, strlen(no_group_given), inf);
		return; /* let him try again */
	}
	
	if ((sql_cmd = (char *)calloc(strlen(cmdstring) + 0x7f, sizeof(char))) == NULL) {
		DO_SYSL("GROUP: not enough memory.")
		free(ptr);
		kill_thread(inf);
		/*NOTREACHED*/
	}
	snprintf(sql_cmd, strlen(cmdstring) + 0x7f, "select * from newsgroups where name='%s';", ptr);
	/* the cb func sets found_group to '1' if the group is found */
	inf->servinf->found_group = 0;
	sqlite3_secexec(inf, sql_cmd, docmd_group_cb, inf);
	free(ptr);
	free(sql_cmd);
	/* if the group was not found (=the cb function was not called) ... */
	if (inf->servinf->found_group == 0) {
		/* send "no such group" */
		ToSend(nosuchgroup, strlen(nosuchgroup), inf);
	}
	
	return;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	POST
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define SEND_441ERR(s) {			\
	if(!correctline) {			\
		ToSend(s, strlen(s), inf);	\
		return;				\
	}					\
	correctline=FALSE;			\
}

/* format: <PK> <group> <high> <low> <pflag> */
static int
docmd_post_ng_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;

	if (strcasecmp(inf->servinf->group_chkname, argv[1]) != 0)
		return 0;
	inf->servinf->found_group = 1;
	return 0;
}

/* check if 'val' matches 'rx' */
static int
docmd_post_rxcont(char *rx, char *val)
{
	regex_t preg;

	if (regcomp(&preg, rx, REG_EXTENDED|REG_NEWLINE) == 0)
		if (regexec(&preg, val, 0, NULL, 0) == 0)
			return 0;
	return 1;
}


static void
docmd_post_sql(server_cb_inf *inf)
{
	extern unsigned short anonym_message_ids; /* config.y */
#define MAX_IPLEN 49
	int len=0;
	size_t lenb = 0;
	size_t e;
	int correctline=FALSE;
	int i;
	u_int32_t cur_high;
	size_t s;
	const char sep[]=",";
	char *newid;
	struct hostent *hostinfo;
	char *addr;
	char *from    = NULL,
	     *subj    = NULL,
	     *body    = NULL,
	     *header  = NULL,
	     *add_to_hdr = NULL,
	     *sql_cmd = NULL,
	     *buf     = NULL,
	     *ptr     = NULL;
	char *message_id = NULL;
	char *ngstrp, *ngstrpb;
	char *newsgroup;
	char *sec_cmd;
	char *remove_lines[] = { "Message-ID: ", "Date: ", "Lines: ", NULL };
	char tbuf[40]={'\0'}; /* strftime() */
#ifndef __WIN32__
	char *saveptr, *saveptr2; /* both for strtok_r() */
#endif
	charstack_t *stackp = NULL;
	
	int linecount=0;
	time_t ltime;
	
	Send(inf->sockinf->sockfd, postok, strlen(postok));
	{
		size_t recv_bytes = 0;
		int finished = 0;
		fd_set fds_post;
		int peak = 0;
		struct timeval tv;

		bzero(&tv, sizeof(struct timeval));
		tv.tv_sec = 1;
		
		FD_ZERO(&fds_post);
		FD_SET(inf->sockinf->sockfd, &fds_post);
		peak = inf->sockinf->sockfd;
		
		if ((buf = (char *) calloc(MAX_POSTSIZE, sizeof(char))) == NULL) {
			perror("malloc");
			DO_SYSL("memory low. killing child process.")
			kill_thread(inf);
			/*NOTREACHED*/
		}
		while (!finished) {
			if (select(peak + 1, &fds_post, NULL, NULL, &tv) == -1) {
				if (errno == EINTR) {
					continue;
				} else {
					perror("select() in docmd_post()");
					kill_thread(inf);
					/*NOTREACHED*/
				}
			}
			if (FD_ISSET(inf->sockinf->sockfd, &fds_post)) {
				recv_bytes += recv(inf->sockinf->sockfd, buf+recv_bytes, MAX_POSTSIZE - recv_bytes - 1, 0);
				if ((int)recv_bytes == -1) {
					perror("recv()");
					DO_SYSL("posting recv() error!");
					kill_thread(inf);
					/*NOTREACHED*/
				}
				if (recv_bytes == 0) {
					/* ordinary sending: done */
					finished = 1;
				}
			} else {
				finished = 1;
			}
		}
	}
	
	
	onxxdebugm("%s%s%s", "buffer: '", buf, "'\n");
	
	/* make the posting more secure.
	 * Note: This also escapes quotes in the body that is stored in the filesystem.
	 * This is why database.c/filebackend_savebody() has to remove the doubled quotes
	 * from the msg body!
	 */
	if ((sec_cmd = sqlite3_mprintf("%q", buf)) == NULL) {
		DO_SYSL("sqlite3_mprintf() returned NULL")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	free(buf);
	buf = sec_cmd;
	
	/*
	 * create a buffer called 'header' that only contains the header part of 'buf'
	 */
	
	for(i=0; buf[i+4]!='\0' && header==NULL; i++) {
		if(strncmp(buf+i, "\r\n\r\n", 4)==0) {
			/* the end of the header */
			if((header=(char *)calloc(sizeof(char), i+ 2/*add the \r\n for the regex checks */+ 1/*\0*/))==NULL) {
				perror("calloc");
				DO_SYSL("mem low. killing child process.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			strncpy(header, buf, i+2);
		}
	}
	onxxdebugm("%s%s%s", "header: '", header, "'\n");
	if(!header) {
		DO_SYSL("client sent garbage posting. terminating connection.")
		kill_thread(inf);
		
	}
	body = strstr(buf, "\r\n\r\n") + 4;
	
	/*
	 * different header checks
	 */
	
	/* Newsgroups: */
	if (docmd_post_rxcont("^[nN][eE][wW][sS][gG][rR][oO][uU][pP][sS]: [a-zA-Z0-9.,-]*\r\n", header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_newsgroup)

	/* From: */
	/* "blah@blah.com" */
	if (docmd_post_rxcont("^[fF][rR][oO][mM]: [a-zA-Z0-9.-_+]*@[a-zA-Z0-9.-]*\r\n", header) == 0)
		correctline = TRUE;
	/* blah@blah.com (Name Name) */
	if (docmd_post_rxcont("^[fF][rR][oO][mM]: [a-zA-Z0-9.-_+]*@[a-zA-Z0-9.-]* ([a-zA-Z0-9. -_+]*)\r\n",
				header) == 0)
		correctline=TRUE;
	/* Name [... [Name]] <blah@blah.com> */
	if (docmd_post_rxcont("^[fF][rR][oO][mM]: [^\r\n\t]* <[a-zA-Z0-9.-_+]*@[a-zA-Z0-9.-]*>\r\n",
				header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_from)

	/* Subject: */
	if (docmd_post_rxcont("^[sS][uU][bB][jJ][eE][cC][tT]: .*\r\n", header) == 0)
		correctline=TRUE;
	SEND_441ERR(hdrerror_subject)
	
	/*
	 * Check if the newsgroup-names are valid
	 */
	
	/* get the newsgroups-string value (func is NOT case sens.) */
	if (!(ngstrp = CDP_return_linevalue(header, "Newsgroups:"))) {
		correctline = FALSE;
		SEND_441ERR(hdrerror_newsgroup)
	}
	/* save a copy of this string, because we need it two times */
	ngstrpb = strdup(ngstrp);
	
	if (!(sql_cmd = (char *) calloc(1024, sizeof(char)))) {
		DO_SYSL("Not enough mem for sql_cmd allocation")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	/* get the first newsgroup + check if all newsgroups exists */
	for (newsgroup = 
#ifndef __WIN32__
	strtok_r(ngstrp, sep, &saveptr)
#else
	strtok(ngstrp, sep)
#endif
	; newsgroup; ) {
		snprintf(sql_cmd, 1023, "select * from newsgroups where name = '%s';", newsgroup);
		inf->servinf->group_chkname = newsgroup;
		
		sqlite3_secexec(inf, sql_cmd, docmd_post_ng_cb, inf);
		if (!inf->servinf->found_group) {
			ToSend(nosuchgroup, strlen(nosuchgroup), inf);
			return;
		}
		inf->servinf->found_group = 0; /* reset */
		
		/* get the next group */
#ifndef __WIN32__
		newsgroup = strtok_r(NULL, sep, &saveptr);
#else
		newsgroup = strtok(NULL, sep);
#endif
		bzero(sql_cmd, 1024);
	}
	free(sql_cmd);
	
	/*
	 * Generate a Message ID
	 */
	
	/* get a uniq number */
	if (!(newid = get_uniqnum())) {
		DO_SYSL("I/O error in file DB -> can't create a new msg-id. Terminating child connection.")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	/*printf("new uniq-id: %s\n", newid);*/
	

	/* If the user wants no anonymous message IDs: Find out hostname or use IP */
	if (anonym_message_ids == 0) {
		/* get the host of the sender */
		CALLOC_Thread(addr, (char *), MAX_IPLEN, sizeof(char))
		if (inf->sockinf->family == FAM_4) {
	#ifdef __WIN32__ /* win32 sux */
			free(addr);
			addr = inet_ntoa(inf->sockinf->sa.sin_addr);
			if(!addr)
	#else
			if (!inet_ntop(AF_INET, &inf->sockinf->sa.sin_addr, addr, MAX_IPLEN))
	#endif
			{
	#ifndef __WIN32__
				free(addr);
	#endif
				DO_SYSL("Can't get address of socket! Terminating child connection.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			hostinfo = gethostbyaddr((char *)&inf->sockinf->sa.sin_addr,
				sizeof(inf->sockinf->sa.sin_addr), AF_INET);
	#ifndef __WIN32__
		} else { /* IPnG */
			inet_ntop(AF_INET6, &inf->sockinf->sa6.sin6_addr, addr, MAX_IPLEN);
			if(!addr) {
				DO_SYSL("Can't get address of socket! Terminating child connection.")
				kill_thread(inf);
				/*NOTREACHED*/
			}
			hostinfo = gethostbyaddr((char *)&inf->sockinf->sa6.sin6_addr,
				sizeof(inf->sockinf->sa6.sin6_addr), AF_INET6);
	#endif
		}
	
		if(!hostinfo) {	
			/* Hostname is not present. Use "addr" instead! */
			len=MAX_IDNUM_LEN+3+strlen(addr) + 1;
			CALLOC_Thread(message_id, (char *), len, sizeof(char))
			snprintf(message_id, MAX_IDNUM_LEN+3+strlen(addr), "%s%s>", newid, addr);
		} else {
			len=MAX_IDNUM_LEN+3+strlen(hostinfo->h_name) + 1;
			CALLOC_Thread(message_id, (char *), len, sizeof(char));
			bzero(message_id, len);
			snprintf(message_id, MAX_IDNUM_LEN+3+strlen(addr), "%s%s>", newid, hostinfo->h_name);
			//free(hostinfo);
		}
		free(addr);/* NEW */
	} else {
		/* Create an anonymous Message-ID */
		len=MAX_IDNUM_LEN+3+strlen(NNTPD_ANONYM_HOST) + 1;
		CALLOC_Thread(message_id, (char *), len, sizeof(char))
		snprintf(message_id, len - 1, "%s" NNTPD_ANONYM_HOST ">", newid);
	}
	/* Free the newly created Message-ID since we finaly built the message_id */
	free(newid);
	
	/*
	 * Manipulate the Header (Do not trust the Message-ID, Line-Count and Date
	 * sent by the Client)
	 */
	
	/* if there is a message ID, remove it (new is inserted by the write_posting() func */
	if ((ptr = strstr(buf, "\r\n\r\n")) == 0) {
		DO_SYSL("(internal) parsing error #1")
		kill_thread(inf);
		/*NOTREACHED*/
	}
	len = ptr - buf;
	
	/* Remove 'Message-ID/Date/Lines' */
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
				// now add the rest of the header from position i on
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
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("time(NULL) returned (time_t)-1")
		return;
	}
	
	nntp_localtime_to_str(tbuf, ltime);
	
	/* Get the # of lines */
	if((ptr=strstr(buf, "\r\n\r\n"))==NULL) {
		DO_SYSL("posting aborted (no end of header found, client sent garbage).")
		ToSend(progerr503, strlen(progerr503), inf);
		return;
	}
	
	for(s=4, linecount=-1; s<strlen(ptr); s++) {
		if(ptr[s]=='\n') {
			linecount++;
		} else if(ptr[s]=='.' && s==strlen(ptr)-2) {
			continue;
		}
	}
	
	/* now, with the new values, generate the new part of the header ... */
	if (!(add_to_hdr = (char *) calloc(0x7ff + strlen(message_id) + 2*127 /* 127 chars for
			the hostname + 127 chars for the domainname /should/ be enough */
			+ strlen(header) /* for strcat() */, sizeof(char)))) {
		DO_SYSL("not enough memory.")
		ToSend(progerr503, strlen(progerr503), inf);
		return;
	}
	
	{
		/* Try to set the FQDN here */
		char hostname[128] = { '\0' };
		char domainname[128] = { '\0' };
		char fqdn[256] = { '\0' };
	
		if (gethostname(hostname, 127) == 0) {
			strncpy(fqdn, hostname, strlen(hostname));
		} else {
			strncpy(fqdn, "unknown", strlen("unknown"));
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
			strncpy(fqdn + strlen(fqdn), "unknown", strlen("unknown"));
		}
#endif
		/* Add FQDN + other important headers */
		snprintf(add_to_hdr, 0x7fe,
			"Path: %s\r\nMessage-ID: %s\r\nDate: %s\r\nLines: %i\r\nX-WendzelNNTPdBodysize: %u\r\n",
			fqdn, message_id, tbuf, linecount, (unsigned int)strlen(body));
	}
	
	/* ... and make the header complete */
	strcat(add_to_hdr, header);
	
	/* add the posting to the database */
	
	/* create table postings (msgid string primary key, date string, author string,
				newsgroups string, subject string, lines string,
				header varchar(16000));		*/
	subj = CDP_return_linevalue(header, "Subject:");
	from = CDP_return_linevalue(header, "From:");

	len = strlen(add_to_hdr) + 0xfff + strlen(subj) + strlen(from);
	if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
		DO_SYSL("Not enough mem -- posting aborted.")
		ToSend(progerr503, strlen(progerr503), inf);
		return;
	}

	snprintf(sql_cmd, len - 1,
		"insert into postings (msgid, date, author, newsgroups, subject, "
		"lines, header) values ('%s', '%li', '%s', '%s', '%s', '%i', '%s');",
		message_id, ltime, from, ngstrpb, subj, linecount, add_to_hdr);
	free(ngstrp);
	free(from);
	free(subj);
	free(add_to_hdr);
	
	sqlite3_secexec(inf, sql_cmd, NULL, 0);
	free(sql_cmd);
	
	filebackend_savebody(message_id, body);
	
	/*
	 * and add one entry for every group in ngposts table and increment their
	 * groups 'high' value.
	 */
#ifndef __WIN32__
	newsgroup = strtok_r(ngstrpb, sep, &saveptr2);
#else
	newsgroup = strtok(ngstrpb, sep);
#endif
	
	if (!(stackp = (charstack_t *) calloc(1, sizeof(charstack_t)))) {
		DO_SYSL("Not enough mem -- posting aborted.")
		ToSend(progerr503, strlen(progerr503), inf);
		return;
	}
	stackp->state = STACK_EMPTY;
	
	while (newsgroup != NULL) {
		cur_high = DB_get_high_value(newsgroup, inf);
		if (cur_high > 0x7ffffffe) { /* :-) */
			DO_SYSL("Warning: This server only can handle 0x7fffffff (= 2.147.483.646) articles!")
			DO_SYSL("Warning: Article limit exeeded. This can lead to big problems!")
			ToSend(progerr503, strlen(progerr503), inf);
			return;
		}
		
		/* update high value */
		len = strlen(newsgroup) + 0x7f;
		if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
			DO_SYSL("Not enough mem -- posting aborted.")
			ToSend(progerr503, strlen(progerr503), inf);
			return;
		}
		snprintf(sql_cmd, len - 1, "update newsgroups set high='%i' where name='%s';",
			cur_high + 1, newsgroup);
		sqlite3_secexec(inf, sql_cmd, NULL, 0);
		free(sql_cmd);

		/* add the posting in ngposts (msgid string primary key, ng string, postnum integer); */
		len = 0xff + strlen(newsgroup) + strlen(message_id) + 20;
		if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
			DO_SYSL("Not enough mem -- posting aborted.")
			ToSend(progerr503, strlen(progerr503), inf);
			return;
		}
		// Only do INSERT, if this 'newsgroup' name occours the first time in this posting
		// since a 'Newsgroups: my.group,my.group' (a double post) would lead to a doubled
		// msgid in the table, what sqlite3 will not accept since this is the primary key!
		
		if (charstack_check_for(stackp, newsgroup) == STACK_NOTFOUND) {
			snprintf(sql_cmd, len - 1,
				"insert into ngposts (msgid, ng, postnum) values ('%s', '%s', '%i');",
				message_id, newsgroup, cur_high + 1);
		
			sqlite3_secexec(inf, sql_cmd, NULL, 0);
			if (charstack_push_on(stackp, newsgroup) != OK_RETURN) {
				DO_SYSL("charstack_push_on() returned an error. This can lead to a closed "
					"client connection if one newsgroup was selected multiple times "
					"for a single posting (what shouldn't be the normal case)")
			}
		} else {
			DO_SYSL("Catched a douple newsgroup posting (posted it only once to the group).")
		}
		free(sql_cmd);
		
		/* write XML output, if the user wants that! */
		if (use_xml_output)
			write_xml();
		
#ifndef __WIN32__
		newsgroup = strtok_r(NULL, sep, &saveptr2);
#else
		newsgroup = strtok(NULL, sep);
#endif
	}
	
	charstack_free(stackp);
	free(ngstrpb);
	
	ToSend(postdone, strlen(postdone), inf);
	free(message_id);
	/* free the main buffer that includes all these... these...
	 * these things!! HAR: THEY ARE IKNZ FANS!!!
	 */
	sqlite3_free(sec_cmd);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	COMMAND HANDLER
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static void
do_command(char *recvbuf, server_cb_inf *inf)
{

#define QUESTION(cmd, len)	strncasecmp(recvbuf, cmd, len)==0
#define QUESTION_AUTH(cmd, len)	(inf->servinf->auth_is_there && strncasecmp(recvbuf, cmd, len)==0)
	
	/* COMMANDS THAT NEED _NO_ AUTHENTICATION */
	/* zunaechst Abfangen von moegl. doppelten cmds! */
	if (QUESTION("quit", 4)) {
		Send(inf->sockinf->sockfd, quitstring, strlen(quitstring));
		kill_thread(inf);
		/*NOTREACHED*/
	} else if (QUESTION("authinfo user ", 14)) {
		docmd_authinfo_user(recvbuf, inf);
	} else if (QUESTION("authinfo pass ", 14)) {
		docmd_authinfo_pass(recvbuf, inf);
	}
	
	/* COMMANDS THAT NEED AUTHENTICATION */
	/* zunaechst Abfangen von moegl. doppelten cmds! */
	
	else if (QUESTION_AUTH("list newsgroups\r\n", 17)) {
		ToSend(unknown_cmd, strlen(unknown_cmd), inf);
	} else if (QUESTION_AUTH("help", 4)) {
		ToSend(helpstring, strlen(helpstring), inf);
	} else if (QUESTION_AUTH("list", 4)) {
		docmd_list_sql(inf);
	} else if (QUESTION_AUTH("group", 5)) {
		docmd_group(recvbuf, inf);
	} else if (QUESTION_AUTH("xover", 5)) {
		docmd_xover(recvbuf, inf);
	} else if (QUESTION_AUTH("xhdr", 4)) {
		docmd_xhdr(recvbuf, inf);
	} else if (QUESTION_AUTH("article", 7) || QUESTION_AUTH("head", 4)
		|| QUESTION_AUTH("body", 4) || QUESTION_AUTH("stat", 4)) {
		docmd_article(recvbuf, inf);
	} else if (QUESTION_AUTH("mode reader", 11)) {
		ToSend(mode_reader_ok, strlen(mode_reader_ok), inf);
	} else if (QUESTION_AUTH("post", 4)) {
		docmd_post_sql(inf);
	} else if (QUESTION_AUTH("date", 4)) {
		docmd_date(inf);
	} else {
		if(inf->servinf->auth_is_there) {
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
	
	/* This is _SERV_-inf */
	bzero(&servinf, sizeof(serverinfo_t));
	/* This is _SOCK_-inf */
	sockinf = (sockinfo_t *)socket_info_ptr;
	/* This is 'inf' containing everything */
	inf.sockinf = sockinf;
	inf.servinf = &servinf;
	
	if (sqlite3_open(DBFILE, &servinf.db)) {
		sqlite3_close(servinf.db);
		ToSend(progerr503, strlen(progerr503), &inf);
		DO_SYSL("Unable to open database.")
		/* kill this thread */
		kill_thread(&inf);
	}

	Send(sockinf->sockfd, welcomestring, strlen(welcomestring));
	
	if(use_auth==1) {
		servinf.auth_is_there=0;
	} else {
		servinf.auth_is_there=1;
	}
	
	while(1) {
		if (len == 0)
			bzero(recvbuf, MAX_CMDLEN);
		if (recv(sockinf->sockfd, recvbuf+len, MAX_CMDLEN-len, 0) <= 0)
			/* kill connection if the client sends more bytes than allowed */
			kill_thread(&inf);
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
			if ((sec_cmd = sqlite3_mprintf("%q", recvbuf)) == NULL) {
				DO_SYSL("sqlite3_mprintf() returned NULL")
				kill_thread(&inf);
			}
			
			/* now proceed */
			do_command(sec_cmd, &inf);
			sqlite3_free(sec_cmd);
					
			Send(sockinf->sockfd, servinf.curstring, strlen(servinf.curstring));
			free(servinf.curstring);
			servinf.curstring=NULL;
			len=0;
		} else {
			len = strlen(recvbuf);
		}
	}
	/* NOTREACHED */
	kill_thread(&inf);
	return NULL;
}

void
kill_thread(server_cb_inf *inf)
{
	char *conn_logstr = NULL;

	/* TODO:  if there's still something in the send() queue, then run Send() */
	
	
	/* Log the ended connection */
	conn_logstr = str_concat("Closed connection from ", inf->sockinf->ip, NULL, NULL, NULL);
	if (conn_logstr) {
		DO_SYSL(conn_logstr)
		free(conn_logstr);
	}
	
	/* close db connection */
	sqlite3_close(inf->servinf->db);
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

