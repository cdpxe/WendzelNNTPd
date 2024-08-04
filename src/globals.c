/*
 * WendzelNNTPd is distributed under the following license:
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

/* External global variables/functions used by both, main.c and
 * cdpnntpadm.c, but which originally belonged to main.c. */
#include "main.h"

int daemon_mode = 0;
int size_sockinfo_t = 0;
short global_mode = MODE_PROCESS; /* don't change default value */

/* sig_handler() for win32, too, since I sometimes call it in code and
 * not only from outside.
 */
void
sig_handler(int signr)
{
#ifdef USE_TLS
	tls_global_close();
#endif

	DO_SYSL("----clean exit after signal.----")
	exit(ERR_EXIT);
}
