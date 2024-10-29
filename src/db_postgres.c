/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2023 Steffen Wendzel <steffen (at) wendzel (dot) de>.
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

/* Postgres Database Interface
 * Copyright (c) 2023 Christian Barthel <bch (at) online (dot) de>
 */

#include "main.h"
#include "cdpstrings.h"


extern unsigned short use_auth;	/* config.y */
extern unsigned short use_acl; /* config.y */
extern unsigned short be_verbose; /* config.y */
extern short message_body_in_db;  /* config.y */
extern short global_mode; /* global.c */

extern char period_end[];

void
db_postgres_backend_terminate(server_cb_inf *inf, int code)
{
    if (global_mode == MODE_THREAD) {
	kill_thread(inf);
	/* NOTREACHED */
    } else {
	exit(code);
    }
}

void
db_postgres_open_connection(server_cb_inf *inf)
{
	extern char *db_server; /* config.l */
	extern char *db_user; /* config.l */
	extern char *db_pass; /* config.l */
	extern short db_port;
	char db_port_string[512];
	snprintf(db_port_string, 512, "%d", db_port);

	assert (inf != NULL);

	fprintf(stderr, "db_postgres: %s, %s, %s\n",
		db_server, db_user, db_port_string);

	if ((inf->servinf->pgconn =
	     PQsetdbLogin(db_server, db_port_string,
			  NULL, NULL, "wendzelnntpd", db_user, db_pass)) == NULL) {
		DO_SYSL("Postgres Init Error! Unable to connect.")
		fprintf(stderr,
			"Postgres Init Error! Unable to connect.\n");
		db_postgres_backend_terminate(inf, 1);
	}

	ConnStatusType status = PQstatus(inf->servinf->pgconn);

	switch (status) {
	case CONNECTION_OK:
	    fprintf(stderr, "db_postgres: successfully connected\n");
	    break;
	case CONNECTION_BAD:
	default:
	    PQfinish(inf->servinf->pgconn);
	    fprintf(stderr, "db_postgres: failed to connect\n");
	    DO_SYSL("postgres connect error. Check your database configuration.");
	    fprintf(stderr,
		    "postgres connect error. Check your database configuration.\n");
	    db_postgres_backend_terminate(inf, 1);
	    break;
	}

	db_postgres_set_secure_searchpath(inf);
}

void
db_postgres_close_connection(server_cb_inf *inf)
{
    PQfinish(inf->servinf->pgconn);
}


void
db_postgres_set_secure_searchpath(server_cb_inf *inf)
{
    PGconn *conn = inf->servinf->pgconn;
    PGresult *res = PQexec(conn,
                 "SELECT pg_catalog.set_config('search_path', 'PUBLIC', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SET failed: %s", PQerrorMessage(conn));
	DO_SYSL("postgres SET failed");
        PQclear(res);
	db_postgres_backend_terminate(inf, 1);
    }
    PQclear(res);
}
/* ***** AUTHINFO PASS ***** */

void
db_postgres_authinfo_check(server_cb_inf *inf)
{
	char *sql_cmd = "select * from users where name=$1 and password=$2";
    	PGconn *conn = inf->servinf->pgconn;


	assert(global_mode == MODE_THREAD);
	/* now check if combination of user+pass is valid */

	const char *const paramValues[] = {
	    inf->servinf->cur_auth_user,
	    inf->servinf->cur_auth_pass
	};
	const int paramLengths[] = {
	    strlen(inf->servinf->cur_auth_user),
	    strlen(inf->servinf->cur_auth_pass)
	};
	const int paramFormats[] = {0,0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0)
		inf->servinf->auth_is_there = 1;
	PQclear(res);
}

/* ***** LIST ***** */

/* format: <group> <high> <low> <pflag> */

void
db_postgres_list(server_cb_inf *inf, int cmdtyp, char *wildmat)
{
	char *sql_cmd = "select * from newsgroups;";
	PGconn *conn = inf->servinf->pgconn;

	PGresult *res = NULL;

	char addbuf[1024] = { '\0' };
	int already_posted;

	if (global_mode == MODE_PROCESS) {
		printf("Newsgroup, Low-, High-Value, Posting-Flag\n");
		printf("-----------------------------------------\n");
	}

	res = PQexec(conn, sql_cmd);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    already_posted = atoi(PQgetvalue(res, i, 3));

	    if (global_mode == MODE_THREAD && use_auth && use_acl) {
		assert(0);
		/* TODO (to be implemented): check if user can see the newsgroup */
		if (db_postgres_acl_check_user_group(inf, inf->servinf->cur_auth_user, PQgetvalue(res, i, 1)) == FALSE)
		{ continue; }
	    }

	    switch (cmdtyp) {
	    case CMDTYP_XGTITLE:
	    case CMDTYP_LIST_NEWSGROUPS:
		if (wnntpd_rx_contain(wildmat, PQgetvalue(res, i, 1)) != 0) {
#ifdef DEBUG
		    fprintf(stderr, "Skipping ng %s for wildmat %s in LIST NEWSGROUPS or XGTITLE\n",
			    PQgetvalue(res, i, 1), wildmat);
#endif
		    continue; /* doesn't match wildmat */
		}
		snprintf(addbuf, 1023, "%s -\r\n", PQgetvalue(res, i, 1));
		break;
		default:
		    snprintf(addbuf, 1023, "%s %s %i %s\r\n",
			     PQgetvalue(res, i, 1),
			     PQgetvalue(res, i, 3),
			     (already_posted ? 1 : 0),
			     PQgetvalue(res, i, 2));
		    break;
	    }

	    if (global_mode == MODE_THREAD)
		ToSend(addbuf, strlen(addbuf), inf);
	    else
		printf("%s", addbuf);
	}

	PQclear(res);
}


void
db_postgres_post_insert_into_postings(server_cb_inf *inf, char *message_id,
	time_t ltime, char *from, char *ngstrpb, char *subj,
	int linecount, char *add_to_hdr)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd =
	    "insert into postings "
	    "(msgid, date, author, newsgroups, subject, lines, header) "
	    "values ($1,$2,$3,$4,$5,$6,$7)";
	char buf[128];
	char buf_line[128];
	snprintf(buf, 128-1, "%ld", ltime);
	snprintf(buf_line, 128-1, "%d", linecount);


	if (be_verbose) {
	    fprintf(stderr, "--- Dump Message ---\n");
	    fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
		    message_id, buf, from, ngstrpb, subj, buf_line, add_to_hdr);
	    fprintf(stderr, "%ld\n%ld\n%ld\n%ld\n%ld\n%ld\n%ld\n",
		    strlen(message_id),
		    strlen(buf),
		    strlen(from),
		    strlen(ngstrpb),
		    strlen(subj),
		    strlen(buf_line),
		    strlen(add_to_hdr));
	    fprintf(stderr, "--- Message End ---\n");
	}

	const char *const paramValues[] = {
	    message_id,
	    buf,
	    from,
	    ngstrpb,
	    subj,
	    buf_line,
	    add_to_hdr
	};
	const int paramLengths[] = {
	    strlen(message_id),
	    strlen(buf),
	    strlen(from),
	    strlen(ngstrpb),
	    strlen(subj),
	    strlen(buf_line),
	    strlen(add_to_hdr)
	};
	const int paramFormats[] = {0,0,0,0,0,0,0};
	int resultFormat = 0;


	PGresult *res = PQexecParams(conn,
				     sql_cmd, 7, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

void
db_postgres_post_update_high_value(server_cb_inf *inf, u_int32_t new_high, char *newsgroup)
{
    	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd = "update newsgroups set high=$1 where name=$2";
	char buf[128];
	snprintf(buf, 128-1, "%d", new_high);
	assert(global_mode == MODE_THREAD);

	const char *const paramValues[] = {
	    buf,
	    newsgroup
	};
	const int paramLengths[] = {
	    strlen(buf),
	    strlen(newsgroup)
	};
	const int paramFormats[] = {0,0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

void
db_postgres_post_insert_into_ngposts(server_cb_inf *inf, char *message_id, char *newsgroup,
	u_int32_t new_high)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd =
	    "insert into ngposts (msgid, ng, postnum) "
	    " values ($1, $2, $3);";
	char new_high_s[512];
	snprintf(new_high_s, 512-1, "%u", new_high);

	assert(global_mode == MODE_THREAD);

	const char *const paramValues[] = {
	    message_id,
	    newsgroup,
	    new_high_s
	};
	const int paramLengths[] = {
	    strlen(message_id),
	    strlen(newsgroup),
	    strlen(new_high_s)
	};
	const int paramFormats[] = {0, 0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 3, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

u_int32_t
db_postgres_get_high_value(server_cb_inf *inf, char *newsgroup)
{
	char *sql_cmd = "select high from newsgroups where name = $1";
	u_int32_t cur_high = 0;
	PGconn *conn = inf->servinf->pgconn;

	const char *const paramValues[] = {newsgroup};
	const int paramLengths[] = {strlen(newsgroup)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0)
	    cur_high = atol(PQgetvalue(res, 0, 0));

	PQclear(res);
	return cur_high;
}

int
db_postgres_chk_if_msgid_exists(server_cb_inf *inf, char *newsgroup, char *msgid)
{
	char *sql_cmd = "select * from ngposts where ng = $1 and msgid = $2";
	int boolval = 0;
	PGconn *conn = inf->servinf->pgconn;

	const char* paramValues[] = {
	    newsgroup, msgid
	};
	int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	int paramFormats[] = {0,0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0) {
		boolval = 1;
	}
	PQclear(res);
	return boolval;
}

void
db_postgres_chk_newsgroup_posting_allowed(server_cb_inf *inf)
{
    char *sql_cmd = "select * from newsgroups where name=$1 and pflag='y';";
    PGconn *conn = inf->servinf->pgconn;

    inf->servinf->found_group = 0;

    const char *const paramValues[] = {inf->servinf->chkname};
    const int paramLengths[] = {strlen(inf->servinf->chkname)};
    const int paramFormats[] = {0};
    int resultFormat = 0;


    PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL,
				 paramValues, paramLengths,
				 paramFormats, resultFormat);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	DO_SYSL("Postgres command error");
	PQclear(res);
	db_postgres_backend_terminate(inf, 1);
    }

    if (PQntuples(res) != 0)
		inf->servinf->found_group = 1;
    PQclear(res);
}

void
db_postgres_chk_newsgroup_existence(server_cb_inf *inf)
{
	char *chkname = inf->servinf->chkname;
	PGconn *conn = inf->servinf->pgconn;

	assert (conn != NULL);
	assert (chkname != NULL);

	/* NOTE: This function is also used by the admin tool! */

	inf->servinf->found_group = 0;

	const char *sql_cmd = "select * from newsgroups where name=$1";
	const char *const paramValues[] = {chkname};
	const int paramLengths[] = {strlen(chkname)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0)
		inf->servinf->found_group = 1;
	PQclear(res);
}


/* ***** XHDR ***** */

/* argv:       0         1
 * select n.postnum, p.<hdrpart>
 */

void
db_postgres_xhdr(server_cb_inf *inf, short message_id_flg, int xhdr, char *article, u_int32_t min,
		u_int32_t max)
{
	PGconn *conn = inf->servinf->pgconn;
	int len;
	char *hdr_type_str;
	char *sql_cmd;
	int articlenum;
	char *add_to_response;
	static char none[] = "(none)";
	char *value;
	char tbuf[40] = { '\0' };

	char min_s[64];
	char max_s[64];
	snprintf(min_s, 64-1, "%d", min);
	snprintf(max_s, 64-1, "%d", max);

	const char* paramValues[3];
	int paramLengths[3];
	int paramFormats[] = {0,0,0};
	int resultFormat = 0;


	assert(global_mode == MODE_THREAD);

	switch (xhdr) {
	case XHDR_FROM:
		hdr_type_str = "author";
		break;
	case XHDR_DATE:
		hdr_type_str = "date";
		/* we must convert the time() returned value stored in the db to a
		 * real standard conform string */
		inf->speccmd = SPECCMD_DATE;
		break;
	case XHDR_NEWSGROUPS:
		hdr_type_str = "newsgroups";
		break;
	case XHDR_SUBJECT:
		hdr_type_str = "subject";
		break;
	case XHDR_LINES:
		hdr_type_str = "lines";
		break;
	default:
		fprintf(stderr, "Internal Exception. No valid XHDR field!\n");
		DO_SYSL("Internal Exception. No valid XHDR field!")
		return;
	}
	int param = 0;
	paramValues[0] = hdr_type_str;
	paramLengths[0] = strlen(paramValues[0]);
	param++;
	if (message_id_flg == 1) {
	    sql_cmd =
		"select * from xhdr_get($1,$2,$3)";
	    paramValues[1] = inf->servinf->selected_group;
	    paramLengths[1] = strlen(paramValues[1]);
	    param++;
	    paramValues[2] = article;
	    paramLengths[2] = strlen(paramValues[2]);
	    param++;
	} else {
	    sql_cmd =
		"select * from xhdr_get($1, 'test.g1', null, $2, $3)";
	    paramValues[1] = min_s;
	    paramLengths[1] = strlen(paramValues[1]);
	    param++;
	    paramValues[2] = max_s;
	    paramLengths[2] = strlen(paramValues[2]);
	    param++;
	}

	ConnStatusType status = PQstatus(inf->servinf->pgconn);
	assert(status == CONNECTION_OK);
	PGresult *res = PQexecParams(conn, sql_cmd, param, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: \n%s.\n%s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}


	for (int i = 0; i < PQntuples(res); i++) {
	    articlenum = atoi(PQgetvalue(res, i, 0));

	    /* If the requested header-part is supported, we get its value in argv[1],
	     * if not, we get NULL since we did not request a send parameter in the
	     * SQL query.
	     */
	    if (PQgetvalue(res, i, 1) != NULL)
		value = PQgetvalue(res, i, 1);
	    else
		value = none;

	    /* catch date values we need to convert! */
	    if (inf->speccmd == SPECCMD_DATE) {
		nntp_localtime_to_str(tbuf, atol(PQgetvalue(res, i, 1)));
		value = tbuf;
	    }

	    len = 0xfff + 1 + strlen(value) + 2 + 1;

	    CALLOC_Thread(inf, add_to_response, (char *), len, sizeof(char));
	    snprintf(add_to_response, len - 1, "%i %s\r\n", articlenum, value);
	    ToSend(add_to_response, strlen(add_to_response), inf);
	    free(add_to_response);
	}

	PQclear(res);
	inf->speccmd = 0;
}

/* ***** ARTICLE ***** */

static void
db_postgres_article_send_header_cb(server_cb_inf *inf, char *msgid)
{
	char *sql_cmd = "select header from postings where msgid=$1;";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_THREAD);


	const char *const paramValues[] = {msgid};
	const int paramLengths[] = {strlen(msgid)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}


	if (PQntuples(res) != 0) {
	    /* add header to the send buffer */
	    switch (inf->cmdtype) {
	    case CMDTYP_ARTICLE:
		ToSend(PQgetvalue(res, 0, 0), strlen(PQgetvalue(res, 0, 0)), inf);
		ToSend("\r\n", 2, inf); /* insert empty line */
		break;
	    case CMDTYP_HEAD:
		ToSend(PQgetvalue(res, 0, 0), strlen(PQgetvalue(res, 0, 0)), inf);
		break;
	    }

	    /* add missing .\r\n\r\n on HEAD cmd */
	    if (inf->cmdtype == CMDTYP_HEAD)
		ToSend(period_end, strlen(period_end), inf);
	}

	PQclear(res);
}

void
db_postgres_article(server_cb_inf *inf,
	int type /* msgid, number or current; NOT the cmd type (ARTICLE, HEAD, ...)! */,
	char *param)
{
	int len;
	char *sql_cmd;
	char *msgid;
	char *id;
	char *sendbuffer;
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_THREAD);

	const char* paramValues[2];
	int paramLengths[2];
	int paramFormats[] = {0,0};
	int resultFormat = 0;

	switch(type) {
	case ARTCLTYP_MESSAGEID:
	    /* select * from ngposts where ng='%s' and messageid='%s'; */
	    sql_cmd = "select * from ngposts where ng=$1 and msgid=$2";
	    paramValues[0] = inf->servinf->selected_group;
	    paramValues[1] = param;
	    paramLengths[0] = strlen(paramValues[0]);
	    paramLengths[1] = strlen(paramValues[1]);
	    break;

	case ARTCLTYP_NUMBER:
	    /* select * from ngposts where ng='%s' and messageid='%s'; */
	    sql_cmd = "select * from ngposts where ng=$1 and postnum=$2";
	    paramValues[0] = inf->servinf->selected_group;
	    paramValues[1] = param;
	    paramLengths[0] = strlen(paramValues[0]);
	    paramLengths[1] = strlen(paramValues[1]);
	    break;

	case ARTCLTYP_CURRENT:
	    /* select * from ngposts where ng='%s' and messageid='%s'; */
	    sql_cmd = "select * from ngposts where ng=$1 and postnum=$2";
	    paramValues[0] = inf->servinf->selected_group;
	    paramValues[1] = inf->servinf->selected_article;
	    paramLengths[0] = strlen(paramValues[0]);
	    paramLengths[1] = strlen(paramValues[1]);
	    break;
	}

	assert (sql_cmd != NULL);
	PGresult *res = PQexecParams(conn, sql_cmd, 2, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0) {
	    msgid = PQgetvalue(res, 0, 0);
	    id = PQgetvalue(res, 0, 2);

	    len = 0xff + strlen(msgid) + strlen(id);
	    CALLOC_Thread(inf, sendbuffer, (char *), len + 1, sizeof(char));

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
		snprintf(sendbuffer, len,
			 "220 %s %s article retrieved - head and body follow\r\n",
			 id, msgid);
		break;
	    case CMDTYP_HEAD:
		snprintf(sendbuffer, len,
			 "221 %s %s article retrieved - head follows\r\n", id, msgid);
		break;
	    case CMDTYP_BODY:
		snprintf(sendbuffer, len,
			 "222 %s %s article retrieved - body follows\r\n", id, msgid);
		break;
	    case CMDTYP_STAT:
		snprintf(sendbuffer, len,
			 "223 %s %s article retrieved - request text separately\r\n",
			 id, msgid);
		break;
	    }
	    ToSend(sendbuffer, strlen(sendbuffer), inf);
	    free(sendbuffer);

	    /* send the header, if needed */
	    if (inf->cmdtype == CMDTYP_ARTICLE || inf->cmdtype == CMDTYP_HEAD) {
		db_postgres_article_send_header_cb(inf, msgid);
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
	PQclear(res);
}


/* ***** GROUP ***** */

static void
db_postgres_group_cb_set_first_article_in_group(server_cb_inf *inf)
{
	char *sql_cmd = "select min(postnum) from ngposts where ng=$1";
	PGconn *conn = inf->servinf->pgconn;
	char *group = inf->servinf->selected_group;
	assert(global_mode == MODE_THREAD);

	/* This var will get changed, if an article is available */
	inf->servinf->selected_article = NULL;


	const char *const paramValues[] = {group};
	const int paramLengths[] = {strlen(group)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) == 1) {
	    inf->servinf->selected_article = strdup(PQgetvalue(res, 0, 0));
	}
	PQclear(res);
}

void
db_postgres_group(server_cb_inf *inf, char *group)
{
	char *sql_cmd = "select * from newsgroups where name=$1";
	PGconn *conn = inf->servinf->pgconn;
	int estimated, first, last;
	char *buf;
	int old_foundgroup = inf->servinf->found_group;

	assert(global_mode == MODE_THREAD);

	const char *const paramValues[] = {group};
	const int paramLengths[] = {strlen(group)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	inf->servinf->found_group = 0;
	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0) {
	    inf->servinf->found_group = 1;
	} else
	    inf->servinf->found_group = 0;

	for (int i = 0; i < PQntuples(res); i++) {
	    last = atoi(PQgetvalue(res, i, 3));
	    if (last)
		first = 1;
	    else
		first = 0;
	    estimated = last;


	    CALLOC_Thread(inf, buf, (char *), strlen(PQgetvalue(res, i, 1)) + 0xff, sizeof(char))
		snprintf(buf, strlen(PQgetvalue(res, i, 1)) + 0xff - 1,
			 "211 %i %i %i %s group selected\r\n",
			 estimated, first, last, PQgetvalue(res, i, 1));
	    ToSend(buf, strlen(buf), inf);
	    free(buf);

	    if (inf->servinf->selected_group)
		free(inf->servinf->selected_group);

	    CALLOC_Thread(inf,
			  inf->servinf->selected_group,
			  (char *),
			  strlen(PQgetvalue(res, i, 1)) + 1, sizeof(char));
	    strncpy(inf->servinf->selected_group,
		    PQgetvalue(res, i, 1),
		    strlen(PQgetvalue(res, i, 1)));

	    /* unset the selected article */
	    inf->servinf->selected_article = NULL;
	    /* we found the group */
	    inf->servinf->found_group = 1;
	    /* Since the above 'first' value is just FAKE, we need to find out the correct one */
	    db_postgres_group_cb_set_first_article_in_group(inf);
	}
	if (inf->servinf->found_group == 0)
	    inf->servinf->found_group = old_foundgroup;
	PQclear(res);
}

/* ***** LISTGROUP ***** */

void
db_postgres_listgroup(server_cb_inf *inf, char *group)
{
	char *sql_cmd = "select postnum from ngposts where ng=$1 order by postnum asc";
	int article_counter = 0;
	char *buf;
    	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_THREAD);


	const char *const paramValues[] = {
	    group
	};
	const int paramLengths[] = { strlen(paramValues[0]) };
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 1, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++ ) {
	    int len = strlen(PQgetvalue(res, i, 0)) + 3;

	    CALLOC_Thread(inf, buf, (char *), len + 1, sizeof(char));
	    snprintf(buf, len, "%s\r\n", PQgetvalue(res, i, 0));
	    ToSend(buf, strlen(buf), inf);

		/* set the new article pointer to the first article in the group
		 * (RFC need!) */
	    if (article_counter == 0) {
		if (inf->servinf->selected_article)
		    free(inf->servinf->selected_article);
		inf->servinf->selected_article = strdup(PQgetvalue(res, i, 0));
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

	if (article_counter == 0) {
		inf->servinf->selected_article = NULL;
	}

	/* Send trailing '.' */
	ToSend(".\r\n", 3, inf);
	PQclear(res);
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
db_postgres_xover(server_cb_inf *inf, u_int32_t min, u_int32_t max)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd =
	    "select n.postnum, p.subject, p.author, p.date, n.msgid, p.lines, p.header"
	    " from ngposts n,postings p"
	    " where ng=$1 and (postnum between $2 and $3) and n.msgid = p.msgid;";
	int len, articlenum, lines;
	int postlen;
	char *ref, *xref, *bodylen;
	char *add_to_response;
	char tbuf[40] = { '\0' };

	char min_s[256];
	char max_s[256];
	snprintf(min_s, 256-1, "%u", min);
	snprintf(max_s, 256-1, "%u", max);

	assert(global_mode == MODE_THREAD);

	const char *const paramValues[] = {
	    inf->servinf->selected_group,
	    min_s, max_s
	};
	const int paramLengths[] = {
	    strlen(inf->servinf->selected_group),
	    strlen(min_s),
	    strlen(max_s)
	};
	const int paramFormats[] = {0,0,0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 3, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    articlenum = atoi(PQgetvalue(res, i, 0));
	    lines = atoi(PQgetvalue(res, i, 5));
	    bodylen = CDP_return_linevalue(PQgetvalue(res, i, 6), "X-WendzelNNTPdBodysize:");

	    if (bodylen) {
		postlen = atoi(bodylen);
		free(bodylen);
	    }
	    postlen += strlen(PQgetvalue(res, i, 6));

	    ref = CDP_return_linevalue(PQgetvalue(res, i, 6), "References:");
	    xref = CDP_return_linevalue(PQgetvalue(res, i, 6), "Xref:");

	    len = 0xff
		+ strlen(PQgetvalue(res, i, 1))
		+ strlen(PQgetvalue(res, i, 2))
		+ 39 /* timestamp len */
		+ strlen(PQgetvalue(res, i, 4));
	    if (ref)
		len += strlen(ref);
	    if (xref)
		len += strlen(xref);

	    nntp_localtime_to_str(tbuf, atol(PQgetvalue(res, i, 3)));

	    CALLOC_Thread(inf, add_to_response, (char *), len, sizeof(char));

	    snprintf(add_to_response, len - 1,
		     "%i\t%s\t%s\t%s\t%s\t%s\t%i\t%i\t%s\r\n",
		     articlenum,
		     PQgetvalue(res, i, 1),
		     PQgetvalue(res, i, 2),
		     tbuf,
		     PQgetvalue(res, i, 4),
		     (ref ? ref : " "),
		     postlen,
		     lines,
		     (xref ? xref : " "));
	    ToSend(add_to_response, strlen(add_to_response), inf);

	    if (ref)
		free(ref);
	    if (xref)
		free(xref);
	    free(add_to_response);
	}
	PQclear(res);
}

/************* ACL **************/

short
db_postgres_acl_check_user_group(server_cb_inf *inf, char *user, char *newsgroup)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd = "SELECT * FROM acl_users WHERE username=$1 AND ng=$2;";
	short allowed = FALSE;

	assert(global_mode == MODE_THREAD);

	const char *const paramValues[] = {
	    user, newsgroup
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[0])
	};
	const int paramFormats[] = {0,0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0)
		allowed = TRUE;

	PQclear(res);

	/* 2nd try: check, if the user has access to the group via an ACL
	 * role he is member in */
	if (allowed == FALSE) {
	    sql_cmd = "SELECT * FROM users2roles ur,acl_roles aclr WHERE "
		"ur.username=$1 AND ur.role=aclr.role AND aclr.ng=$2;";

	    res = PQexecParams(conn,
					 sql_cmd, 2, NULL,
					 paramValues, paramLengths,
					 paramFormats, resultFormat);
	    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		fprintf(stderr, "Postgres command error: %s. %s\n",
			sql_cmd, PQerrorMessage(conn));
		DO_SYSL("Postgres command error");
		PQclear(res);
		db_postgres_backend_terminate(inf, 1);
	    }

	    if (PQntuples(res) != 0)
		allowed = TRUE;
	}

	PQclear(res);
	return allowed;
}

/* ****** ADMINISTRATION ***** */

void
db_postgres_create_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	PGconn *conn = inf->servinf->pgconn;
	char *post = (post_flg == 'y')? "y" : "n";
	char *high = "0";
	assert (conn != NULL);
	assert (newsgroup != NULL);
	assert(global_mode == MODE_PROCESS);

	char *sql_cmd = "insert into newsgroups (name, pflag, high) values ($1, $2, $3);";

	const char *const paramValues[] = {newsgroup, post, high};
	const int paramLengths[] = {
	    strlen(newsgroup),
	    strlen(post),
	    strlen(high)};
	const int paramFormats[] = {0, 0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 3, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

void
db_postgres_delete_newsgroup(server_cb_inf *inf, char *newsgroup)
{
	char *sql_cmd = "delete from newsgroups where \"name\"=$1;";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);

	/* Delete on cascade: removes all postings within other tables.
	 * Do not bother with deleting them manually.  */
	const char *const paramValues[] = {newsgroup};
	const int paramLengths[] = {strlen(newsgroup)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

void
db_postgres_modify_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	char *sql_cmd = "update newsgroups set pflag = $2 where name = $1";
	char *post = (post_flg == 'y')? "y" : "n";
	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {newsgroup, post};
	const int paramLengths[] = {strlen(newsgroup), strlen(post)};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 2, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);
}

/* These functions are used by WendzelNNTPadm what means that global_mode must be
 * MODE_PROCESS!
 */

void
db_postgres_list_users(server_cb_inf *inf)
{
	char *sql_cmd = "select * from users;";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);

	printf("Username, Password\n");
	printf("------------------\n");

	PGresult *res = PQexec(conn, sql_cmd);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    printf("%s, %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
	}
	PQclear(res);
}


void
db_postgres_chk_user_existence(server_cb_inf *inf)
{
	PGconn *conn = inf->servinf->pgconn;
	char *chkname = inf->servinf->chkname;
	char *sql_cmd = "select * from users where name=$1";

	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {chkname};
	const int paramLengths[] = {strlen(chkname)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0)
		inf->servinf->found_group = 1;
	PQclear(res);
}

void
db_postgres_chk_role_existence(server_cb_inf *inf)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd = "select * from roles where role = $1;";

	assert(global_mode == MODE_PROCESS);
	const char *const paramValues[] = {
	    inf->servinf->chkname
	};
	const int paramLengths[] = { strlen(paramValues[0]) };
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 1, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 0) {
		inf->servinf->found_group = 1;
	}
	PQclear(res);
}

void
db_postgres_acl_add_role(server_cb_inf *inf, char *role)
{
	char *sql_cmd = "insert into roles (role) values ($1);";
	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {
	    role
	};
	const int paramLengths[] = {
	    strlen(paramValues[0])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 1, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
}

void
db_postgres_acl_del_role(server_cb_inf *inf, char *role)
{
	char *sql_cmd = "delete from roles where role=$1";
	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_PROCESS);

	/* First remove the associated data sets */
	/* pt. 1: users */
	printf("Removing associations of role %s with their users ... ", role);
	printf("done\n");

	/* pt. 2: newsgroups */
	printf("Removing associations of role %s with their newsgroups ... ", role);
	printf("done\n");


	/* Now remove the role */
	printf("Removing role %s ... ", role);
	const char *const paramValues[] = {
	    role
	};
	const int paramLengths[] = {
	    strlen(paramValues[0])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 1, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
	printf("done\n");
}

void
db_postgres_acl_role_connect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd ="INSERT INTO acl_roles (role, ng) VALUES ($1,$2);" ;

	assert(global_mode == MODE_PROCESS);

	printf("Connecting role %s with newsgroup %s ... ", role, newsgroup);

	const char *const paramValues[] = {
	    role, newsgroup
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
	printf("done\n");
}

void
db_postgres_acl_role_disconnect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	char *sql_cmd = "DELETE FROM acl_roles WHERE role='%s' AND ng='%s';";
	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_PROCESS);

	printf("Dis-Connecting role %s from newsgroup %s ... ", role, newsgroup);
	const char *const paramValues[] = {
	    role, newsgroup
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);

	printf("done\n");
}

void
db_postgres_acl_role_connect_user(server_cb_inf *inf, char *role, char *user)
{
	char *sql_cmd = "INSERT INTO users2roles (username, role) VALUES ($1,$2);";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);

	printf("Connecting role %s with user %s ... ", role, user);

	const char *const paramValues[] = {
	    user, role
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);

	printf("done\n");
}

void
db_postgres_acl_role_disconnect_user(server_cb_inf *inf, char *role, char *user)
{
	char *sql_cmd = "DELETE FROM users2roles WHERE role=$1 AND username=$2;";
	PGconn *conn = inf->servinf->pgconn;
	assert(global_mode == MODE_PROCESS);

	printf("Dis-Connecting role %s from user %s ... ", role, user);


	const char *const paramValues[] = {
	    role, user
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
	printf("done\n");
}

void
db_postgres_list_acl_tables(server_cb_inf *inf)
{
	PGconn *conn = inf->servinf->pgconn;
	char *sql_cmd ="select role from roles";

	assert(global_mode == MODE_PROCESS);

	printf("List of roles in database:\n");
	printf("Roles\n");
	printf("-----\n");
	sql_cmd = "select role from roles";

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 0, NULL,
				     NULL, NULL,
				     NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    printf("%s\n", PQgetvalue(res, i, 0));
	}
	PQclear(res);

	putchar('\n');
	printf("Connections between users and roles:\n"
		"Role, User\n"
		"----------\n");
	sql_cmd = "select role,username from users2roles order by role asc;";
	res = PQexecParams(conn,
			   sql_cmd, 0, NULL,
			   NULL, NULL,
			   NULL, 0);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    printf("%s, %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
	}
	PQclear(res);

	putchar('\n');
	printf("Username, Has access to group\n");
	printf("-----------------------------\n");
	sql_cmd = "select username,ng from acl_users;";
	res = PQexecParams(conn,
			   sql_cmd, 0, NULL,
			   NULL, NULL,
			   NULL, 0);


	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    printf("%s %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
	}
	PQclear(res);


	putchar('\n');
	printf("Role, Has access to group\n");
	printf("-------------------------\n");
	sql_cmd = "select role,ng from acl_roles;";
	res = PQexecParams(conn,
			   sql_cmd, 0, NULL,
			   NULL, NULL,
			   NULL, 0);


	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	for (int i = 0; i < PQntuples(res); i++) {
	    printf("%s %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
	}
	PQclear(res);
}


void
db_postgres_add_user(server_cb_inf *inf, char *username, char *password)
{
	char *sql_cmd = "insert into users (name, password) values ($1, $2);";
	PGconn *conn = inf->servinf->pgconn;


	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {username, password};
	const int paramLengths[] = {strlen(username), strlen(password)};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 2, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	/* Make sure we left no password in the memory */
	PQclear(res);
}

void
db_postgres_del_user(server_cb_inf *inf, char *username)
{
	char *sql_cmd = "delete from users where name=$1";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);
	printf("Deleting user %s from database ... ", username);

	const char *const paramValues[] = {username};
	const int paramLengths[] = {strlen(username)};
	const int paramFormats[] = {0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn, sql_cmd, 1, NULL, paramValues, paramLengths,
                     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n", sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	PQclear(res);

	printf("done\n");

}


void
db_postgres_acl_add_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd = "insert into acl_users (username, ng) values ($1,$2);";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {
	    username, newsgroup
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
}

void
db_postgres_acl_del_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	char *sql_cmd = "delete from acl_users where username=$1 and ng=$2";
	PGconn *conn = inf->servinf->pgconn;

	assert(global_mode == MODE_PROCESS);

	const char *const paramValues[] = {
	    username, newsgroup
	};
	const int paramLengths[] = {
	    strlen(paramValues[0]),
	    strlen(paramValues[1])
	};
	const int paramFormats[] = {0, 0};
	int resultFormat = 0;

	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
}


void
db_postgres_store_message_body(server_cb_inf *inf, char *message_id, char *body)
{
	char *sql_cmd = "insert into body (msgid, content) values ($1,$2);";
	PGconn *conn = inf->servinf->pgconn;

	const char *const paramValues[] = { message_id, body };
	const int paramLengths[] = { strlen(paramValues[0]), strlen(paramValues[1])};
	const int paramFormats[] = {0,0};
	int resultFormat = 0;


	PGresult *res = PQexecParams(conn,
				     sql_cmd, 2, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}
	PQclear(res);
}

char*
db_postgres_load_message_body(server_cb_inf *inf, char *message_id)
{
	char *sql_cmd =
	    "select octet_length(content), content from body where msgid = $1";
	PGconn *conn = inf->servinf->pgconn;

	const char *const paramValues[] = { message_id };
	const int paramLengths[] = { strlen(paramValues[0]) };
	const int paramFormats[] = {0};
	int resultFormat = 0;


	PGresult *res = PQexecParams(conn,
				     sql_cmd, 1, NULL,
				     paramValues, paramLengths,
				     paramFormats, resultFormat);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) <= 0) {
	    PQclear(res);
	    return NULL;
	}

	assert (PQntuples(res) == 1); /* msgid is a primary key */

	unsigned int octet = atoi(PQgetvalue(res, 0, 0));
	char *body = PQgetvalue(res, 0, 1);

	if (body == NULL) {
	    PQclear(res);
	    return NULL;
	}

	char *dup_s = strndup(body, octet);

	if (be_verbose) {
	    fprintf(stderr, "octent length / strlen: %u/%lu Bytes at (%p)\n",
		    octet, strlen(body), body);
	}
	PQclear(res);
	return dup_s;
}

char*
db_postgres_get_uniqnum(server_cb_inf *inf)
{
	char *sql_cmd = "select nextval('nntp_next_msg_id')";
	PGconn *conn = inf->servinf->pgconn;

	if (be_verbose) {
	    fprintf(stderr, "fetch new number\n");
	}

	PGresult *res = PQexec(conn, sql_cmd);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
	    fprintf(stderr, "Postgres command error: %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	if (PQntuples(res) != 1) {
	    fprintf(stderr,
		    "Postgres command error (assert: row count = 1): %s. %s\n",
		    sql_cmd, PQerrorMessage(conn));
	    DO_SYSL("Postgres command error");
	    PQclear(res);
	    db_postgres_backend_terminate(inf, 1);
	}

	char *buf;
	if (!(buf = (char *) calloc(MAX_IDNUM_LEN + 1, sizeof(char)))) {
		DO_SYSL("Not enough memory!")
		return NULL;
	}
	snprintf(buf, MAX_IDNUM_LEN, "<cdp%s@", PQgetvalue(res, 0, 0));
	if (be_verbose) {
	    fprintf(stderr, "allocated new number: %s\n", PQgetvalue(res, 0, 0));
	}
	PQclear(res);
	return buf;
}
