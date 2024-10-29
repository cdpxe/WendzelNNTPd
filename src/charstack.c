/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2009-2010 Steffen Wendzel <steffen (at) wendzel (dot) de>
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

int
charstack_check_for(charstack_t *stackp, char *value)
{
	charstack_t *p;
		
	for (p = stackp; p != NULL; p = p->next) {
		if (stackp->state != STACK_EMPTY) {
			if (strcmp(p->value, value) == 0)
				return STACK_FOUND;
		}
	}
	return STACK_NOTFOUND;
}

int
charstack_push_on(charstack_t *stackp, char *value)
{
	char *value_cp;
	charstack_t *stackp_new;
	
	if (!(value_cp = (char *) calloc(strlen(value) + 1, sizeof(char)))) {
		return ERR_RETURN;
	}
	strncpy(value_cp, value, strlen(value));
	
	if (stackp->state == STACK_EMPTY) {
		stackp->value = value_cp;
		stackp->state = STACK_HASDATA;
	} else {
		if (!(stackp_new = (charstack_t *) calloc(1, sizeof(charstack_t)))) {
			free(value_cp);
			return ERR_RETURN;
		}
		stackp->next = stackp_new;
		stackp_new->value = value_cp;
		stackp_new->state = STACK_HASDATA;
	}
	return OK_RETURN;
}

void
charstack_free(charstack_t *stackp)
{
	charstack_t *nxt = NULL;
	
	while (stackp != NULL) {
		free(stackp->value);
		nxt = stackp->next;
		free(stackp);
		stackp = nxt;
	}
}


