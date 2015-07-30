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

/* very simple XML support. this functions output the last few postings. */

#include "main.h"

extern char *xml_outfile;

void write_html(FILE *, char *);

void
write_html(FILE *fp, char *buf)
{
	int i, len;
	
	for (i = 0, len = strlen(buf); i < len; i++) {
		switch (buf[i]) {
			case '&': fprintf(fp, "&amp;"); break;
			case '<': fprintf(fp, "&lt;"); break;
			case '>': fprintf(fp, "&gt;"); break;
			//case ' ': fprintf(fp, "&nbsp;"); break;
			case '"': fprintf(fp, "&quot;"); break;
			case '~': fprintf(fp, "&tilde"); break;
			default: fputc(buf[i], fp); break;
		}
	}
}

static int
xml_cb(void *_fp, int argc, char **argv, char **ColName)
{
	FILE *fp = (FILE *) _fp;
	char *msgid;
	
	if (argc < 5) {
		printf("argc -lt 4\n");
		return 0;
	}
	/* delete < and > from the msgid */
	msgid = argv[0];
	msgid = msgid + 1;
	msgid[strlen(msgid) - 1] = '\0';
	
	fprintf(fp, "<item>\n  <msgid>");
	write_html(fp, msgid);
	fprintf(fp, "</msgid>\n  <author>");
	write_html(fp, argv[1]);
	fprintf(fp, "</author>\n  <groups>");
	write_html(fp, argv[2]);
	fprintf(fp, "</groups>\n  <subject>");
	write_html(fp, argv[3]);
	fprintf(fp, "</subject>\n  <date>");
	write_html(fp, argv[4]);
	fprintf(fp, "</date>\n</item>");
	return 0;
}


void
write_xml()
{
	char startbuf[] = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
	char sql_cmd[] =  "SELECT DISTINCT "
					  "p.msgid,p.author,p.newsgroups,p.subject,p.date "
					  "FROM ngposts n,postings p WHERE n.msgid = p.msgid "
					  "ORDER BY n.postnum DESC LIMIT 10;";
	sqlite3 *db;
	char *sqlite_err_msg = NULL;
	FILE *fp;
	
	if (!(fp = fopen(xml_outfile, "w+"))) {
		perror(XML_OUTFILE);
		return;
	}
	if (sqlite3_open(DBFILE, &db)) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(ERR_EXIT);
	}
	
	fwrite(startbuf, strlen(startbuf), 1, fp);
	if (sqlite3_exec(db, sql_cmd, xml_cb, (void *) fp, &sqlite_err_msg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", sqlite_err_msg);
		sqlite3_free(sqlite_err_msg);
		exit(ERR_EXIT);
	}
	sqlite3_close(db);
	fclose(fp);
}

