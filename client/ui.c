//-----------------------------------------------------------------------------
// Copyright (C) 2009 Michael Gernoth <michael at gernoth.net>
// Copyright (C) 2010 iZsh <izsh at fail0verflow.com>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// UI utilities
//-----------------------------------------------------------------------------

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <pthread.h>

#include "ui.h"

extern pthread_mutex_t print_lock;

static char *logfilename = "proxmark3.log";
static bool flushAfterWrite = false;

void PrintAndLog(char *fmt, ...)
{
	char *saved_line;
	int saved_point;
	va_list argptr, argptr2;
	// TODO: stash these elsewhere
	static FILE *logfile = NULL;
	static int logging=1;

	// lock this section to avoid interlacing prints from different threads
	pthread_mutex_lock(&print_lock);
  
	if (logging && !logfile) {
		logfile=fopen(logfilename, "a");
		if (!logfile) {
			fprintf(stderr, "Can't open logfile, logging disabled!\n");
			logging=0;
		}
	}

#ifdef RL_STATE_READCMD
	// We are using GNU readline.
	int need_hack = (rl_readline_state & RL_STATE_READCMD) > 0;

	if (need_hack) {
		saved_point = rl_point;
		saved_line = rl_copy_text(0, rl_end);
		rl_save_prompt();
		rl_replace_line("", 0);
		rl_redisplay();
	}
#else
	// We are using libedit (OSX), which doesn't support this flag.
#endif
	
	va_start(argptr, fmt);
	va_copy(argptr2, argptr);
	vprintf(fmt, argptr);
	printf("          "); // cleaning prompt
	va_end(argptr);
	printf("\n");

	// This needs to be wrapped in ifdefs, as this if optimisation is disabled,
	// this block won't be removed, and it'll fail at the linker.
#ifdef RL_STATE_READCMD
	if (need_hack) {
		rl_restore_prompt();
		rl_replace_line(saved_line, 0);
		rl_point = saved_point;
		rl_redisplay();
		free(saved_line);
	}
#endif
	
	if (logging && logfile) {
		vfprintf(logfile, fmt, argptr2);
		fprintf(logfile,"\n");
		fflush(logfile);
	}
	va_end(argptr2);

	//release lock
	pthread_mutex_unlock(&print_lock);  
}

void SetLogFilename(char *fn)
{
  logfilename = fn;
}

void SetFlushAfterWrite(bool flush_after_write) {
	flushAfterWrite = flush_after_write;
}
