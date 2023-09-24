/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2009-2010 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

/* Implement Access Control Lists using the database backend */

#include "main.h"

short
acl_check_user_group(server_cb_inf *inf, char *user, char *group)
{
	if (user == NULL || group == NULL) {
#ifdef DEBUG
		fprintf(stderr, "acl_check_user_group(): user=%p, group=%p\n", user, group);
#endif
		DO_SYSL("acl_check_user_group(): Internal error: user==NULL OR group==NULL")
		return FALSE;
	}
	return db_acl_check_user_group(inf, user, group);
}

