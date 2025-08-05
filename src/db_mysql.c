/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2012 Steffen Wendzel <steffen (at) wendzel (dot) de>
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

/* MySQL Database Interface */

#include "main.h"
#include "cdpstrings.h"

#define MYSQL_CHK_ERR(__wn_syslog_msg)				\
   {								\
	if (mysql_errno(inf->servinf->myhndl) != 0) {		\
		fprintf(stderr, "MySQL error message: %s\n",	\
			mysql_error(inf->servinf->myhndl));	\
		fprintf(stderr, __wn_syslog_msg);		\
		DO_SYSL(__wn_syslog_msg)			\
		if (global_mode == MODE_THREAD) {		\
			kill_thread(inf);			\
		} else {					\
			exit(1);				\
		}						\
								\
   	}							\
   }



extern unsigned short use_auth;	/* config.y */
extern unsigned short use_acl; /* config.y */
extern short global_mode; /* global.c */
extern short message_body_in_db;  /* config.y */

extern char progerr503[];
extern char period_end[];

void
db_mysql_open_connection(server_cb_inf *inf)
{
	extern char *db_server; /* config.l */
	extern char *db_user; /* config.l */
	extern char *db_pass; /* config.l */
	extern short db_port;

	if ((inf->servinf->myhndl = mysql_init(NULL)) == NULL) {
		DO_SYSL("MySQL Init Error! Unable to access database. Closing client connection.")
		fprintf(stderr,
			"MySQL Init Error! Unable to access database. Closing client connection.");
		if (global_mode == MODE_THREAD) {
			kill_thread(inf);
			/* NOTREACHED */
		} else {
			exit(1);
		}
	}
	if (mysql_real_connect(inf->servinf->myhndl, db_server, db_user, db_pass,
		"WendzelNNTPd", db_port, NULL, 0) == NULL) {
		fprintf(stderr, "mysql_real_connect() error: %s\n",
			mysql_error(inf->servinf->myhndl));
		if (global_mode == MODE_THREAD) {
			DO_SYSL("mysql_real_connect error. Check your database configuration. "
				"Closing client connection. "
				"Start the server in non-daemon mode to read the mysql_error() on stdout")
			kill_thread(inf);
			/* NOTREACHED */
		} else {
			exit(1);
		}
	}
}

void
db_mysql_close_connection(server_cb_inf *inf)
{
	mysql_close(inf->servinf->myhndl);
}

/* ***** AUTHINFO PASS ***** */

void
db_mysql_authinfo_check(server_cb_inf *inf)
{
	MYSQL_ROW  row;
	MYSQL_RES  *res;
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_THREAD);
	/* now check if combination of user+pass is valid */
	len = strlen(inf->servinf->cur_auth_user) + strlen(inf->servinf->cur_auth_pass) + 0xff;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from users where name='%s' and password='%s';",
		inf->servinf->cur_auth_user, inf->servinf->cur_auth_pass);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	if ((row = mysql_fetch_row(res)) != NULL) {
		inf->servinf->auth_is_there = 1;
	}
	mysql_free_result(res);
	free(sql_cmd);
}

/* ***** LIST ***** */

/* format: <group> <high> <low> <pflag> */

void
db_mysql_list(server_cb_inf *inf, int cmdtyp, char *wildmat)
{
	char *sql_cmd = "select * from newsgroups;";
	MYSQL_ROW  row;
	MYSQL_RES  *res;
	char addbuf[1024] = { '\0' };
	/* the first message is '1' if we already have an article, else it will be zero */
	int already_posted;

	if (global_mode == MODE_PROCESS) {
		printf("Newsgroup, Low-, High-Value, Posting-Flag\n");
		printf("-----------------------------------------\n");
	}

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		already_posted = atoi(row[3]);
		if (global_mode == MODE_THREAD && use_auth && use_acl) {
			if (db_mysql_acl_check_user_group(inf, inf->servinf->cur_auth_user, row[1]) == FALSE) {
				/* Newsgroup invisible for this user */
				continue;
			}
		}

		/* 1. If the newsgroup does NOT match the wildmat sent by the user, then we
		 *    do not have to return it.
		 * 2. Also the format differs to the normal list format!
		 */
		switch (cmdtyp) {
		case CMDTYP_XGTITLE:
		case CMDTYP_LIST_NEWSGROUPS:
			if (wnntpd_rx_contain(wildmat, row[1]) != 0) {
#ifdef DEBUG
				fprintf(stderr, "Skipping ng %s for wildmat %s in LIST NEWSGROUPS or XGTITLE\n",
						row[1], wildmat);
#endif
				continue; /* doesn't match wildmat */
			}
			/* our description is always empty ;-) */
			snprintf(addbuf, 1023, "%s -\r\n", row[1]);
			break;
		default:
			snprintf(addbuf, 1023, "%s %s %i %s\r\n", row[1], row[3],
				(already_posted ? 1 : 0), row[2]);
			break;
		}

		if (global_mode == MODE_THREAD)
			ToSend(addbuf, strlen(addbuf), inf);
		else
			printf("%s", addbuf);
	}
	mysql_free_result(res);
}


void
db_mysql_post_insert_into_postings(server_cb_inf *inf, char *message_id,
	time_t ltime, char *from, char *ngstrpb, char *subj,
	int linecount, char *add_to_hdr)
{
	int len;
	char *sql_cmd;

	/* CREATE TABLE postings (`msgid` VARCHAR(196),`date` INTEGER,`author` VARCHAR(196),
	 *    `newsgroups` VARCHAR(2048),`subject` VARCHAR(2048),`lines` VARCHAR(10),
	 *    `header` VARCHAR(20000),INDEX(`msgid`),PRIMARY KEY(`msgid`)) ENGINE=INNODB; */
	len = strlen(add_to_hdr) + 0xfff + strlen(subj) + strlen(from);
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"insert into postings (`msgid`, `date`, `author`, `newsgroups`, `subject`, "
		"`lines`, `header`) values ('%s', '%li', '%s', '%s', '%s', '%i', '%s');",
		message_id, ltime, from, ngstrpb, subj, linecount, add_to_hdr);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

void
db_mysql_post_update_high_value(server_cb_inf *inf, u_int32_t new_high, char *newsgroup)
{
	int len;
	char *sql_cmd;

	assert(global_mode == MODE_THREAD);

	len = strlen(newsgroup) + 0x7f;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "update newsgroups set high='%u' where name='%s';",
		new_high, newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

void
db_mysql_post_insert_into_ngposts(server_cb_inf *inf, char *message_id, char *newsgroup,
	u_int32_t new_high)
{
	int len;
	char *sql_cmd;

	assert(global_mode == MODE_THREAD);

	/* add the posting in ngposts:
	 * `msgid` VARCHAR(196),
	 * `ng` VARCHAR(196),
	 * `postnum` INTEGER
	 * Plus: Key stuff */
	len = 0xff + strlen(newsgroup) + strlen(message_id) + 20;
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"insert into ngposts (msgid, ng, postnum) values ('%s', '%s', '%i');",
		message_id, newsgroup, new_high);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

u_int32_t
db_mysql_get_high_value(server_cb_inf *inf, char *newsgroup)
{
	int len;
	char *sql_cmd;
	u_int32_t cur_high;
	MYSQL_RES *res;
	MYSQL_ROW row;

	/* get high value */
	len = strlen(newsgroup) + 0x7f;
	if (!(sql_cmd = (char *) calloc(len, sizeof(char)))) {
		DO_SYSL("Not enough mem")
		ToSend(progerr503, strlen(progerr503), inf);
		kill_thread(inf);
		/* NOTREACHED */
	}
	snprintf(sql_cmd, len - 1, "select `high` from newsgroups where name = '%s';",
		newsgroup);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if ((row = mysql_fetch_row(res)) != NULL) {
		cur_high = atol(row[0]);
	} else {
		cur_high = 0;
		DO_SYSL("Internal Error (mysql_fetch_row(res) in db_mysql.c/db_mysql_get_high_value()")
	}
	free(sql_cmd);
	mysql_free_result(res);
	return cur_high;
}

int
db_mysql_chk_if_msgid_exists(server_cb_inf *inf, char *newsgroup, char *msgid)
{
	int len;
	char *sql_cmd;
	int boolval = 0;
	MYSQL_RES *res;

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

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if (mysql_num_rows(res) != 0) {
		boolval = 1;
	}
	free(sql_cmd);
	mysql_free_result(res);
	return boolval;
}

void
db_mysql_chk_newsgroup_posting_allowed(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	MYSQL_RES *res;

	inf->servinf->found_group = 0;

	len = 0xfff + strlen(inf->servinf->chkname);
	if (global_mode == MODE_THREAD) {
		CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	} else {
		CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name='%s' and pflag='y';",
		inf->servinf->chkname);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if (mysql_num_rows(res) != 0) {
		inf->servinf->found_group = 1;
	}
	free(sql_cmd);
	mysql_free_result(res);
}

void
db_mysql_chk_newsgroup_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	MYSQL_RES *res;

	/* NOTE: This function is also used by the admin tool! */

	inf->servinf->found_group = 0;

	len = 0xfff + strlen(inf->servinf->chkname);
	if (global_mode == MODE_THREAD) {
		CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	} else {
		CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name='%s';",
		inf->servinf->chkname);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if (mysql_num_rows(res) != 0) {
		inf->servinf->found_group = 1;
	}
	free(sql_cmd);
	mysql_free_result(res);
}


/* ***** XHDR ***** */

/* argv:       0         1
 * select n.postnum, p.<hdrpart>
 */

void
db_mysql_xhdr(server_cb_inf *inf, short message_id_flg, int xhdr, char *article, u_int32_t min,
		u_int32_t max)
{
	int len;
	char *hdr_type_str;
	char *sql_cmd;
	int articlenum;
	char *add_to_response;
	static char none[] = "(none)";
	char *value;
	char tbuf[40] = { '\0' };
	MYSQL_ROW row;
	MYSQL_RES *res;

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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}

	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		articlenum = atoi(row[0]);

		/* If the requested header-part is supported, we get its value in argv[1],
		 * if not, we get NULL since we did not request a send parameter in the
		 * SQL query.
		 */
		if (row[1] != NULL)
			value = row[1];
		else
			value = none;

		/* catch date values we need to convert! */
		if (inf->speccmd == SPECCMD_DATE) {
			nntp_localtime_to_str(tbuf, atol(row[1]));
			value = tbuf;
		}

		len = 0xfff + 1 + strlen(value) + 2 + 1;

		CALLOC_Thread(inf, add_to_response, (char *), len, sizeof(char))
		snprintf(add_to_response, len - 1, "%i %s\r\n", articlenum, value);
		ToSend(add_to_response, strlen(add_to_response), inf);
		free(add_to_response);
	}
	mysql_free_result(res);
	free(sql_cmd);
	/* Sqlite parameter clearing start */
	inf->speccmd = 0;
	/* Sqlite parameter clearing end */
}

/* ***** ARTICLE ***** */

static void
db_mysql_article_send_header_cb(server_cb_inf *inf, char *msgid)
{
	char *sql_cmd;
	int len;
	MYSQL_ROW row;
	MYSQL_RES *res;

	assert(global_mode == MODE_THREAD);

	len = 0x7f + strlen(msgid);
	CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
	snprintf(sql_cmd, len,  "select header from postings where msgid='%s';", msgid);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);

	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	if ((row = mysql_fetch_row(res)) == NULL) {
		MYSQL_CHK_ERR("Unable to load message header from database")
		return;
	}

	/* add header to the send buffer */
	switch (inf->cmdtype) {
	case CMDTYP_ARTICLE:
		ToSend(row[0], strlen(row[0]), inf);
		ToSend("\r\n", 2, inf); /* insert empty line */
		break;
	case CMDTYP_HEAD:
		ToSend(row[0], strlen(row[0]), inf);
		break;
	}
	/* add missing .\r\n\r\n on HEAD cmd */
	if (inf->cmdtype == CMDTYP_HEAD)
		ToSend(period_end, strlen(period_end), inf);

	/* don't free(sql_cmd) here since already done */
	mysql_free_result(res);
}

void
db_mysql_article(server_cb_inf *inf,
	int type /* msgid, number or current; NOT the cmd type (ARTICLE, HEAD, ...)! */,
	char *param)
{
	int len;
	char *sql_cmd = NULL;
	char *msgid;
	char *id;
	char *sendbuffer;
	MYSQL_ROW row;
	MYSQL_RES *res;

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
	assert(sql_cmd != NULL);

	/* try to find the article and add header+body if found/needed */
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}

	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	if ((row = mysql_fetch_row(res)) != NULL) {

		msgid = row[0];
		id = row[2];

		len = 0xff + strlen(msgid) + strlen(id);
		CALLOC_Thread(inf, sendbuffer, (char *), len + 1, sizeof(char))

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
		switch (inf->cmdtype) {
		case CMDTYP_ARTICLE:
			snprintf(sendbuffer, len, "220 %s %s article retrieved - head and body follow\r\n",
				id, msgid);
			break;
		case CMDTYP_HEAD:
			snprintf(sendbuffer, len, "221 %s %s article retrieved - head follows\r\n", id, msgid);
			break;
		case CMDTYP_BODY:
			snprintf(sendbuffer, len, "222 %s %s article retrieved - body follows\r\n", id, msgid);
			break;
		case CMDTYP_STAT:
			snprintf(sendbuffer, len, "223 %s %s article retrieved - request text separately\r\n",
				id, msgid);
			break;
		}
		ToSend(sendbuffer, strlen(sendbuffer), inf);
		free(sendbuffer);

		/* send the header, if needed */
		if (inf->cmdtype == CMDTYP_ARTICLE || inf->cmdtype == CMDTYP_HEAD) {
			db_mysql_article_send_header_cb(inf, msgid);
		}

		/* send the body, if needed */
		if (inf->cmdtype == CMDTYP_ARTICLE || inf->cmdtype == CMDTYP_BODY) {
			char *msgbody;
			if (message_body_in_db) {
				msgbody = db_load_message_body(inf, msgid);
			} else {
				msgbody = filebackend_retrbody(msgid);
			}
			if (msgbody != NULL) {
				ToSend(msgbody, strlen(msgbody), inf);
				free(msgbody);
			}
		}

		inf->servinf->found_article = 1;
	}
	mysql_free_result(res);
	free(sql_cmd);
}


/* ***** GROUP ***** */

static void
db_mysql_group_cb_set_first_article_in_group(server_cb_inf *inf)
{
	char *sql_cmd;
	int len;
	MYSQL_ROW row;
	MYSQL_RES *res;

	assert(global_mode == MODE_THREAD);

	/* This var will get changed, if an article is available */
	inf->servinf->selected_article = NULL;

	len = 0x7f + strlen(inf->servinf->selected_group);

	CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))
	snprintf(sql_cmd, len, "select min(postnum) from ngposts where ng='%s';",
		inf->servinf->selected_group);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);

	/* use_result, not store_result() here! */
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	if ((row = mysql_fetch_row(res)) != NULL && row[0] != NULL) {
		/* set the new selected article */
		inf->servinf->selected_article = strdup(row[0]);
	}
}

void
db_mysql_group(server_cb_inf *inf, char *group)
{
	char *sql_cmd;
	int estimated, first, last;
	char *buf;
	MYSQL_ROW row;
	MYSQL_RES *res;
	int old_foundgroup = inf->servinf->found_group;

	assert(global_mode == MODE_THREAD);

	/* not use CALLOC_Thread() here since there is a free() inside */
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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);

	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	if ((row = mysql_fetch_row(res)) != NULL) {
		last = atoi(row[3]);
		if (last)
			first = 1;	/* if there's already a posting: the 1st posting has the num 1 */
		else
			first = 0;	/* else, we display a zero */
		estimated = last;
		CALLOC_Thread(inf, buf, (char *), strlen(row[1]) + 0xff, sizeof(char))
		snprintf(buf, strlen(row[1]) + 0xff - 1, "211 %i %i %i %s group selected\r\n",
			estimated, first, last, row[1]);
		ToSend(buf, strlen(buf), inf);
		free(buf);

		if (inf->servinf->selected_group)
			free(inf->servinf->selected_group);
		CALLOC_Thread(inf, inf->servinf->selected_group, (char *), strlen(row[1]) + 1, sizeof(char))
		strncpy(inf->servinf->selected_group, row[1], strlen(row[1]));

		/* unset the selected article */
		inf->servinf->selected_article = NULL;
		/* we found the group */
		inf->servinf->found_group = 1;
		/* Since the above 'first' value is just FAKE, we need to find out the correct one */
		db_mysql_group_cb_set_first_article_in_group(inf);
	}
	mysql_free_result(res);

	/* restore old settings, if needed */
	if (inf->servinf->found_group == 0)
		inf->servinf->found_group = old_foundgroup;
}

/* ***** LISTGROUP ***** */

void
db_mysql_listgroup(server_cb_inf *inf, char *group)
{
	char *sql_cmd;
	char *buf;
	MYSQL_ROW row;
	MYSQL_RES *res;
	int article_counter = 0;

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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		/* len: why not +2 (\r\n\0)? could not find the bug up to now ... :-( */
		int len = strlen(row[0]) + 3;

		CALLOC_Thread(inf, buf, (char *), len + 1, sizeof(char))
		snprintf(buf, len, "%s\r\n", row[0]);
		ToSend(buf, strlen(buf), inf);

		/* set the new article pointer to the first article in the group
		 * (RFC need!) */
		if (article_counter == 0) {
			if (inf->servinf->selected_article)
				free(inf->servinf->selected_article);
			inf->servinf->selected_article = strdup(row[0]);
			if (!inf->servinf->selected_article) {
				DO_SYSL("strdup() error!")
				fprintf(stderr, "strdup() error!");
				kill_thread(inf);
				/* NOTREACHED */
			}
			article_counter++;
		}
		free(buf);
	}

	/* If there are NO articles in the group: Select NO article.
	 * (RFC need!) */
	if (article_counter == 0) {
		inf->servinf->selected_article = NULL;
	}

	/* Send trailing '.' */
	ToSend(".\r\n", 3, inf);
	mysql_free_result(res);
}


/* ***** XOVER ***** */

 /* returns:  "%s\t%s\t%s\t%s\t%s\t%s\t%i\t%s\t%s\r\n",
 *		articlenum, subject, author, date, messageid, (references=='\0' ? " " : references),
 *		get_openfilelen(fp), linecount, (xref=='\0' ? " " : xref)
 */

/* argv:       0         1         2         3        4       5        6
 * select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header
 */

void
db_mysql_xover(server_cb_inf *inf, u_int32_t min, u_int32_t max)
{
	int len;
	char *sql_cmd;
	int articlenum, lines;
	int postlen = 0;
	char *ref, *xref, *bodylen;
	char *add_to_response;
	char tbuf[40] = { '\0' };
	MYSQL_ROW row;
	MYSQL_RES *res;

	assert(global_mode == MODE_THREAD);

	len = 0xfff + strlen(inf->servinf->selected_group);
	CALLOC_Thread(inf, sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header"
		" from ngposts n,postings p"
		" where ng='%s' and (postnum between %i and %i) and n.msgid = p.msgid;",
		inf->servinf->selected_group, min, max);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}

	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		articlenum = atoi(row[0]);
		lines = atoi(row[5]);

		/* Calculate the posting size (byte count) */
		bodylen = CDP_return_linevalue(row[6], "X-WendzelNNTPdBodysize:");
		if (bodylen) {
			postlen = atoi(bodylen);
			free(bodylen);
		}
		postlen += strlen(row[6]);

		ref = CDP_return_linevalue(row[6], "References:");
		xref = CDP_return_linevalue(row[6], "Xref:");

		len = 0xff + strlen(row[1]) + strlen(row[2]) + 39 /* timestamp len */
			+ strlen(row[4]);
		if (ref)
			len += strlen(ref);
		if (xref)
			len += strlen(xref);

		nntp_localtime_to_str(tbuf, atol(row[3]));

		CALLOC_Thread(inf, add_to_response, (char *), len, sizeof(char))

		snprintf(add_to_response, len - 1,
			"%i\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\r\n",
			articlenum, row[1], row[2], tbuf, row[4], (ref ? ref : " "),
			postlen, lines, (xref ? xref : " "));
		ToSend(add_to_response, strlen(add_to_response), inf);

		if (ref)
			free(ref);
		if (xref)
			free(xref);
		free(add_to_response);
	}
	mysql_free_result(res);
	free(sql_cmd);
}

/************* ACL **************/

short
db_mysql_acl_check_user_group(server_cb_inf *inf, char *user, char *newsgroup)
{
	int len;
	char *sql_cmd;
	short allowed = FALSE;
	MYSQL_RES *res;

	assert(global_mode == MODE_THREAD);

	len = strlen(user) + strlen(newsgroup) + 0xff;
	CALLOC_Thread(inf, sql_cmd, (char *), len + 1, sizeof(char))

	/* 1st try: check, if the user has direct access to the group using
	 * NON-role ACL */
	snprintf(sql_cmd, len, "SELECT * FROM acl_users WHERE username='%s' AND ng='%s';",
		user, newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}
	if (mysql_num_rows(res) != 0) {
		allowed = TRUE;
	}
	mysql_free_result(res);

	/* 2nd try: check, if the user has access to the group via an ACL
	 * role he is member in */
	if (allowed == FALSE) {
		snprintf(sql_cmd, len,
			"SELECT * FROM users2roles ur,acl_roles aclr WHERE "
			"ur.username='%s' AND ur.role=aclr.role AND aclr.ng='%s';",
			user, newsgroup);
		if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
			MYSQL_CHK_ERR("mysql_real_query() error")
		}
		if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
			MYSQL_CHK_ERR("mysql_store_result error")
		}
		if (mysql_num_rows(res) != 0) {
			allowed = TRUE;
		}
		mysql_free_result(res);
	}

	free(sql_cmd);
	return allowed;
}

/* ****** ADMINISTRATION ***** */

void
db_mysql_create_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"insert into newsgroups (name, pflag, high) values ('%s', '%c', '0');",
		newsgroup, post_flg);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

void
db_mysql_delete_newsgroup(server_cb_inf *inf, char *newsgroup)
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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 2nd: Delete ACL associations of the newsgroup */
	printf("Clearing ACL associations of newsgroup %s... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from acl_users where ng='%s';", newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 3rd: Delete ACL role associations of the newsgroup */
	printf("Clearing ACL role associations of newsgroup %s... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from acl_roles where ng='%s';", newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 4st: Delete the Newsgroup itself */
	printf("Deleting newsgroup %s itself ... ", newsgroup);
	snprintf(sql_cmd, len - 1, "delete from newsgroups where name = '%s';", newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 5rd: Delete all remaining postings which do not belong to any existing
	 * newsgroup. */
	printf("Cleanup: Deleting postings that do not belong to an existing newsgroup ... ");
	snprintf(sql_cmd, len - 1,
		"DELETE FROM postings WHERE NOT EXISTS (select * from ngposts "
		"where postings.msgid = ngposts.msgid);");
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	free(sql_cmd);
}

void
db_mysql_modify_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"update newsgroups set pflag = '%c' where name = '%s';", post_flg,
		newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

/* These functions are used by WendzelNNTPadm what means that global_mode must be
 * MODE_PROCESS!
 */

void
db_mysql_list_users(server_cb_inf *inf)
{
	char sql_cmd[] = "select * from users;";
	MYSQL_ROW row;
	MYSQL_RES *res;

	assert(global_mode == MODE_PROCESS);

	printf("Username, Password\n");
	printf("------------------\n");

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		printf("%s, %s\n", row[0], row[1]);
	}
	mysql_free_result(res);
}


void
db_mysql_chk_user_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	MYSQL_RES  *res;

	assert(global_mode == MODE_PROCESS);

	len = 0xfff + strlen(inf->servinf->chkname);
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from users where name='%s';",
		inf->servinf->chkname);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if (mysql_num_rows(res) != 0) {
		inf->servinf->found_group = 1;
	}
	free(sql_cmd);
	mysql_free_result(res);
}

void
db_mysql_chk_role_existence(server_cb_inf *inf)
{
	int len;
	char *sql_cmd;
	MYSQL_RES *res;

	assert(global_mode == MODE_PROCESS);

	len = 0xfff + strlen(inf->servinf->chkname);
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "select * from roles where role = '%s';",
		inf->servinf->chkname);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}

	if ((res = mysql_store_result(inf->servinf->myhndl)) == NULL) {
		MYSQL_CHK_ERR("mysql_store_result error")
	}

	if (mysql_num_rows(res) != 0) {
		inf->servinf->found_group = 1;
	}
	free(sql_cmd);
	mysql_free_result(res);
}

void
db_mysql_acl_add_role(server_cb_inf *inf, char *role)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "insert into roles (role) values ('%s');", role);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
}

void
db_mysql_acl_del_role(server_cb_inf *inf, char *role)
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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");

	/* pt. 2: newsgroups */
	printf("Removing associations of role %s with their newsgroups ... ", role);
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "delete from acl_roles where role='%s';", role);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");


	/* Now remove the role */
	printf("Removing role %s ... ", role);
	len = strlen(role) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1, "delete from roles where role='%s';", role);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");
}

void
db_mysql_acl_role_connect_group(server_cb_inf *inf, char *role, char *newsgroup)
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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");
}

void
db_mysql_acl_role_disconnect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	printf("Dis-Connecting role %s from newsgroup %s ... ", role, newsgroup);
	len = strlen(role) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"DELETE FROM acl_roles WHERE role='%s' AND ng='%s';", role, newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");
}

void
db_mysql_acl_role_connect_user(server_cb_inf *inf, char *role, char *user)
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
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");
}

void
db_mysql_acl_role_disconnect_user(server_cb_inf *inf, char *role, char *user)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	printf("Dis-Connecting role %s from user %s ... ", role, user);
	len = strlen(role) + strlen(user) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"DELETE FROM users2roles WHERE role='%s' AND username='%s';", role, user);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query() error")
	}
	free(sql_cmd);
	printf("done\n");
}

static void
db_mysql_list_acl_tables_cb(MYSQL_ROW row)
{
	printf("%s, %s\n", row[0], row[1]);
	return;
}


void
db_mysql_list_acl_tables(server_cb_inf *inf)
{
	MYSQL_ROW row;
	MYSQL_RES *res;
	char *sql_cmd;

	assert(global_mode == MODE_PROCESS);

	printf("List of roles in database:\n");
	printf("Roles\n");
	printf("-----\n");
	sql_cmd = "select role from roles";
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		printf("%s\n", row[0]);
	}
	mysql_free_result(res);

	putchar('\n');
	printf("Connections between users and roles:\n"
		"Role, User\n"
		"----------\n");
	sql_cmd = "select role,username from users2roles order by role asc;";
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		db_mysql_list_acl_tables_cb(row);
	}
	mysql_free_result(res);

	putchar('\n');
	printf("Username, Has access to group\n");
	printf("-----------------------------\n");
	sql_cmd = "select username,ng from acl_users;";
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		db_mysql_list_acl_tables_cb(row);
	}
	mysql_free_result(res);

	putchar('\n');
	printf("Role, Has access to group\n");
	printf("-------------------------\n");
	sql_cmd = "select role,ng from acl_roles;";
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	res = mysql_store_result(inf->servinf->myhndl);
	MYSQL_CHK_ERR("mysql_store_result")

	while ((row = mysql_fetch_row(res)) != NULL) {
		db_mysql_list_acl_tables_cb(row);
	}
	mysql_free_result(res);
}


void
db_mysql_add_user(server_cb_inf *inf, char *username, char *password)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(username) + strlen(password) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))

	snprintf(sql_cmd, len - 1,
		"insert into users (name, password) values ('%s', '%s');",
		username, password);

	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}

	/* Make sure we left no password in the memory */
	bzero(sql_cmd, strlen(sql_cmd));
	free(sql_cmd);
}

void
db_mysql_del_user(server_cb_inf *inf, char *username)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(username) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))

	/* 1. delete the acl associations of the user */
	printf("Clearing ACL associations of user %s... ", username);
	snprintf(sql_cmd, len - 1, "delete from acl_users where username='%s';", username);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 2. delete the role associations of the user */
	printf("Clearing ACL role associations of user %s... ", username);
	snprintf(sql_cmd, len - 1, "delete from users2roles where username='%s';", username);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	/* 3. delete the user from table "users" */
	printf("Deleting user %s from database ... ", username);
	snprintf(sql_cmd, len - 1, "delete from users where name='%s';", username);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	printf("done\n");

	free(sql_cmd);
}


void
db_mysql_acl_add_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(username) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"insert into acl_users (username, ng) values ('%s', '%s');",
		username, newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}

void
db_mysql_acl_del_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd;
	int len;

	assert(global_mode == MODE_PROCESS);

	len = strlen(username) + strlen(newsgroup) + 0x7f;
	CALLOC_Process(sql_cmd, (char *), len, sizeof(char))
	snprintf(sql_cmd, len - 1,
		"delete from acl_users where username='%s' and ng='%s';",
		username, newsgroup);
	if (mysql_real_query(inf->servinf->myhndl, sql_cmd, strlen(sql_cmd)) != 0) {
		MYSQL_CHK_ERR("mysql_real_query error")
	}
	free(sql_cmd);
}
