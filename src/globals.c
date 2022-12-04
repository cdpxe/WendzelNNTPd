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

/* external global variables/functions used by both, main.c and cdpnntpadm.c but which both belong originally to main.c */
#include "main.h"

int daemon_mode = 0;
int size_sockinfo_t = 0;
short global_mode = MODE_PROCESS; /* don't change default value */

sig_atomic_t rec_sighup=0;    	//SIGHUP has been sent to process
sig_atomic_t rec_sigterm=0;      //SIGTERM (KILL) has been sent to process

/* sig_handler for win32 too since I sometimes call it in code and
 * not only from outside.
 */
void
sig_handler(int signr)
{
	DO_SYSL("----clean exit after signal.----")
	exit(ERR_EXIT);
}


void signal_action_handler (int signal_number, siginfo_t *sigstru, void *dum) {

   char line[256],sname[30]="UNKNOWN",code[30]="UNKNOWN";

   switch (signal_number)
   {
      case SIGHUP  	: rec_sighup=1;														//kill -1
                       strcpy(sname,"SIGHUP");
                       break;
      case SIGINT  	: rec_sigterm=1;														
                       strcpy(sname,"SIGINT");
							  switch(sigstru->si_code) {
							  		case 128		:	rec_sigterm=1;								//CTRL-^C
														break;
							  }
                       break;
      case SIGQUIT 	: rec_sigterm=1;														
                       strcpy(sname,"SIGQUIT");
                       break;
      case SIGILL	 	: strcpy(sname,"SIGILL");
                       break;
      case SIGTRAP 	: strcpy(sname,"SIGTRAP");
                       break;
      case SIGABRT 	: strcpy(sname,"SIGABRT");
                       break;
      case SIGBUS	 	: strcpy(sname,"SIGBUS");
                       break;
      case SIGFPE	 	: strcpy(sname,"SIGFPE");
                       break;
      case SIGKILL 	: strcpy(sname,"SIGKILL");
                       break;
      case SIGUSR1 	: strcpy(sname,"SIGUSR1");
                       break;
      case SIGSEGV 	: strcpy(sname,"SIGSEGV");
                       switch(sigstru->si_code)
                       {
                       		case SEGV_MAPERR : strcpy(code,"SEGV_MAPERR");
                                              break;
									case SEGV_ACCERR : strcpy(code,"SEGV_ACCERR");
                                        		 break;
                		  }
                       sprintf(line,"SIGNAL Received Signal Name: %s  Nr. %d  Error. %d  Code %d %s  PID %d Addr: %p",sname,signal_number,sigstru->si_errno,sigstru->si_code,code,sigstru->si_pid,sigstru->si_addr);
                       DO_SYSL(line);
                       exit(ERR_EXIT);
                       break;
      case SIGUSR2 	: strcpy(sname,"SIGUSR2");
                       break;
      case SIGPIPE 	: strcpy(sname,"SIGPIPE");
							  DO_SYSL("SIGNAL Client broken connection");				//Socket broken withour clean shutdown
                       break;
      case SIGALRM 	: strcpy(sname,"SIGALRM");
                       break;
      case SIGTERM 	: rec_sigterm=1;
                       strcpy(sname,"SIGTERM");
                       break;
      case SIGSTKFLT : strcpy(sname,"SIGSTKFLT");
                       break;
      case SIGCHLD 	: strcpy(sname,"SIGCHLD");
                       break;
      case SIGCONT 	: strcpy(sname,"SIGCONT");
                       break;
      case SIGSTOP 	: strcpy(sname,"SIGSTOP");
                       break;
      case SIGTSTP 	: strcpy(sname,"SIGTSTP");
                       break;
      case SIGTTIN 	: strcpy(sname,"SIGTTIN");
                       break;
      case SIGTTOU 	: strcpy(sname,"SIGTTOU");
                       break;
      case SIGURG 	: strcpy(sname,"SIGURG");
                       break;
      case SIGXCPU 	: strcpy(sname,"SIGXCPU");
                       break;
      case SIGXFSZ 	: strcpy(sname,"SIGXFSZ");
                       break;
      case SIGVTALRM	: strcpy(sname,"SIGVTALRM");
                       break;
      case SIGPROF	: strcpy(sname,"SIGPROF");
                       break;
      case SIGWINCH	: strcpy(sname,"SIGWINCH");
                       break;
      case SIGIO		: strcpy(sname,"SIGIO");
                       break;
      case SIGPWR		: strcpy(sname,"SIGPWR");
                       break;
      case SIGSYS		: strcpy(sname,"SIGSYS");
                       break;
   }
   sprintf(line,"SIGNAL Received Signal Name: %s  Nr. %d  Error. %d  Code %d ",sname,signal_number,sigstru->si_errno,sigstru->si_code);
   DO_SYSL(line);
}


