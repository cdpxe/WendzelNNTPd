/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2010 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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
#include <stdarg.h>

void
onxxdebug(char *str)
{
#ifdef XXDEBUG
	printf("%s", str);
#endif
}

void
onxxdebugm(char *str, ...)
{
#ifdef XXDEBUG
	va_list ap;
	int d, c;
	char *s;
	double f;

	va_start(ap, str);
	printf("XXDEBUG: ");
	while(*str) {
		switch(*str++) {
			case 's':
				s=va_arg(ap, char *);
				printf("%s", s);
				break;
			case 'd':
			case 'i':
				d=va_arg(ap, int);
				printf("%i", d);
				break;
			case 'c':
				c=va_arg(ap, int);
				printf("%c", c);
				break;
			case 'f':
				f=va_arg(ap, double);
				printf("%f", f);
				break;
		}
	}
	va_end(ap);
	fflush(stdout);
	fflush(stderr);
#endif
}

/* Log string and file/line/function information to logfile and syslog.
 * Do not call this function directly, use the DO_SYSL(fs) macro of
 * main.h instead.
 */
void
logstr(char *file, int line, char *func_name, char *str)
{
	FILE *fp;
	time_t ltime;
	int len;
	char *buf;
	char tbuf[40] = {'\0'};
	extern short be_verbose;
	
	ltime = time(NULL);
	strftime(tbuf, 39, "%a, %d %b %y %H:%M:%S", localtime(&ltime));
	
	len = strlen(tbuf) + strlen(file) + strlen(func_name)
		+ strlen(str) + 0x7f;
	if (!(buf = (char *)calloc(len, sizeof(char)))) {
		perror("logstr: buf = calloc()");
	}
	snprintf(buf, len - 1, "%s %s:%i:%s: %s\n", tbuf, file, line,
		func_name, str);
	/* If we run in verbose mode: output everything to stderr too. */
	if (be_verbose)
		fprintf(stderr, "%s", buf);
	
	if (chk_file_sec(LOGFILE) == INSECURE_RETURN) {
		/* do nothing here -> this could lead to a while(1)! */
#ifndef __WIN32__
		syslog(LOG_DAEMON|LOG_NEWS|LOG_NOTICE, "%s:%i: " LOGFILE
			" has insecure file permissions, e.g. is a symlink.",
			file, line);
#endif
	}
	if (!(fp = fopen(LOGFILE, "a+"))) {
		perror(LOGFILE);
	} else {
		fwrite(buf, strlen(buf), 1, fp);
		fclose(fp);
	}
	
#ifndef __WIN32__ /* *nix like systems */
	syslog(LOG_DAEMON|LOG_NEWS|LOG_NOTICE, "%s:%i: %s", file, line, str);
#endif
	
	free(buf);
}
