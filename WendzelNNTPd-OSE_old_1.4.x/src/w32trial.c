/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2007-2009 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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

/* This file was primary created to check, if there are already 60 or more
 * of the 60 allowed posts for the w32 trial version. Since I now removed
 * this handycap for w32 users, I also removed the code in this file. The
 * only reason why this file still exists is that it checks the database
 * connection on daemon startup.
 */

#include "main.h"

static int
check_db_cb(void *unused, int argc, char **argv, char **ColName)
{
	/* do nothing */
	return 0;
}

/* the DB connection should work fine if we'll receive no error here ... */
void
check_db(void)
{
	sqlite3 *db;
	char *sql_cmd;
	char *sqlite_err_msg;

	if (sqlite3_open(DBFILE, &db)) {
		DO_SYSL("Unable to open database.")
		exit(1);
	}

	if (!(sql_cmd = (char *)calloc(0xff, sizeof(char)))) {
		perror("not enough memory!\n");
		exit(1);
	}
	
	snprintf(sql_cmd, 0xff - 1, "select count(*) from newsgroups;");
	if (sqlite3_exec(db, sql_cmd, check_db_cb, 0, &sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "sqlite: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		DO_SYSL("Unable to exec on database " DBFILE ".")
		exit(1);
	}
	free(sql_cmd);
}


