/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2006-2008 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

#include <string.h>
#include <stdlib.h>
#ifdef __svr4__
   #include <strings.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include "cdpstrings.h"

#define PERROR_PREAMBLE "LibCDPstrings: File " __FILE__ ": "

/* return the last char of a string */
char
CDP_lastchr(char *str)
{
	return str[strlen(str) - 1];
}

/* This is a special function I implemented for my Usenet-Server. Say that the buffer
 * contains 'name: nobody\nage: 100yrs\n', then you specify a 'key' (e.g. 'name:') and
 * this function will return you the value for this line (e.g. 'nobody').
 * Update: This function is NOT case sensitive what brings more comfort with it.
 */

char *
CDP_return_linevalue(char *buf, char *key)
{
	int i, start_value;
	int len_key=strlen(key);
	int len_buf=strlen(buf);
	char *str; /* the new string this function will return */
	int len_str;
	
	for (i = 0; i < len_buf; i++) {
		/* try to save time by only checking the first char here */
		if (strncasecmp(buf+i, key, 1) == 0) {
			/* check if buf+i is still as long as key */
			if (strlen(buf + i) >= strlen(key)) {
				/* check if the name of the attribute is eq to key for the len of key */
				if (strncasecmp(buf + i, key, len_key) == 0
				   /*  next char has to be an ' ' or '\t' */
				   && (buf[i + len_key] == ' ' || buf[i + len_key] == '\t')) {
					/* we found the right attribute */
					start_value = i + len_key + 1;
					for (i = start_value; i < len_buf && buf[i] != '\r' && buf[i] != '\n'; i++)
						;
					
					len_str = i - start_value;
					if (len_str) { /* value should at least be 1 char long */
						if ((str = (char *)calloc(len_str + 1, sizeof(char))) == NULL) {
							perror(PERROR_PREAMBLE "calloc(len_str+1)");
							return NULL;
						}
						strncpy(str, buf + start_value, len_str);
						return str;
					} else {
						return NULL;
					}
				} /* wrong attribute. -> next line (end of loop does this for me) */
			} else { /* the *(buf+i) is not as long as the key. there is no value for this key. */
				return NULL;
			}
		} /* else -> next line */
		
		/* go to the next line */
		while (buf[i] != '\r' && buf[i] != '\n') {
			if (buf[i] == '\0') {
				return NULL;
			}
			i++;
		}
	}
	return NULL;
}

