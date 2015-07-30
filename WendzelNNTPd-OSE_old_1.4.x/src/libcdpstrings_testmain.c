/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2006-2008 Steffen Wendzel <steffen (at) ploetner-it (dot) de>
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

#include "cdpstrings.h"
#include <stdio.h>

int
main(int argc, char *argv[])
{
	char str[] = "Hello, World!";
	char *q;
//	char posting[] = "Newsgroups: de.comp.os.unix.hackers\r\n"
			 "Subject: this is just a test subject\r\n"
			  "From: Muchad\r\n";
			  
	char posting[] = "Lines: 1\r\nnewsgroups: alt.test\r\nfrom: nobody@localhost\r\nsubject: hmm.. quite forgot it\r\n";

	
	
	printf("letztes Zeichen ist: %c\n", CDP_lastchr(str));
	
	q = CDP_return_linevalue(posting, "Newsgroups:");
	printf("Newsgroups: %s\n", q);
	free(q);
	q = CDP_return_linevalue(posting, "neWSgroups:");
	printf("neWSgroups: %s\n", q);
	free(q);
	q = CDP_return_linevalue(posting, "Subject:");
	printf("Subject: %s\n", q);
	free(q);
	q = CDP_return_linevalue(posting, "Lines:");
	printf("Lines: %s\n", q);
	free(q);
	q = NULL;
	
	return 0;
}

