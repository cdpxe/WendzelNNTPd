/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2015 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
 * http://www.wendzel.de
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
 
/* This file implements the plain filesystem database functionality.
 * WendzelNNTPd stores all content in the filesystem instead of the
 * using the database for that. This means the databases are only used
 * to store meta information. I think the combination of both is a
 * perfect performance oriented solution.
 * It has also the effect of shrinking the database specific ares of
 * the WendzelNNTPd code.
 */

#include "main.h"
#include <dirent.h>
#include <time.h>

extern char progerr503[];

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
		fprintf(stderr, "chk_file_sec: file does not exists: ");
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
		DO_SYSL("File " MSGID_FILE " not found (not a problem,"
			" it will be created for you right now and "
			"this message will not appear next time) "
			"-OR- the file's permissions are insecure "
			"(e.g. file is a symlink).")
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
		DO_SYSL("will not open requested usenet body file (it does not exist or "
			"has insecure permissions, e.g. is a symlink)")
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
	{
		size_t read_bytes;
	
		if (!(read_bytes = fread(body, 1, file_len, fp))) {
			perror("fread");
			DO_SYSL("Unable to read usenet backend file")
			free(body);
			return NULL;
		}
	}
	fclose(fp);
	return body;
}

/* This function saves the message body in the file system.
 */

short int
filebackend_savebody(char *message_id_in, char *body)
{
	char *filename;
	FILE *fp;
	char *message_id;
	char *copy = strdup(message_id_in);
    
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
	
	fwrite(body, strlen(body), 1, fp);
	fclose(fp);
	
	return 0;
}


