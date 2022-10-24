/*
 * WendzelNNTPadm is distributed under the following license:
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

#include "main.h"
#ifndef __WIN32__
	#include <pwd.h>
#endif

#ifndef THIS_TOOLNAME
	#define THIS_TOOLNAME	"wendzelnntpadm"
#endif

#define MOD_NGCREATE	0x01
#define MOD_NGDELETE	0x02
#define MOD_NGMODIFY	0x03
#define MOD_NGLIST	0x04

#define MOD_ADDUSER	0x11
#define MOD_DELUSER	0x12
#define MOD_LISTUSER	0x13

#define MOD_ADDACLUSER	0x21
#define MOD_DELACLUSER	0x22
#define MOD_LISTACL	0x23
#define MOD_ADDACLROLE	0x24
#define MOD_DELACLROLE	0x25
#define MOD_ROLEGROUPC	0x26
#define MOD_ROLEGROUPUC	0x27
#define MOD_ROLEUSERC	0x28
#define MOD_ROLEUSERUC	0x29

#define BUFSIZE		4096

u_int8_t mode;

static void usage(void);
void set_existence_of_newsgroup(server_cb_inf *, char *);
void exit_if_newsgroup_exists(server_cb_inf *, char *);
void exit_if_newsgroup_not_exists(server_cb_inf *, char *);
void set_existence_of_user(server_cb_inf *, char *);
void exit_if_user_exists(server_cb_inf *, char *);
void exit_if_user_not_exists(server_cb_inf *, char *);
void set_existence_of_role(server_cb_inf *, char *);
void exit_if_role_exists(server_cb_inf *, char *);
void exit_if_role_not_exists(server_cb_inf *, char *);

static void
usage()
{
	fprintf(stderr, "usage: " THIS_TOOLNAME " <command> [parameters]\n"
			"*** Newsgroup Administration:\n"
			" <listgroups>\n"
			" <addgroup | modgroup> <newsgroup> <posting-allowed-flag (y/n)>\n"
			" <delgroup> <newsgroup>\n"
			"*** User Administration:\n"
			" <listusers>\n"
			" <adduser> <username> [<password>]\n"
			" <deluser> <username>\n"
			"*** ACL (Access Control List) Administration:\n"
			" <listacl>\n"
			" <addacluser | delacluser> <username> <newsgroup>\n"
			" <addaclrole | delaclrole> <role>\n"
			" <rolegroupconnect | rolegroupdisconnect> <role> <newsgroup>\n"
			" <roleuserconnect | roleuserdisconnect> <role> <username>\n"
			);
	exit(ERR_EXIT);
}

/* ** NEWSGROUP Existence ** */

void
set_existence_of_newsgroup(server_cb_inf *inf, char *group)
{
	inf->servinf->found_group = 0;
	inf->servinf->chkname = group;
	db_check_newsgroup_existence(inf);
}

void
exit_if_newsgroup_exists(server_cb_inf *inf, char *group)
{
	set_existence_of_newsgroup(inf, group);
	if (inf->servinf->found_group) {
		fprintf(stderr, "Newsgroup %s already exists.\n", group);
		exit(ERR_EXIT);
	} else
		printf("Newsgroup %s does not exist. Creating new group.\n", group);
}

void
exit_if_newsgroup_not_exists(server_cb_inf *inf, char *group)
{
	set_existence_of_newsgroup(inf, group);
	if (!inf->servinf->found_group) {
		fprintf(stderr, "Newsgroup %s does not exists.\n", group);
		exit(ERR_EXIT);
	} else
		printf("Newsgroup %s exists: okay.\n", group);
}

/* ** USER Existence ** */
void
set_existence_of_user(server_cb_inf *inf, char *user)
{
	inf->servinf->found_group = 0;
	inf->servinf->chkname = user;
	db_check_user_existence(inf);
}

void
exit_if_user_exists(server_cb_inf *inf, char *user)
{
	set_existence_of_user(inf, user);
	if (inf->servinf->found_group) {
		fprintf(stderr, "User %s does already exists.\n", user);
		exit(ERR_EXIT);
	} else
		printf("User %s does currently not exist: okay.\n", user);
}

void
exit_if_user_not_exists(server_cb_inf *inf, char *user)
{
	set_existence_of_user(inf, user);
	if (!inf->servinf->found_group) {
		fprintf(stderr, "User %s does not exists.\n", user);
		exit(ERR_EXIT);
	} else
		printf("User %s exists: okay.\n", user);
}

/* ** ROLE Existence ** */

void
set_existence_of_role(server_cb_inf *inf, char *role)
{
	inf->servinf->found_group = 0;
	inf->servinf->chkname = role;
	db_check_role_existence(inf);
}

void
exit_if_role_exists(server_cb_inf *inf, char *role)
{
	set_existence_of_role(inf, role);
	if (inf->servinf->found_group) {
		fprintf(stderr, "Role %s already exists.\n", role);
		exit(ERR_EXIT);
	} else
		printf("Role %s does not exists: okay.\n", role);
}

void
exit_if_role_not_exists(server_cb_inf *inf, char *role)
{
	set_existence_of_role(inf, role);
	if (!inf->servinf->found_group) {
		fprintf(stderr, "Role %s does not exists.\n", role);
		exit(ERR_EXIT);
	} else
		printf("Role %s exists: okay.\n", role);
}

int
main(int argc, char *argv[])
{
	char *pass, *pass_hash;
	int len_argvs;
	server_cb_inf *inf;
	
	/* detect the database engine */
	basic_setup_admtool();
	
	if (!(inf = (server_cb_inf *) calloc(1, sizeof(server_cb_inf)))) {
		fprintf(stderr, "Unable to allocate memory");
		exit(ERR_EXIT);
	}
	if (!(inf->servinf = (serverinfo_t *) calloc(1, sizeof(serverinfo_t)))) {
		fprintf(stderr, "Unable to allocate memory");
		exit(ERR_EXIT);
	}

	if (argc < 3)
		if (! (argc == 2 && ((strcmp(argv[1], "listgroups") == 0)
			|| (strcmp(argv[1], "listusers") == 0)
			|| (strcmp(argv[1], "listacl") == 0))))
			usage();
	
	db_open_connection(inf);

	if (strcmp(argv[1], "addgroup") == 0)
		mode = MOD_NGCREATE;
	else if (strcmp(argv[1], "delgroup") == 0)
		mode = MOD_NGDELETE;
	else if (strcmp(argv[1], "modgroup") == 0)
		mode = MOD_NGMODIFY;
	else if (strcmp(argv[1], "listgroups") == 0)
		mode = MOD_NGLIST;
	else if (strcmp(argv[1], "adduser") == 0)
		mode = MOD_ADDUSER;
	else if (strcmp(argv[1], "deluser") == 0)
		mode = MOD_DELUSER;
	else if (strcmp(argv[1], "listusers") == 0)
		mode = MOD_LISTUSER;
	else if (strcmp(argv[1], "listacl") == 0)
		mode = MOD_LISTACL;
	else if (strcmp(argv[1], "delacluser") == 0)
		mode = MOD_DELACLUSER;
	else if (strcmp(argv[1], "addacluser") == 0)
		mode = MOD_ADDACLUSER;
	else if (strcmp(argv[1], "addaclrole") == 0)
		mode = MOD_ADDACLROLE;
	else if (strcmp(argv[1], "delaclrole") == 0)
		mode = MOD_DELACLROLE;
	else if (strcmp(argv[1], "rolegroupconnect") == 0)
		mode = MOD_ROLEGROUPC;
	else if (strcmp(argv[1], "rolegroupdisconnect") == 0)
		mode = MOD_ROLEGROUPUC;
	else if (strcmp(argv[1], "roleuserconnect") == 0)
		mode = MOD_ROLEUSERC;
	else if (strcmp(argv[1], "roleuserdisconnect") == 0)
		mode = MOD_ROLEUSERUC;
	else {
		fprintf(stderr, "Invalid mode: %s'.\n", argv[1]);
		exit(ERR_EXIT);
	}
	
	/* check lengths */
	switch (mode) {
	case MOD_LISTACL:
	case MOD_NGLIST:
	case MOD_LISTUSER:
		/* only 1 parameter (= the command) is needed => no check needed */
		break;
	default:
		/* again: to make sure we have a 2nd command parameter for those
		 * commands who need that */
		switch (mode) {
		case MOD_NGCREATE:
		case MOD_NGMODIFY:
		case MOD_ADDACLUSER:
		case MOD_DELACLUSER:
		case MOD_ROLEGROUPC:
		case MOD_ROLEGROUPUC:
		case MOD_ROLEUSERC:
		case MOD_ROLEUSERUC:
			if (argc < 4) {
				fprintf(stderr, "Need more parameters.\n\n");
				usage();
				/* NOTREACHED */
			}
			break;
		}
	
		/* a 2nd (or 3rd) parameter is needed */
		len_argvs = strlen(argv[2]);
		if (argc > 3)
			len_argvs += strlen(argv[3]);
		if (argc > 4)
			len_argvs += strlen(argv[4]);
		
		if (len_argvs > (BUFSIZE - 0xff)) {
			fprintf(stderr, "Error: newsgroup name or username or both name is/are too long.\n");
			exit(ERR_EXIT);
		}
	}	
	
	switch (mode) {
	case MOD_NGCREATE:
		if (argc < 4)
			usage();
		/* first check if the group already exists */
		exit_if_newsgroup_exists(inf, argv[2]);
		/* Race condition: theoretically, a newsgroup could be
		 * created before we now call db_create_newsgroup().
		 * However, this can be tolerated and would not have
		 * much consequences. Also: two admins would need to do
		 * that simultaneously!
		 */
		/* now insert the new group */
		db_create_newsgroup(inf, argv[2], argv[3][0]);
		break;
	case MOD_NGDELETE:
		exit_if_newsgroup_not_exists(inf, argv[2]);
		db_delete_newsgroup(inf, argv[2]);
		break;
	case MOD_NGMODIFY:
		if (argc < 4)
			usage();
		exit_if_newsgroup_not_exists(inf, argv[2]);
		db_modify_newsgroup(inf, argv[2], argv[3][0]);
		break;
	case MOD_NGLIST:
		db_list(inf, CMDTYP_LIST, NULL);
		break;
	case MOD_ADDUSER:
		/* If the password was given as a parameter: use it */
		if (argc >= 4) {
			pass = argv[3];
		}
		/* If the password was not given as a parameter: read it */
		else {
#ifndef __WIN32__ /* why the hell does win32 not support this func??? */
			pass = getpass("Enter new password for this user (max. 100 chars):");
#else
			if (!(pass = (char *)calloc(101, sizeof(char)))) {
				perror("calloc");
				exit(ERR_EXIT);
			}
			printf("Enter new password for this user (max. 100 chars): ");
			fgets(pass, 100, stdin);
			pass[strlen(pass) - 1] = '\0'; /* remove \n */
#endif
		}
		
		if (strlen(argv[2]) > 100) {
			printf("username too long (max. 100 chars).\n");
			exit(ERR_EXIT);
		}
		if (strlen(pass) < 8) {
			printf("password too short (min. 8 chars).\n");
			exit(ERR_EXIT);
		}
		/* this check is at least needed for the *nix getpass() version above */
		if (strlen(pass) > 100) {
			printf("password too long (max. 100 chars).\n");
			exit(ERR_EXIT);
		}
		
		exit_if_user_exists(inf, argv[2]);
		pass_hash = get_sha256_hash_from_str(argv[2], pass);
		if (!pass_hash) {
			/* bzero the password to clean up the mem */
			memset(pass, 0x0, strlen(pass));
			printf("error in mhash utilization");
			exit(ERR_EXIT);
		}
		/* Race condition: theoretically, the user could be
		 * created before we now call db_add_user().
		 * However, this can be tolerated and would not have
		 * much consequences. Also: two admins would need to do
		 * that simultaneously!
		 */
		db_add_user(inf, argv[2], pass_hash);
		
		/* bzero the pass just to clean up the mem */
		memset(pass, 0x0, strlen(pass));
		memset(pass_hash, 0x0, strlen(pass_hash));
		break;
	case MOD_DELUSER:
		exit_if_user_not_exists(inf, argv[2]);
		db_del_user(inf, argv[2]);
		break;
	case MOD_LISTUSER:
		db_list_users(inf);
		break;
	case MOD_LISTACL:
		db_list_acl_tables(inf);
		break;
	case MOD_ADDACLUSER:
		exit_if_user_not_exists(inf, argv[2]);
		exit_if_newsgroup_not_exists(inf, argv[3]);
		db_acl_add_user(inf, argv[2], argv[3]);
		break;
	case MOD_DELACLUSER:
		exit_if_user_not_exists(inf, argv[2]);
		exit_if_newsgroup_not_exists(inf, argv[3]);
		db_acl_del_user(inf, argv[2], argv[3]);
		break;
	case MOD_ADDACLROLE:
		exit_if_role_exists(inf, argv[2]);
		db_acl_add_role(inf, argv[2]);
		break;
	case MOD_DELACLROLE:
		exit_if_role_not_exists(inf, argv[2]);
		db_acl_del_role(inf, argv[2]);
		break;
	case MOD_ROLEGROUPC:
		exit_if_role_not_exists(inf, argv[2]);
		exit_if_newsgroup_not_exists(inf, argv[3]);
		db_acl_role_connect_group(inf, argv[2], argv[3]);
		break;
	case MOD_ROLEGROUPUC:
		exit_if_role_not_exists(inf, argv[2]);
		exit_if_newsgroup_not_exists(inf, argv[3]);
		db_acl_role_disconnect_group(inf, argv[2], argv[3]);
		break;
	case MOD_ROLEUSERC:
		exit_if_role_not_exists(inf, argv[2]);
		exit_if_user_not_exists(inf, argv[3]);
		db_acl_role_connect_user(inf, argv[2], argv[3]);
		break;
	case MOD_ROLEUSERUC:
		exit_if_role_not_exists(inf, argv[2]);
		exit_if_user_not_exists(inf, argv[3]);
		db_acl_role_disconnect_user(inf, argv[2], argv[3]);
		break;
	default:
		fprintf(stderr, "unknown command implementation.\n");
		exit(ERR_EXIT);
	};

	db_close_connection(inf);
	printf("done.\n");
	return 0;
}

