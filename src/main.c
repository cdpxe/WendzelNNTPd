/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2021 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

#define RCVBUFSIZ	512 /* including CR-LF */ + 1 /* \0 */

char lowercase[256];
extern int size_sockinfo_t;
extern int daemon_mode;
extern short global_mode;

/* need this global for server.c */
struct sockaddr_in sa;
struct sockaddr_in6 sa6;

extern fd_set fds;
extern sockinfo_t *sockinfo;
extern int peak;

static void usage_(void);
static void welcome_(int ready);

static void
usage_(void)
{
	fprintf(stderr, "WendzelNNTPd accepts the following *exclusive* parameters:\n"
					"-d     run WendzelNNTPd in daemon mode\n"
					"-h     print help\n"
					"-v     print the version and exit\n");
}

static void
welcome_(int ready)
{
	PROMPT("version " WELCOMEVERSION);
	if (ready == 1) {
		PROMPT("service is ready.");
	}
}

int
main(int argc, char *argv[])
{
	int i;
	int connfd;
	socklen_t salen = sizeof(struct sockaddr_in);
	socklen_t sa6len = sizeof(struct sockaddr_in6);
#ifndef __WIN32__
	pid_t pid;
#endif
	pthread_t th1;
	sockinfo_t *sockinf;

	if (argc > 1) { /* non-daemon mode parameters are checked before startup */
		if (strncmp(argv[1], "-v", 2) == 0) { /* just display the version */
			welcome_(0);
			exit(1);
		} else if (!strncmp(argv[1], "-d", 2) == 0) { /* display help if not daemon mode requested here */
			/* this also catches -h :-) */
			usage_();
			exit(1);
		}
	}

/* BASIC INITIALIZATION */
	bzero(&sa, sizeof(sa));
	bzero(&sa6, sizeof(sa6));

	basic_setup_server();

#ifndef __WIN32__
	/* signal handling */
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal");
		exit(ERR_EXIT);
	}
	if (signal(SIGTERM, sig_handler) == SIG_ERR) {
		perror("signal");
		exit(ERR_EXIT);
	}
#endif
	/* 41 - 5a = upper case -> + 0x20 = lower case */
	for (i = 0; i < 256; i++) {
		if (i >= 0x41 && i <= 0x5a)
			lowercase[i] = i + 0x20;
		else
			lowercase[i] = i;
	}

/* GLOBAL INIT */
	/* simple but stable and useful ;-) */
	if (argc > 1) {
#ifndef __WIN32__ /* lol */
		if (strncmp(argv[1], "-d", 2) == 0) { /* daemon mode */
			if ((pid = fork()) < 0) {
         			fprintf(stderr, "cannot fork().\n");
        	 		return ERR_EXIT;
      			} else if (pid) { /* parent */
				return OK_EXIT;
	      		}
			daemon_mode = 1;
	      		setsid();
	      		chdir("/");
	      		umask(077);
	      		DO_SYSL("----WendzelNNTPd is running in daemon mode now----")
		}
#endif
	}

#ifndef __WIN32__ /* some *nix security */
	umask(077);
#endif

	check_db();
	welcome_(1);

/* SOCKET MAINLOOP */
	/* from now on: db abstraction in thread mode.
	 * this means no exit(ERR_EXIT) is used. instead only
	 * threads are killed on err.
	 */
	global_mode = MODE_THREAD;

	do {
		for (i = 0; i < size_sockinfo_t; i++) {
			FD_SET((sockinfo+i)->sockfd, &fds);
		}

		if (select(peak + 1, &fds, NULL, NULL, NULL)== -1) {
			if (errno == EINTR) {
				continue;
			} else {
#ifdef DEBUG
				if (errno == EFAULT) printf("EFAULT\n");
				else if (errno == EBADF) printf("EBADF\n");
				else if (errno == EINVAL) printf("EINVAL\n");
#endif
				perror("select");
				sig_handler(0);
			}
		}

		for (i = 0; i < size_sockinfo_t; i++) {
			if (FD_ISSET((sockinfo + i)->sockfd, &fds)) {
				onxxdebug("accepting tcp connection.\n");
				if ((connfd = accept((sockinfo+i)->sockfd,
					SWITCHIP(i, (struct sockaddr *)&sa, (struct sockaddr *)&sa6),
					SWITCHIP(i, &salen, &sa6len))) < 0) {
						perror("accept");
				} else {
					/* Logging */
					char conn_logstr[CONN_LOGSTR_LEN] = { '\0' };
					char new_conn_prefix[] = "Accepted connection from ";

					strncpy(conn_logstr, new_conn_prefix, strlen(new_conn_prefix));
#ifdef __WIN32__
					{
					 /* Windows helper code, only IPv4-ready */
					 char *w32_helper_ip_ptr;
					 w32_helper_ip_ptr = inet_ntoa(sa.sin_addr);
					 strncpy((sockinfo+i)->ip, w32_helper_ip_ptr, sizeof((sockinfo+i)->ip));
					}
					if (!strlen((sockinfo+i)->ip))
#else
					if (inet_ntop((sockinfo+i)->family,
						SWITCHIP(i, (void *) &sa.sin_addr, (void *) &sa6.sin6_addr),
						(sockinfo+i)->ip, IPv6ADDRLEN) == NULL)
#endif
					{
						DO_SYSL("Unable to do inet_ntop() or inet_ntoa(). "
							"Trying to serve the client nevertheless ...")
					}
					strncpy(conn_logstr + strlen(new_conn_prefix), (sockinfo+i)->ip,
						strlen((sockinfo+i)->ip));

					DO_SYSL(conn_logstr)

					/* After Logging: The real stuff */
					CALLOC(sockinf, (sockinfo_t *), 1, sizeof(sockinfo_t))
					sockinf->sockfd = connfd;
					sockinf->family = SWITCHIP(i, FAM_4, FAM_6);
					memcpy(&sockinf->sa, &sa, sizeof(sa));
					memcpy(&sockinf->sa6, &sa6, sizeof(sa6));

					strncpy(sockinf->ip, (sockinfo+i)->ip, strlen((sockinfo+i)->ip));
					bzero((sockinfo+i)->ip, strlen((sockinfo+i)->ip));

					if (pthread_create(&th1, NULL, &do_server, sockinf) != 0) {
						DO_SYSL("pthread_create() returned != 0")
						free(sockinf);
					}

					/* don't free() sockinf here since the thread does it itself */
				}
			}
		}
	} while (1);

	return 0;
}
