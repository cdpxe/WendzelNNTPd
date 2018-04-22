/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2007-2010 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

/* This file includs plain database connectivity checks done on startup.
 * The database abstraction interface is NOT used in this case since the
 * db abstr. interface was optized to work with the server threads and
 * depends (because of that) on 'inf' pointers.
 */

#include "main.h"

extern unsigned short dbase;	/* from config.y */
extern short be_verbose;	

/* the DB connection should work fine if we'll receive no error here ... */
void
check_db(void)
{
	server_cb_inf inf;
	
	if (!(inf.servinf = (serverinfo_t *) calloc(1, sizeof(serverinfo_t)))) {
		fprintf(stderr, "Unable to allocate memory");
		exit(ERR_EXIT);
	}
	
	/* Make sure we have a database engine */
	switch (dbase) {
	case DBASE_NONE:
		fprintf(stderr, "There is no database specified in the configuration!\n");
		fprintf(stderr, "Please write at least 'database-engine sqlite3' in \n");
		fprintf(stderr, "your config file.\n");
		DO_SYSL("Exiting. No database engine specified.");
		exit(ERR_EXIT);
		/*NOTREACHED*/
		break;
	case DBASE_SQLITE3:
	case DBASE_MYSQL:
		db_open_connection(&inf);
		db_close_connection(&inf);
		break;
	default:
		fprintf(stderr, "DB engine not supported. exiting.");
		DO_SYSL("DB engine not supported. exiting.");
		exit(ERR_EXIT);
		break;
	}
	if (be_verbose)
		printf("Database check %s: Success.\n",
			(dbase == DBASE_SQLITE3 ? "Sqlite3" : "MySQL"));
}


