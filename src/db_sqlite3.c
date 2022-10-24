/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2009-2021 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

/* SQlite3 Database Interface */

#include "main.h"
#include "cdpstrings.h"

extern unsigned short use_auth;	/* config.y */
extern unsigned short use_acl; /* config.y */
extern short global_mode; /* global.c */

extern char progerr503[];
extern char period_end[];

void
db_sqlite3_open_connection(server_cb_inf *inf)
{
	if (sqlite3_open(DBFILE, &(inf->servinf->db))) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(inf->servinf->db));
		sqlite3_close(inf->servinf->db);
		if (global_mode == MODE_THREAD) {
			ToSend(progerr503, strlen(progerr503), inf);
			DO_SYSL("Unable to open database.")
			/* kill this thread */
			kill_thread(inf);
			/* NOTREACHED */
		} else {
			exit(1);
		}
	}
}

void
db_sqlite3_close_connection(server_cb_inf *inf)
{
	sqlite3_close(inf->servinf->db);
}

/* This was a good idea but actually it does nothing more than executing sqlite3_exec. I do
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
		if (global_mode == MODE_THREAD) {
			ToSend(progerr503, strlen(progerr503), inf);
			DO_SYSL("sqlite3_SECexec(): Unable to exec on database.")
			kill_thread(inf);
			/* NOTREACHED */
		} else {
			exit(ERR_EXIT);
		}
	}
}

/* ***** AUTHINFO PASS ***** */

static int
db_sqlite3_authinfo_checkpass_cb(void *infp, int argc, char **argv, char **ColName)
{
	assert(global_mode == MODE_THREAD);
        ((server_cb_inf *)infp)->servinf->auth_is_there = 1;
	return 0;
}

void
db_sqlite3_authinfo_check(server_cb_inf *inf)
{

	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_THREAD);
	/* now check if combination of user+pass is valid */
	len = strlen(inf->servinf->cur_auth_user) + strlen(inf->servinf->cur_auth_pass) + 0xff;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from users where name='%s' and password='%s';",
		inf->servinf->cur_auth_user, inf->servinf->cur_auth_pass);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_authinfo_checkpass_cb, inf);
	free(sql_cmd);
}

/* ***** LIST ***** */

/* format: <group> <high> <low> <pflag> */
static int
db_sqlite3_list_cb(void *infp, int argc, char **argv, char **ColName)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	char addbuf[1024] = { '\0' };
	/* the first message is '1' if we already have an article, else it will be zero */
	int already_posted = atoi(argv[3]);
	
	if (global_mode == MODE_THREAD && use_auth && use_acl) {
		if (db_sqlite3_acl_check_user_group(inf, inf->servinf->cur_auth_user, argv[1]) == FALSE) {
			/* Newsgroup invisible for this user */
			return 0;
		}
	}
	
	/* 1. If the newsgroup does NOT match the wildmat sent by the user, then we
	 *    do not have to return it.
	 * 2. Also the format differs to the normal list format!
	 */
	switch (inf->speccmd) {
	case CMDTYP_XGTITLE:
	case CMDTYP_LIST_NEWSGROUPS:
		if (wnntpd_rx_contain(inf->servinf->wildmat, argv[1]) != 0) {
#ifdef DUBUG
			fprintf(stderr, "Skipping ng %s for wildmat %s in LIST NEWSGROUPS or XGTITLE\n",
					argv[1], wildmat);
#endif
			return 0; /* doesn't match wildmat */
		}
		/* our description is always empty ;-) */
		snprintf(addbuf, 1023, "%s -\r\n", argv[1]);
		break;
	default:
		snprintf(addbuf, 1023, "%s %s %i %s\r\n", argv[1], argv[3],
			(already_posted ? 1 : 0), argv[2]);
		break;
	}
	
	if (global_mode == MODE_THREAD)
		ToSend(addbuf, strlen(addbuf), inf);
	else
		printf("%s", addbuf);
	return 0;
}

void
db_sqlite3_list(server_cb_inf *inf, int cmdtyp, char *wildmat)
{
	char *sql_cmd = "select * from Newsgroups;";

	if (global_mode == MODE_PROCESS) {
		printf("Newsgroup, Low-, High-Value, Posting-Flag\n");
		printf("-----------------------------------------\n");
	}
	/* Sqlite parameter pushing start */
	inf->servinf->wildmat = wildmat;
	inf->speccmd = cmdtyp;
	/* Sqlite parameter pushing end */
	
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_list_cb, inf);

	/* Sqlite parameter clearing start */
	inf->servinf->wildmat = NULL;
	inf->speccmd = 0;
	/* Sqlite parameter clearing end */
}

/* ***** XHDR ***** */

/* argv:       0         1
 * select n.postnum, p.<hdrpart>
 */

static int
db_sqlite3_xhdr_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int len;
	int articlenum;
	char *add_to_response;
	static char none[] = "(none)";
	char *value;
	char tbuf[40] = { '\0' };
	server_cb_inf *linf;

	assert(global_mode == MODE_THREAD);
	
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
	
	CALLOC_Thread(infp, add_to_response, (char *), len, sizeof(char))
	snprintf(add_to_response, len - 1, "%i %s\r\n", articlenum, value);
	ToSend(add_to_response, strlen(add_to_response), inf);
	
	free(add_to_response);
	
	return 0;
}

void
db_sqlite3_xhdr(server_cb_inf *inf, short message_id_flg, int xhdr, char *article, u_int32_t min,
		u_int32_t max)
{
	int len;
	char *hdr_type_str;
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	switch (xhdr) {
	case XHDR_FROM:
		hdr_type_str = ",p.author";
		break;
	case XHDR_DATE:
		hdr_type_str = ",p.date";
		/* we must convert the time() returned value stored in the db to a
		 * real standard conform string */
		inf->speccmd = SPECCMD_DATE;
		break;
	case XHDR_NEWSGROUPS:
		hdr_type_str = ",p.newsgroups";
		break;
	case XHDR_SUBJECT:
		hdr_type_str = ",p.subject";
		break;
	case XHDR_LINES:
		hdr_type_str = ",p.lines";
		break;
	default:
		fprintf(stderr, "Internal Exception. No valid XHDR field!\n");
		DO_SYSL("Internal Exception. No valid XHDR field!")
		return;
	}
	
	len = 0xfff + strlen(inf->servinf->selected_group) + strlen(hdr_type_str);
	if (article != NULL)
		len += strlen(article);
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	
	if (message_id_flg == 1) {
		snprintf(sql_cmd, len - 1,
			"select n.postnum %s"
			" from ngposts n,postings p"
			" where ng='%s' and p.msgid = '%s' and n.msgid = p.msgid;",
			hdr_type_str, inf->servinf->selected_group, article);
	} else {
		snprintf(sql_cmd, len - 1,
			"select n.postnum %s"
			" from ngposts n,postings p"
			" where ng='%s' and (postnum between %i and %i) and n.msgid = p.msgid;",
			hdr_type_str, inf->servinf->selected_group, min, max);
	}
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_xhdr_cb, inf);
	free(sql_cmd);
	/* Sqlite parameter clearing start */
	inf->speccmd = 0;
	/* Sqlite parameter clearing end */
}

/* ***** ARTICLE ***** */

static int
db_sqlite3_article_cb_cb2(void *infp, int argc, char **argv, char **col)
{
	assert(global_mode == MODE_THREAD);
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
db_sqlite3_article_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	char *msgid;
	char *id;
	char *sendbuffer;
	char *sql_cmd = NULL;
	int len, len_sql;
	
	assert(global_mode == MODE_THREAD);
	
	if (argc < 3) {
		DO_SYSL("ARTICLE(andCo)_cb: argc < 3!")
		kill_thread(infp);
		/* NOTREACHED */
	}
	msgid = argv[0];
	id = argv[2];
	
	len = 0xff + strlen(msgid) + strlen(id);
	CALLOC_Thread(infp, sendbuffer, (char *), len + 1, sizeof(char))

	/* set the selected article */
	if (inf->servinf->selected_article)
		free(inf->servinf->selected_article);
	
	/* Do not use CALLOC_Thread() here since there is a free() inside! */
	if (!(inf->servinf->selected_article = (char *) calloc(strlen(id) + 1, sizeof(char)))) {
		free(sendbuffer);
		DO_SYSL("Not enough memory")
		kill_thread(inf);
		/* NOTREACHED */
	}
	strncpy(inf->servinf->selected_article, id, strlen(id));

	/* compose return message */
//postings (msgid string primary key, date string, author string, newsgroups string, subject string, lines string, header varchar(16000), body varchar(250000));
	len_sql = 0x7f + strlen(msgid);
	
	/* Do not use CALLOC_Thread() here since there is a free() inside! */
	if (!(sql_cmd = (char *) calloc(len + 1, sizeof(char)))) {
		free(sendbuffer);
		free(inf->servinf->selected_article);
		inf->servinf->selected_article = NULL;
		DO_SYSL("Not enough memory")
		kill_thread(inf);
		/* NOTREACHED */
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
		sqlite3_secexec(inf, sql_cmd, db_sqlite3_article_cb_cb2, inf);
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

void
db_sqlite3_article(server_cb_inf *inf, int type, char *param)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	switch(type) {
	case ARTCLTYP_MESSAGEID:
		/* select * from ngposts where ng='%s' and messageid='%s'; */
		len = 0x7f + strlen(inf->servinf->selected_group) + strlen(param);
		CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
		snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and msgid='%s'",
			inf->servinf->selected_group, param);
		break;
	
	case ARTCLTYP_NUMBER:
		/* select * from ngposts where ng='%s' and messageid='%s'; */
		len = 0x7f + strlen(inf->servinf->selected_group) + strlen(param);
		CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
		snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and postnum='%s'",
			inf->servinf->selected_group, param);
		break;
	
	case ARTCLTYP_CURRENT:
		/* select * from ngposts where ng='%s' and messageid='%s'; */
		len = 0x7f + strlen(inf->servinf->selected_group) + strlen(inf->servinf->selected_article);
		CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
		snprintf(sql_cmd, len, "select * from ngposts where ng='%s' and postnum='%s'",
			inf->servinf->selected_group, inf->servinf->selected_article);
		break;
	}

	/* try to find the article and add header+body if found/needed */
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_article_cb, inf);
	free(sql_cmd);
}

/* ***** GROUP ***** */

static int
db_sqlite3_group_cbset_first_article_in_group_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;

	/* set the new selected article */
	/* This make sure that we do not get a crashdump result due to argv[0]==NULL set by
	 * Sqlite because of an empty newsgroup (see comment above the SQL statement in the
	 * function that calls this callback function to understand it completely!).
	 * libc background: strdup() calls strlen(). If argv[0] is NULL, then strlen() kills
	 * us, what is OUR fault because we would be lame.
	 */
	if (argv[0]) {
		inf->servinf->selected_article = strdup(argv[0]);
		if (!inf->servinf->selected_article) {
			DO_SYSL("strdup() error");
			fprintf(stderr, "strdup() error!");
			kill_thread(inf);
			/* NOTREACHED */
		}
	}
	return 0;
}

static void
db_sqlite3_group_cb_set_first_article_in_group(server_cb_inf *inf)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_THREAD);
	
	len = 0x7f + strlen(inf->servinf->selected_group);

	CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
	/* This query prevents an argv[0]==NULL value in the callback function if
	 * there is no result. Without that subselect, Sqlite3 would return
	 * argv[0]==NULL if there is an empty result set, what means that
	 * min(postnum) does not really exist, what leads to a crashdump
	 * in the callback function.
	 * I added an additional check for argv[0] in the callback function just
	 * to make sure the server runs 100% stable!
	 */
	snprintf(sql_cmd, len, "SELECT postnum FROM ngposts WHERE postnum = "
		"(select min(postnum) as minimaleee from ngposts where ng='%s');",
		inf->servinf->selected_group);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_group_cbset_first_article_in_group_cb, inf);
	free(sql_cmd);
}

static int
db_sqlite3_group_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int estimated, first, last;
	char *buf;
	
	assert(global_mode == MODE_THREAD);

	last = atoi(argv[3]);
	if (last)
		first = 1;	/* if there's already a posting: the 1st posting has the num 1 */
	else
		first = 0;	/* else, we display a zero */
	estimated = last;
	
	CALLOC_Thread(infp, buf, (char *), strlen(argv[1]) + 0xff, sizeof(char))
	snprintf(buf, strlen(argv[1]) + 0xff - 1, "211 %i %i %i %s group selected\r\n",
		estimated, first, last, argv[1]);
	ToSend(buf, strlen(buf), inf);
	free(buf);
	
	if (inf->servinf->selected_group)
		free(inf->servinf->selected_group);
	
	CALLOC_Thread(infp, inf->servinf->selected_group, (char *), strlen(argv[1]) + 1, sizeof(char))
	strncpy(inf->servinf->selected_group, argv[1], strlen(argv[1]));
	
	/* unset the selected article */
	if (inf->servinf->selected_article)
		free(inf->servinf->selected_article);
	inf->servinf->selected_article = NULL;
	
	/* Since the above 'first' value is just FAKE, we need to find out the correct one */
	db_sqlite3_group_cb_set_first_article_in_group(inf);
	
	/* we found the group */
	inf->servinf->found_group = 1;
	
	return 0;
}

void
db_sqlite3_group(server_cb_inf *inf, char *group)
{
	char *sql_cmd;
	int old_foundgroup = inf->servinf->found_group;
	
	assert(global_mode == MODE_THREAD);
	
	/* Do not use CALLOC_Thread() here since there is a free() inside! */
	if ((sql_cmd = (char *) calloc(strlen(group) + 0x7f, sizeof(char))) == NULL) {
		DO_SYSL("GROUP: not enough memory.")
		free(group);
		kill_thread(inf);
		/* NOTREACHED */
	}
	snprintf(sql_cmd, strlen(group) + 0x7f,
		"select * from newsgroups where name='%s';", group);
	/* the cb func sets found_group to '1' if the group is found */
	inf->servinf->found_group = 0;
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_group_cb, inf);
	
	/* restore old settings, if needed */
	if (inf->servinf->found_group == 0)
		inf->servinf->found_group = old_foundgroup;

	free(sql_cmd);
}

/* ***** LISTGROUP ***** */

static int
db_sqlite3_listgroup_cb(void *infp, int argc, char **argv, char **col)
{
	char *buf;
	server_cb_inf *inf = (server_cb_inf *) infp;
	/* len: why not +2 (\r\n\0)? could not find the bug up to now ... :-( */
	int len = strlen(argv[0]) + 3;
	
	CALLOC_Thread(inf, buf, (char *), len + 1, sizeof(char))
	snprintf(buf, len, "%s\r\n", argv[0]);
	ToSend(buf, strlen(buf), inf);
	
	/* set the new article pointer to the first article in the group
	 * (RFC need!) */
	if (inf->servinf->found_article == 0) {
		if (inf->servinf->selected_article)
			free(inf->servinf->selected_article);
		inf->servinf->selected_article = strdup(argv[0]);
		if (!inf->servinf->selected_article) {
			DO_SYSL("strdup() error!")
			fprintf(stderr, "strdup() error!");
			kill_thread(inf);
			/* NOTREACHED */
		}
		inf->servinf->counter++;
	}
	free(buf);
	return 0;
}

void
db_sqlite3_listgroup(server_cb_inf *inf, char *group)
{
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	/* Do not use CALLOC_Thread() here since there is a free() inside! */
	if ((sql_cmd = (char *) calloc(strlen(group) + 0x7f, sizeof(char))) == NULL) {
		DO_SYSL("GROUP: not enough memory.")
		free(group);
		kill_thread(inf);
		/* NOTREACHED */
	}
	snprintf(sql_cmd, strlen(group) + 0x7f,
		"select postnum from ngposts where `ng`='%s' order by `postnum` asc;", group);
	inf->servinf->counter = 0; /* incremented, if cb-func is called with article */
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_listgroup_cb, inf);
	free(sql_cmd);
	
	/* If there are NO articles in the group: Select NO article.
	 * (RFC need!) */
	if (inf->servinf->counter == 0) {
		inf->servinf->selected_article = NULL;
	}
	
	/* Send trailing '.' */
	ToSend(".\r\n", 3, inf);
}


/* ***** XOVER ***** */

 /* returns:  "%s\t%s\t%s\t%s\t%s\t%s\t%i\t%s\t%s\r\n",
 *		articlenum, subject, author, date, messageid, (references=='\0' ? " " : references),
 *		get_openfilelen(fp), linecount, (xref=='\0' ? " " : xref)
 */

/* argv:       0         1         2         3        4       5        6
 * select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header
 */

static int
db_sqlite3_xover_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;
	int articlenum, lines;
	int postlen = 0;
	int len;
	char *ref, *xref, *bodylen;
	char *add_to_response;
	char tbuf[40] = { '\0' };
	
	assert(global_mode == MODE_THREAD);
	
	articlenum = atoi(argv[0]);
	lines = atoi(argv[5]);
	
	/* Calculate the posting size (byte count) */
	bodylen = CDP_return_linevalue(argv[6], "X-WendzelNNTPdBodysize:");
	if (bodylen) {
		postlen = atoi(bodylen);
		free(bodylen);
	} else {
		/* this should not be reachable as the minimum body size
		 * is 3 (.\r\n). */
		bodylen = 0;
	}
	postlen += strlen(argv[6]);
	
	ref = CDP_return_linevalue(argv[6], "References:");
	xref = CDP_return_linevalue(argv[6], "Xref:");
	
	len = 0xff + strlen(argv[1]) + strlen(argv[2]) + 39 /* timestamp len */
		+ strlen(argv[4]);
	if (ref)
		len += strlen(ref);
	if (xref)
		len += strlen(xref);
	
	nntp_localtime_to_str(tbuf, atol(argv[3]));
	
	CALLOC_Thread(infp, add_to_response, (char *), len, sizeof(char))
	
	snprintf(add_to_response, len - 1,
		"%i\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\r\n",
		articlenum, argv[1], argv[2], tbuf, argv[4], (ref ? ref : " "),
		postlen, lines, (xref ? xref : " "));
	ToSend(add_to_response, strlen(add_to_response), inf);
	
	if (ref)
		free(ref);
	if (xref)
		free(xref);
	free(add_to_response);
	return 0;
}

void
db_sqlite3_xover(server_cb_inf *inf, u_int32_t min, u_int32_t max)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	len = 0xfff + strlen(inf->servinf->selected_group);
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header"
		" from ngposts n,postings p"
		" where ng='%s' and (postnum between %i and %i) and n.msgid = p.msgid;",
		inf->servinf->selected_group, min, max);
		
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_xover_cb, inf);
	free(sql_cmd);
}

/* ***** POST FUNCTIONS, ALSO (PARTIALLY) USABLE BY OTHER PARTS ***** */

/* NOTES:
 * 1. This function is used by multiple sqlite3 functions as
 *    a callback function. Do not make changes here without a good
 *    reason.
 * 2. This callback function is used by both: the server and the
 *    admin tool.
 * 3. The variable inf->servinf->found_group is used not only for
 *    newsgroups, but also for users and roles (and maybe even more).
 *    The name of the variable might change in the future to some more
 *    reasonable name.
 */
static int
db_sqlite3_post_ng_cb(void *infp, int argc, char **argv, char **col)
{
	server_cb_inf *inf = (server_cb_inf *) infp;

	/* I think, this check is not really needed since the function
	 * is only called if a group with the given name exists. This
	 * means that setting found_group = 1 should be the only task
	 * here ... */
//	if (strcasecmp(inf->servinf->chkname, argv[1]) != 0)
//		return 0;
	inf->servinf->found_group = 1; /* or: user, or: $something! */
	return 0;
}

static int
db_sqlite3_get_high_value_cb(void *cur_high, int argc, char **argv, char **col)
{
	*((int *)cur_high) = atol(argv[3]);
	return 0;
}

u_int32_t
db_sqlite3_get_high_value(server_cb_inf *inf, char *newsgroup)
{
	int len;
	char *sql_cmd;
	u_int32_t cur_high;
	
	/* get high value */
	len = strlen(newsgroup) + 0x7f;
	if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
		DO_SYSL("Not enough mem")
		ToSend(progerr503, strlen(progerr503), inf);
		kill_thread(inf);
		/* NOTREACHED */
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name = '%s';",
		newsgroup);
	
	if (sqlite3_exec(inf->servinf->db, sql_cmd, db_sqlite3_get_high_value_cb,
			&cur_high, &inf->servinf->sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", inf->servinf->sqlite_err_msg);
		sqlite3_free(inf->servinf->sqlite_err_msg);
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("Unable to exec on database.")
		kill_thread(inf);
		/* NOTREACHED */
	}
	free(sql_cmd);
	return cur_high;
}

static int
DB_chk_if_msgid_exists_cb(void *boolval, int argc, char **argv, char **col)
{
	*((int *)boolval) = 1;
	return 0;
}

int
db_sqlite3_chk_if_msgid_exists(server_cb_inf *inf, char *newsgroup, char *msgid)
{
	int len;
	char *sql_cmd;
	int boolval = 0;
	
	/* get high value */
	len = strlen(newsgroup) + strlen(msgid) + 0x7f;
	if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
		DO_SYSL("Not enough mem")
		ToSend(progerr503, strlen(progerr503), inf);
		kill_thread(inf);
		/* NOTREACHED */
	}
	snprintf(sql_cmd, len - 1, "select * from ngposts where ng = '%s' and msgid = '%s';",
		newsgroup, msgid);
	
	if (sqlite3_exec(inf->servinf->db, sql_cmd, DB_chk_if_msgid_exists_cb, &boolval,
			&inf->servinf->sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", inf->servinf->sqlite_err_msg);
		sqlite3_free(inf->servinf->sqlite_err_msg);
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("Unable to check if msgid exists")
		kill_thread(inf);
		/* NOTREACHED */
	}
	free(sql_cmd);
	return boolval;
}

void
db_sqlite3_chk_newsgroup_posting_allowed(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	
	/* first: reset */
	inf->servinf->found_group = 0;
	
	len = 0xfff + strlen(inf->servinf->chkname);
	if (global_mode == MODE_THREAD) {
		CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	} else {
		CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name='%s' and pflag='y';",
		inf->servinf->chkname);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_post_ng_cb, inf);
	free(sql_cmd);
}


void
db_sqlite3_chk_newsgroup_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	
	/* NOTE: This function is also used by the admin tool! */

	/* first: reset */
	inf->servinf->found_group = 0;
	
	len = 0xfff + strlen(inf->servinf->chkname);
	if (global_mode == MODE_THREAD) {
		CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	} else {
		CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name='%s';",
		inf->servinf->chkname);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_post_ng_cb, inf);
	free(sql_cmd);
}

void
db_sqlite3_chk_user_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_PROCESS);

	len = 0xfff + strlen(inf->servinf->chkname);
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from users where name='%s';",
		inf->servinf->chkname);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_post_ng_cb, inf);
	free(sql_cmd);
}

void
db_sqlite3_chk_role_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_PROCESS);

	len = 0xfff + strlen(inf->servinf->chkname);
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from roles where role = '%s';",
		inf->servinf->chkname);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_post_ng_cb, inf);
	free(sql_cmd);
}

void
db_sqlite3_post_insert_into_postings(server_cb_inf *inf, char *message_id,
	time_t ltime, char *from, char *ngstrpb, char *subj,
	int linecount, char *add_to_hdr)
{
	int len;
	char *sql_cmd;
	
	/* create table postings (msgid string primary key, date string, author string,
	 * newsgroups string, subject string, lines string, header varchar(16000));
	 */
	
	len = strlen(add_to_hdr) + 0xfff + strlen(subj) + strlen(from);
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	
	snprintf(sql_cmd, len - 1,
		"insert into postings (msgid, date, author, newsgroups, subject, "
		"lines, header) values ('%s', '%li', '%s', '%s', '%s', '%i', '%s');",
		message_id, ltime, from, ngstrpb, subj, linecount, add_to_hdr);
	
	sqlite3_secexec(inf, sql_cmd, NULL, 0);
	free(sql_cmd);
}

void
db_sqlite3_post_update_high_value(server_cb_inf *inf, u_int32_t new_high, char *newsgroup)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	len = strlen(newsgroup) + 0x7f;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "update newsgroups set high='%u' where name='%s';",
		new_high, newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, 0);
	free(sql_cmd);
}

void
db_sqlite3_post_insert_into_ngposts(server_cb_inf *inf, char *message_id, char *newsgroup,
	u_int32_t new_high)
{
	int len;
	char *sql_cmd;
	
	assert(global_mode == MODE_THREAD);
	
	/* add the posting in ngposts (msgid string primary key, ng string, postnum integer); */
	len = 0xff + strlen(newsgroup) + strlen(message_id) + 20;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"insert into ngposts (msgid, ng, postnum) values ('%s', '%s', '%i');",
		message_id, newsgroup, new_high);
	sqlite3_secexec(inf, sql_cmd, NULL, 0);
	free(sql_cmd);
}

/* ***** ACL (not administration part of ACL) ****** */

/* check if a user 'user' has access to newsgroup 'newsgroup' */
static int
db_sqlite3_acl_check_user_group_cb(void *allowed, int argc, char **argv, char **ColName)
{
	/* If this function was called, a set of one row must exists
	 * that matches (user,group). This means it is allowed for
	 * the given user to access the given newsgroup.
	 */
	assert(global_mode == MODE_THREAD);
	*((short *)allowed) = TRUE;
	return 0;
}

short
db_sqlite3_acl_check_user_group(server_cb_inf *inf, char *user, char *newsgroup)
{
	int len;
	char *sql_cmd;
	short allowed = FALSE;

	assert(global_mode == MODE_THREAD);
	
	len = strlen(user) + strlen(newsgroup) + 0xff;
	CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))

	/* 1st try: check, if the user has direct access to the group using
	 * NON-role ACL */
	snprintf(sql_cmd, len, "SELECT * FROM acl_users WHERE username='%s' AND ng='%s'\n",
		user, newsgroup);
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_acl_check_user_group_cb, &allowed);
	
	/* 2nd try: check, if the user has access to the group via an ACL
	 * role he is member in */
	if (allowed == FALSE) {
		snprintf(sql_cmd, len,
			"SELECT * FROM users2roles ur,acl_roles aclr WHERE "
			"ur.username='%s' AND ur.role=aclr.role AND aclr.ng='%s';",
			user, newsgroup);
		sqlite3_secexec(inf, sql_cmd, db_sqlite3_acl_check_user_group_cb, &allowed);
	}
	
	free(sql_cmd);
	return allowed;
}

/* ****** ADMINISTRATION ***** */

/* These functions are used by WendzelNNTPadm what means that global_mode must be
 * MODE_PROCESS!
 */

static int
db_sqlite3_list_users_cb(void *unused, int argc, char **argv, char **ColName)
{
	printf("%s, %s\n", argv[0], argv[1]);
	return 0;
}

void
db_sqlite3_list_users(server_cb_inf *inf)
{
	char sql_cmd[] = "select * from users;";

	assert(global_mode == MODE_PROCESS);
	
	printf("Username, Password\n");
	printf("------------------\n");
	sqlite3_secexec(inf, sql_cmd, db_sqlite3_list_users_cb, NULL);
}

static int
db_sqlite3_list_acl_tables_cb(void *unused, int argc, char **argv, char **ColName)
{
	printf("%s, %s\n", argv[0], argv[1]);
	return 0;
}

static int
db_sqlite3_list_acl_tables_cb_roles(void *unused, int argc, char **argv, char **ColName)
{
	printf("%s\n", argv[0]);
	return 0;
}

void
db_sqlite3_list_acl_tables(server_cb_inf *inf)
{
	assert(global_mode == MODE_PROCESS);

	printf("List of roles in database:\n");
	printf("Roles\n");
	printf("-----\n");
	sqlite3_secexec(inf, "select role from roles", db_sqlite3_list_acl_tables_cb_roles, NULL);
	
	putchar('\n');
	printf("Connections between users and roles:\n"
		"Role, User\n"
		"----------\n");
	sqlite3_secexec(inf, "select role,username from users2roles order by role asc;", db_sqlite3_list_acl_tables_cb, NULL);

	putchar('\n');
	printf("Username, Has access to group\n");
	printf("-----------------------------\n");
	sqlite3_secexec(inf, "select username,ng from acl_users;", db_sqlite3_list_acl_tables_cb, NULL);

	putchar('\n');
	printf("Role, Has access to group\n");
	printf("-------------------------\n");
	sqlite3_secexec(inf, "select role,ng from acl_roles;", db_sqlite3_list_acl_tables_cb, NULL);
}

void
db_sqlite3_acl_add_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(username) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"insert into acl_users (username, ng) values ('%s', '%s');",
		username, newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
}

void
db_sqlite3_acl_del_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(username) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"delete from acl_users where username='%s' and ng='%s';",
		username, newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
}

void
db_sqlite3_acl_add_role(server_cb_inf *inf, char *role)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "insert into roles (role) values ('%s');", role);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
}

void
db_sqlite3_acl_del_role(server_cb_inf *inf, char *role)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);

	/* First remove the associated data sets */
	/* pt. 1: users */
	printf("Removing associations of role %s with their users ... ", role);
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "delete from users2roles where role='%s';", role);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
	
	/* pt. 2: newsgroups */
	printf("Removing associations of role %s with their newsgroups ... ", role);
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "delete from acl_roles where role='%s';", role);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");

	
	/* Now remove the role */
	printf("Removing role %s ... ", role);
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "delete from roles where role='%s';", role);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
}

void
db_sqlite3_acl_role_connect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	printf("Connecting role %s with newsgroup %s ... ", role, newsgroup);
	len = strlen(role) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"INSERT INTO acl_roles (role, ng) VALUES ('%s', '%s');", role,
		newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
}

void
db_sqlite3_acl_role_disconnect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	printf("Dis-Connecting role %s from newsgroup %s ... ", role, newsgroup);
	len = strlen(role) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"DELETE FROM acl_roles WHERE role='%s' AND ng='%s';", role, newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
}

void
db_sqlite3_acl_role_connect_user(server_cb_inf *inf, char *role, char *user)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	printf("Connecting role %s with user %s ... ", role, user);
	len = strlen(role) + strlen(user) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"INSERT INTO users2roles (username, role) VALUES ('%s', '%s');",
		user, role);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
}

void
db_sqlite3_acl_role_disconnect_user(server_cb_inf *inf, char *role, char *user)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	printf("Dis-Connecting role %s from user %s ... ", role, user);
	len = strlen(role) + strlen(user) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"DELETE FROM users2roles WHERE role='%s' AND username='%s';", role, user);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
	printf("done\n");
}

void
db_sqlite3_create_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	
	snprintf(sql_cmd, len - 1,
		"insert into newsgroups (name, pflag, high) values ('%s', '%c', '0');",
		newsgroup, post_flg);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);
}

void
db_sqlite3_delete_newsgroup(server_cb_inf *inf, char *newsgroup)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);

	len = strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))

	/* 1nd: Delete all posting-to-newsgroup entries from the association class
	 * ngposts which belong to %newsgroup */
	printf("Clearing association class ... ");
	snprintf(sql_cmd, len - 1, "delete from ngposts where ngposts.ng='%s';", newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	/* 2nd: Delete ACL associations of the newsgroup */
	printf("Clearing ACL associations of newsgroup %s... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from acl_users where ng='%s';", newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	/* 3rd: Delete ACL role associations of the newsgroup */
	printf("Clearing ACL role associations of newsgroup %s... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from acl_roles where ng='%s';", newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	/* 4st: Delete the Newsgroup itself */
	printf("Deleting newsgroup %s itself ... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from newsgroups where name = '%s';", newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
		
	/* 5rd: Delete all remaining postings which do not belong to any existing
	 * newsgroup. */
	printf("Cleanup: Deleting postings that do not belong to an existing newsgroup ... ");
	snprintf(sql_cmd, len - 1,
		"DELETE FROM postings WHERE NOT EXISTS (select * from ngposts "
		"where postings.msgid = ngposts.msgid);");
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	free(sql_cmd);
}

void
db_sqlite3_modify_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	
	snprintf(sql_cmd, len - 1,
		"update newsgroups set pflag = '%c' where name = '%s';", post_flg,
		newsgroup);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	free(sql_cmd);	
}

void
db_sqlite3_add_user(server_cb_inf *inf, char *username, char *password)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(username) + strlen(password) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	
	snprintf(sql_cmd, len - 1,
		"insert into users (name, password) values ('%s', '%s');",
		username, password);
	
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	
	/* Make sure we left no password in the memory */
	bzero(sql_cmd, strlen(sql_cmd));
	free(sql_cmd);
}

void
db_sqlite3_del_user(server_cb_inf *inf, char *username)
{
	char *sql_cmd;
	int len;
	
	assert(global_mode == MODE_PROCESS);
	
	len = strlen(username) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	
	/* 1. delete the acl associations of the user */
	printf("Clearing ACL associations of user %s... ", username);
	snprintf(sql_cmd, len - 1, "delete from acl_users where username='%s';", username);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	/* 2. delete the role associations of the user */
	printf("Clearing ACL role associations of user %s... ", username);
	snprintf(sql_cmd, len - 1, "delete from users2roles where username='%s';", username);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");
	
	/* 3. delete the user from table "users" */
	printf("Deleting user %s from database ... ", username);
	snprintf(sql_cmd, len - 1, "delete from users where name='%s';", username);
	sqlite3_secexec(inf, sql_cmd, NULL, NULL);
	printf("done\n");

	free(sql_cmd);
}



