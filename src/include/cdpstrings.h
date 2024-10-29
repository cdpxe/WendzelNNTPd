/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2006-2010 Steffen Wendzel <steffen (at) wendzel (dot) de>
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

char CDP_lastchr(char *);
char *CDP_return_linevalue(char *, char *);

#define CDP_TRUE	1
#define CDP_FALSE	0
#define CDP_ERR		-1


/* Macro definitions */

/* check if a character is printable */
#define CDP_isPrintable(c)    ((c >= 0x21 && c <= 0x7e) ? 1 : 0)
/* return '1' if the char is 0...9 || A...Z || a...z. Else return 0 */
#define CDP_isChrOrNr(c)     (((c >= 0x30 && c <= 0x39) /* number */           \
                             ||(c >= 0x41 && c <= 0x5a) /* upper case char */  \
                             ||(c >= 0x61 && c <= 0x7a) /* lower case char */  \
                              ) ? 1 : 0)
