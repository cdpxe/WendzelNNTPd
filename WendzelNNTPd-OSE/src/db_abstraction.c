/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2009-2015 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

/* Database Abstraction Interface */

#include "main.h"

extern unsigned short dbase;	/* from config.y */

void
db_authinfo_check(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_authinfo_check(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_authinfo_check(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_list(server_cb_inf *inf, int cmdtyp, char *wildmat)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_list(inf, cmdtyp, wildmat);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_list(inf, cmdtyp, wildmat);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_xhdr(server_cb_inf *inf, short message_id_flg, int xhdr, char *article,
	u_int32_t min, u_int32_t max)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_xhdr(inf, message_id_flg, xhdr, article, min, max);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_xhdr(inf, message_id_flg, xhdr, article, min, max);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_article(server_cb_inf *inf, int type, char *param)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_article(inf, type, param);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_article(inf, type, param);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_group(server_cb_inf *inf, char *group)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_group(inf, group);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_group(inf, group);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_listgroup(server_cb_inf *inf, char *group)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_listgroup(inf, group);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_listgroup(inf, group);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_xover(server_cb_inf *inf, u_int32_t min, u_int32_t max)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_xover(inf, min, max);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_xover(inf, min, max);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

u_int32_t
db_get_high_value(server_cb_inf *inf, char *newsgroup)
{
	u_int32_t retval;
	
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		retval = db_sqlite3_get_high_value(inf, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		retval = db_mysql_get_high_value(inf, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(1);
	}
	return retval;
}

int
db_chk_if_msgid_exists(server_cb_inf *inf, char *newsgroup, char *msgid)
{
	int retval;
	
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		retval = db_sqlite3_chk_if_msgid_exists(inf, newsgroup, msgid);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		retval = db_mysql_chk_if_msgid_exists(inf, newsgroup, msgid);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(1);
	}
	return retval;
}


void
db_check_newsgroup_existence(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_chk_newsgroup_existence(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_chk_newsgroup_existence(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_check_user_existence(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_chk_user_existence(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_chk_user_existence(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_check_role_existence(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_chk_role_existence(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_chk_role_existence(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_post_insert_into_postings(server_cb_inf *inf, char *message_id,
	time_t ltime, char *from, char *ngstrpb, char *subj,
	int linecount, char *add_to_hdr)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_post_insert_into_postings(inf, message_id, ltime,
			from, ngstrpb, subj, linecount, add_to_hdr);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_post_insert_into_postings(inf, message_id, ltime,
			from, ngstrpb, subj, linecount, add_to_hdr);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_post_update_high_value(server_cb_inf *inf, u_int32_t high, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_post_update_high_value(inf, high, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_post_update_high_value(inf, high, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_post_insert_into_ngposts(server_cb_inf *inf, char *message_id, char *newsgroup,
	u_int32_t new_high)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_post_insert_into_ngposts(inf, message_id, newsgroup,
			new_high);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_post_insert_into_ngposts(inf, message_id, newsgroup,
			new_high);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

/* Access Control List Features */

/* check if a user 'user' has access to newsgroup 'newsgroup' */
short
db_acl_check_user_group(server_cb_inf *inf, char *user, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		return db_sqlite3_acl_check_user_group(inf, user, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		return db_mysql_acl_check_user_group(inf, user, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
	/* NOTREACHED */
	return FALSE;
}


/* Administrative Functions */

void
db_list_users(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_list_users(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_list_users(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_list_acl_tables(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_list_acl_tables(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_list_acl_tables(inf);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_add_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_add_user(inf, username, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_add_user(inf, username, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_del_user(server_cb_inf *inf, char *username, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_del_user(inf, username, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_del_user(inf, username, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_add_role(server_cb_inf *inf, char *role)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_add_role(inf, role);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_add_role(inf, role);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_del_role(server_cb_inf *inf, char *role)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_del_role(inf, role);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_del_role(inf, role);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_role_connect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_role_connect_group(inf, role, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_role_connect_group(inf, role, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_role_unconnect_group(server_cb_inf *inf, char *role, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_role_unconnect_group(inf, role, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_role_unconnect_group(inf, role, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_role_connect_user(server_cb_inf *inf, char *role, char *user)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_role_connect_user(inf, role, user);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_role_connect_user(inf, role, user);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_acl_role_unconnect_user(server_cb_inf *inf, char *role, char *user)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_acl_role_unconnect_user(inf, role, user);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_acl_role_unconnect_user(inf, role, user);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}


void
db_create_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_create_newsgroup(inf, newsgroup, post_flg);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_create_newsgroup(inf, newsgroup, post_flg);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_delete_newsgroup(server_cb_inf *inf, char *newsgroup)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_delete_newsgroup(inf, newsgroup);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_delete_newsgroup(inf, newsgroup);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_modify_newsgroup(server_cb_inf *inf, char *newsgroup, char post_flg)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_modify_newsgroup(inf, newsgroup, post_flg);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_modify_newsgroup(inf, newsgroup, post_flg);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_add_user(server_cb_inf *inf, char *username, char *password)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_add_user(inf, username, password);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_add_user(inf, username, password);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

void
db_del_user(server_cb_inf *inf, char *username)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_del_user(inf, username);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_del_user(inf, username);
		break;
#endif
	default:
		DO_SYSL("NOT IMPLEMENTED; TODO!")
	}
}

/* Main Functions */

char *
db_secure_sqlbuffer(server_cb_inf *inf, char *in)
{
	char *sec_cmd = NULL;
	
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		if ((sec_cmd = sqlite3_mprintf("%q", in)) == NULL) {
			DO_SYSL("sqlite3_mprintf() returned NULL")
			kill_thread(inf);
			/* NOTREACHED */
		}
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		sec_cmd = (char *) calloc((strlen(in) * 2) + 1, sizeof(char));
		if (!sec_cmd) {
			DO_SYSL("Not enough memory! Closing client connection")
			kill_thread(inf);
			/* NOTREACHED */
		}
		mysql_real_escape_string(inf->servinf->myhndl, sec_cmd, in, strlen(in));
		break;
#endif
	default:
		fprintf(stderr, "NOT IMPLEMENTED: db_secure_sqlbuffer; dbase=%i\n",
			dbase);
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(ERR_EXIT);
	}
	return sec_cmd;
}

void
db_secure_sqlbuffer_free(char *buf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		sqlite3_free(buf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		free(buf);
		break;
#endif
	default:
		fprintf(stderr, "NOT IMPLEMENTED: db_secure_sqlbuffer_free; dbase=%i\n",
			dbase);
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(ERR_EXIT);
	}
}

void
db_open_connection(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_open_connection(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_open_connection(inf);
		break;
#endif
	default:
		fprintf(stderr, "NOT IMPLEMENTED: db_open_connection; dbase=%i\n",
			dbase);
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(ERR_EXIT);
	}
}

void
db_close_connection(server_cb_inf *inf)
{
	switch (dbase) {
#ifndef NOSQLITE
	case DBASE_SQLITE3:
		db_sqlite3_close_connection(inf);
		break;
#endif
#ifndef NOMYSQL
	case DBASE_MYSQL:
		db_mysql_close_connection(inf);
		break;
#endif
	default:
		fprintf(stderr, "NOT IMPLEMENTED: db_close_connection; dbase=%i\n",
			dbase);
		DO_SYSL("NOT IMPLEMENTED; TODO!")
		exit(ERR_EXIT);
	}
}

