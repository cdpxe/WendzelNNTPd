/*
 * WendzelNNTPd is distributed under the following license:
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
 
/* database.c */

#include "main.h"
#include <dirent.h>
#include <time.h>

extern char progerr503[];

int get_openfilelen(FILE *);
char *extract_pure_message_id(char *);

int
get_openfilelen(FILE *fp)
{
	int len;
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	return len;
}

/* check if the permissions of the file are acceptable for this service and
 * if the file we try to open is a symlink (what could be part of an attack!)
 */
int
chk_file_sec(char *filename)
{
#ifndef __WIN32__
	/* based on the AstroCam source code I wrote in the past
	 * (see sf.net/projects/astrocam for details)
	 */
	int file;
	struct stat s;
	
	if ((file = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "chk_file_sec: file does not exists");
		perror(filename);
		return -1;
	}
	
	if (fstat(file, &s) == -1) {
		fprintf(stderr, "fstat(%s) returned -1\n", filename);
		return -1;
	}
	close(file);
	
	if (S_ISLNK(s.st_mode) || (S_IWOTH & s.st_mode) || (S_IWGRP & s.st_mode)) {
		fprintf(stderr, "File mode of %s has changed or file is a symlink!\n",
			filename);
		return -1;
	}
	
	if(s.st_uid != 0 && s.st_uid != getuid() && s.st_uid != geteuid()) {
		fprintf(stderr, "Owner of %s is neither zero (root) nor my (e)uid!\n",
			filename);
		return -1;
	}
#endif
	return 0;
}

static int
DB_chk_if_msgid_exists_cb(void *boolval, int argc, char **argv, char **col)
{
	*((int *)boolval) = 1;
	return 0;
}

int
DB_chk_if_msgid_exists(char *newsgroup, char *msgid, server_cb_inf *inf)
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
	}
	snprintf(sql_cmd, len - 1, "select * from ngposts where ng = '%s' and msgid = '%s';", newsgroup, msgid);
	
	if (sqlite3_exec(inf->servinf->db, sql_cmd, DB_chk_if_msgid_exists_cb, &boolval, &inf->servinf->sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", inf->servinf->sqlite_err_msg);
		sqlite3_free(inf->servinf->sqlite_err_msg);
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("Unable to exec on database.")
		kill_thread(inf);
	}
	free(sql_cmd);
	return boolval;
}



static int
DB_get_high_cb(void *cur_high, int argc, char **argv, char **col)
{
	*((int *)cur_high) = atol(argv[3]);
	return 0;
}

u_int32_t
DB_get_high_value(char *newsgroup, server_cb_inf *inf)
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
	}
	snprintf(sql_cmd, len - 1, "select * from newsgroups where name = '%s';", newsgroup);
	
	if (sqlite3_exec(inf->servinf->db, sql_cmd, DB_get_high_cb, &cur_high, &inf->servinf->sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "%s\n", inf->servinf->sqlite_err_msg);
		sqlite3_free(inf->servinf->sqlite_err_msg);
		ToSend(progerr503, strlen(progerr503), inf);
		DO_SYSL("Unable to exec on database.")
		kill_thread(inf);
	}
	free(sql_cmd);
	return cur_high;
}

/* create a new message id and return it */


/* TODO: lock the file and/or use pthread_locking */
char *
get_uniqnum(void)
{
	char *buf;
	FILE *fp;
	long long id;
	
	if (!(buf = (char *) calloc(MAX_IDNUM_LEN + 1, sizeof(char)))) {
		DO_SYSL("Not enough memory!")
		return NULL;
	}
	
	if (chk_file_sec(MSGID_FILE) != 0) {
		DO_SYSL("File permissions of " MSGID_FILE " are "
			"insecure and/or file is a symlink!")
		/* do not exit here, only log it ... */
	}
	if (!(fp=fopen(MSGID_FILE, "rb+"))) {
		id = 0;
		fp = fopen(MSGID_FILE, "wb+");
		if(!fp) {
            		DO_SYSL("Unable to create file " MSGID_FILE)
			return NULL;
		}
	} else {
		if(!fread(&id, sizeof(long long), 1, fp)) {
			fclose(fp);
            		DO_SYSL("Unable to read file " MSGID_FILE)
			return NULL;
		}
		fseek(fp, 0, SEEK_SET);
	}
	
	/* setup the next id */
	id++;
	/* TODO: add a timestamp to the message-id! */
	snprintf(buf, MAX_IDNUM_LEN, "<cdp%lld@", id);
	
	if(!fwrite(&id, sizeof(long long), 1, fp)) {
		fclose(fp);
        	DO_SYSL("Unable to write to file " MSGID_FILE)
		return NULL;
	}
	fclose(fp);
	return buf;
}

char *
filebackend_retrbody(char *message_id_in)
{
	int file_len;
	char *body;
	char *filename;
	FILE *fp;
	char *message_id = message_id_in;
	message_id++;
	message_id[strlen(message_id)-1] = '\0';

	
	if (!(filename = (char *) calloc(strlen(SPOOLFOLDER) + 1 + strlen(message_id) + 1, sizeof(char)))) {
		DO_SYSL("Not enough memory!")
		free(message_id);
		return NULL;
	}
	snprintf(filename, strlen(SPOOLFOLDER) + 1 + strlen(message_id) + 1, SPOOLFOLDER "/%s", message_id);
	
	if (chk_file_sec(filename) != 0) {
		DO_SYSL("will not open requested usenet body file due to insecurity")
		fprintf(stderr, "filebackend_retrbody:chk_file_sec:error\n");
		return NULL;
	}
	fp = fopen(filename, "r");
	free(filename);
	if (!fp) {
		DO_SYSL("Unable to open requested body file")
		fprintf(stderr, "filebackend_retrbody:fopen:error\n");
		return NULL;
	}
	
	file_len = get_openfilelen(fp);
	
	if (!(body = (char *) calloc(file_len + 1, sizeof(char)))) {
		DO_SYSL("Not enough memory!")
		return NULL;
	}
	if (!fread(body, 1, file_len, fp)) {
       perror("fread");
		DO_SYSL("Unable to read usenet backend file")
		free(body);
		return NULL;
	}
	fclose(fp);
	return body;
}

/* This function saves the message body in the file system and also removes double quotes
 * by replacing '' with '.
 */

short int
filebackend_savebody(char *message_id_in, char *body)
{
	char *filename;
	FILE *fp;
	char *message_id;
	char *copy = strdup(message_id_in);
	unsigned int i, j;
	int double_quotes;
	char *non_sql_body;
    
	message_id = copy + 1;
	message_id[strlen(message_id)-1] = '\0';
	
	if (!(filename = (char *) calloc(strlen(SPOOLFOLDER) + 1 + strlen(message_id) + 1, sizeof(char)))) {
		DO_SYSL("Not enough memory!")
		free(copy);
		return -1;
	}
	snprintf(filename, strlen(SPOOLFOLDER) + 1 + strlen(message_id) + 1, SPOOLFOLDER "/%s", message_id);
	free(copy);
	/* try to remove the file first (could be a symlink!), if unsuccessful, don't care since file didn't exist */
	unlink(filename);
	fp = fopen(filename, "w+");
	if (!fp) {
		DO_SYSL("Unable to open requested body file")
		perror(filename);
		free(filename);
		return -1;
	}
	free(filename);
	
	/* Now make a copy of the body without doubled quotes ('' should be ' after this task) */
	/* -> 1st: count the # of ''s */
	for (i = 0, double_quotes = 0; i < strlen(body);) {
		if (body[i] == '\'' && body[i+1] == '\'') {
			i+=2;
			double_quotes++;
		} else {
			i++;
		}
	}
	/* -> 2nd: copy the buffer but replace all ''s with 's */
	if ((non_sql_body = (char *) calloc(strlen(body) - double_quotes + 1, sizeof(char)))) {
		i = 0;
		j = 0;
		while (i < strlen(body)) {
			non_sql_body[j] = body[i];
			if (body[i] == '\'' && body[i+1] == '\'') {
				i+=2;
			} else {
				i++;
			}
			j++;
		}
		fwrite(non_sql_body, strlen(non_sql_body), 1, fp);
		free(non_sql_body);
	} else {
		DO_SYSL("Not enough memory to calloc mem for the msg body copy")
	}
	fclose(fp);
	
	return 0;
}


