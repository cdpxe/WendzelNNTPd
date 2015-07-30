/*
 * WendzelNNTPadm is distributed under the following license:
 *
 * Copyright (c) 2004-2009 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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
#ifndef __WIN32__
	#include <pwd.h>
#endif

#ifndef THIS_TOOLNAME
	#define THIS_TOOLNAME	"wendzelnntpadm"
#endif

#define BUFSIZE		4096

#define MOD_NGCREATE	0x01
#define MOD_NGDELETE	0x02
#define MOD_NGMODIFY	0x03
#define MOD_NGLIST	0x04

#define MOD_ADDUSER	0x11
#define MOD_DELUSER	0x12
#define MOD_LISTUSER	0x13

u_int8_t mode;

static void usage(void);

static void
usage()
{
	fprintf(stderr, "usage: " THIS_TOOLNAME " <create | delete | modify> <newsgroup> [<posting-allowed-flag (y/n)>]\n"
			"       " THIS_TOOLNAME " <adduser> <username> [<password>]\n"
			"       " THIS_TOOLNAME " <deluser> <username>\n"
			"       " THIS_TOOLNAME " <listgroups | listusers>\n");
	exit(ERR_EXIT);
}

static int
create_cb(void *unused, int argc, char **argv, char **ColName)
{
	if (atoi(argv[0]) != 0) {
		fprintf(stderr, "This newsgroup does already exist.\n");
		exit(1);
	}
	return 0;
}

/* CB function for printing a list of either newsgroups OR users */
static int
list_cb(void *unused, int argc, char **argv, char **ColName)
{
	static int first_call = 1;
	
	if (first_call)
		switch (mode) {
		case MOD_NGLIST:
			printf("Name, Posting, Messages\n-----------------------\n");
			break;
		case MOD_LISTUSER:
			printf("Name, Password\n--------------\n");
			break;
		}
	
	if (mode == MOD_LISTUSER) {
		/* users */
		printf("%s, %s\n", argv[0], argv[1]);
	} else {
		/* groups */
		printf("%s, %s, %s\n", argv[1], argv[2], argv[3]);
	}
	first_call = 0;
	return 0;
}


int
main(int argc, char *argv[])
{
	sqlite3 *db;
	char cmd[BUFSIZE];
	char *sqlite_err_msg = NULL;
	char *pass;

	if (argc < 3)
		if (! (argc == 2 && ((strcmp(argv[1], "listgroups") == 0) || (strcmp(argv[1], "listusers") == 0))))
			usage();

	if (sqlite3_open(DBFILE, &db)) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(ERR_EXIT);
	}

	if (strcmp(argv[1], "listgroups") != 0 && strcmp(argv[1], "listusers") != 0 && strlen(argv[2]) > (BUFSIZE - 0x7f)) {
		fprintf(stderr, "Newsgroup name '%s' is too long.\n", argv[2]);
		exit(ERR_EXIT);
	}
	
	if (strcmp(argv[1], "create") == 0)
		mode = MOD_NGCREATE;
	else if (strcmp(argv[1], "delete") == 0)
		mode = MOD_NGDELETE;
	else if (strcmp(argv[1], "modify") == 0)
		mode = MOD_NGMODIFY;
	else if (strcmp(argv[1], "listgroups") == 0)
		mode = MOD_NGLIST;
	else if (strcmp(argv[1], "adduser") == 0)
		mode = MOD_ADDUSER;
	else if (strcmp(argv[1], "deluser") == 0)
		mode = MOD_DELUSER;
	else if (strcmp(argv[1], "listusers") == 0)
		mode = MOD_LISTUSER;
	else {
		fprintf(stderr, "Invalid mode: %s'.\n", argv[1]);
		exit(ERR_EXIT);
	}
	
	switch(mode) {
	case MOD_NGCREATE:
		if (argc < 4)
			usage();
		/* first check if the group already exists */
		snprintf(cmd, BUFSIZE-1, "select count(*) from newsgroups where name='%s';", argv[2]);
		if (sqlite3_exec(db, cmd, create_cb, 0, &sqlite_err_msg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			sqlite3_free(sqlite_err_msg);
			exit(ERR_EXIT);
		}
		/* now insert the new group */
		bzero(cmd, BUFSIZE);
		snprintf(cmd, BUFSIZE-1,
			"insert into newsgroups (name, pflag, high) values ('%s', '%c', '0');",
			argv[2], argv[3][0]);
		break;
	case MOD_NGDELETE:
		/* 1st: Delete the Newsgroup itself */
		printf("Deleting newsgroup %s itself ...\n", argv[2]);
		snprintf(cmd, BUFSIZE-1,
			"delete from newsgroups where name = '%s';", argv[2]);
		if (sqlite3_exec(db, cmd, create_cb, 0, &sqlite_err_msg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			sqlite3_free(sqlite_err_msg);
			exit(ERR_EXIT);
		}
		/* 2nd: Delete all posting-to-newsgroup entries from the association class
		 * ngposts which belong to %newsgroup */
		printf("Clearing association class ...\n");
		bzero(cmd, BUFSIZE);
		snprintf(cmd, BUFSIZE-1,
			"DELETE FROM ngposts WHERE ngposts.ng='%s';", argv[2]);
		if (sqlite3_exec(db, cmd, create_cb, 0, &sqlite_err_msg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
			sqlite3_free(sqlite_err_msg);
			exit(ERR_EXIT);
		}

		/* 3rd: Delete all remaining postings which do not belong to any existing
		 * newsgroup. */
		printf("Deleting postings that do not belong to an existing newsgroup ...\n");
		bzero(cmd, BUFSIZE);
		snprintf(cmd, BUFSIZE-1,
			"DELETE FROM postings WHERE NOT EXISTS (select * from ngposts "
			"where postings.msgid = ngposts.msgid);");
		break;
	case MOD_NGMODIFY:
		if (argc < 4)
			usage();
		snprintf(cmd, BUFSIZE-1, "update newsgroups set pflag = '%c' where name = '%s';",
			argv[3][0], argv[2]);
		break;
	case MOD_NGLIST:
		snprintf(cmd, BUFSIZE-1, "select * from newsgroups;");
		break;
	case MOD_ADDUSER:
		/* If the password was given as a parameter: use it */
		if (argc >= 4) {
			pass = argv[3];
		}
		/* If the password was not given as a parameter: read it */
		else {
#ifndef __WIN32__ /* why the hell does win32 don't support this func??? */
			pass = getpass("Enter new password for this user (max. 100 chars):");
#else
			if (!(pass = (char *)calloc(101, sizeof(char)))) {
				perror("calloc");
				exit(1);
			}
			printf("Enter new password for this user (max. 100 chars): ");
			fgets(pass, 100, stdin);
			pass[strlen(pass) - 1] = '\0'; /* remove \n */
#endif
		}
		
		if (strlen(argv[2]) > 100) { printf("username too long (max. 100 chars).\n"); exit(1); }
		if (strlen(pass) < 8) { printf("password too short (min. 8 chars).\n"); exit(1); }
		/* this check is at least needed for the *nix getpass() version above */
		if (strlen(pass) > 100) { printf("password too long (max. 100 chars).\n"); exit(1); }
		
		snprintf(cmd, BUFSIZ - 1, "insert into users (name, password) values ('%s', '%s');", argv[2], pass);

		/* bzero the pass just to clean up the mem */
		memset(pass, 0x0, strlen(pass));
		break;
	case MOD_DELUSER:
		snprintf(cmd, BUFSIZ - 1, "delete from users where name='%s';", argv[2]);
		break;
	case MOD_LISTUSER:
		snprintf(cmd, BUFSIZ - 1, "select * from users;");
		break;
	default:
		fprintf(stderr, "unknown command implementation.\n");
		exit(1);
	};
#ifdef DEBUG
	printf("executing cmd: %s\n", cmd);
#endif
	if (sqlite3_exec(db, cmd, list_cb, 0, &sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		exit(ERR_EXIT);
	}
	sqlite3_close(db);
	printf("done.\n");
	return 0;
}

