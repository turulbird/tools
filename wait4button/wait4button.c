/*
 * wait4button based on evremote.c
 *
 * (c) 2014 j00zek
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include <semaphore.h>
#include <pthread.h>

#include "global.h"
#include "remotes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned int wait4KeyCode = 0;
static unsigned int vCurrentCode = 0;

int processSimple(Context_r_t *context_r, Context_t *context, int argc, char *argv[])
{

	if (context_r->br->Init)
	{
		context->fd = context_r->br->Init(context, argc, argv);
	}
	else
	{
		fprintf(stderr, "[wait4button] driver does not support init function\n");
		exit(1);
	}
	if (context->fd < 0)
	{
		fprintf(stderr, "[wait4button] error in device initialization\n");
		exit(1);
	}
	while (true)
	{
		// wait for new command
		if (context_r->br->Read)
		{
			vCurrentCode = context_r->br->Read(context);
		}
		if (vCurrentCode <= 0)
		{
			continue;
		}
		if ((wait4KeyCode > 0) && (wait4KeyCode != vCurrentCode))
		{
			continue;
		}
		printf("[wait4button] Read KeyCode:%i\n", vCurrentCode);
		break;
	}
	context_r->br->Shutdown(context);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ignoreSIGPIPE()
{
	struct sigaction vAction;

	vAction.sa_handler = SIG_IGN;

	sigemptyset(&vAction.sa_mask);
	vAction.sa_flags = 0;
	sigaction(SIGPIPE, &vAction, (struct sigaction *)NULL);
}

int main(int argc, char *argv[])
{
	eBoxType vBoxType = LircdName;
	Context_t context;
	Context_r_t context_r;

	/* Dagobert: if tuxtxt closes the socket while
	 * we are writing a sigpipe occurence which kills
	 * wait4button. so lets ignore it ...
	 */
	ignoreSIGPIPE();

	if (argc >= 2
	&& (!strncmp(argv[1], "-h", 2) || !strncmp(argv[1], "--help", 6)))
	{
		printf("USAGE:\n");
		printf("wait4button [<button code>] returns code of pressed button on RC.\n");
		printf("Parameter(s) description:\n");
		printf("<button code> - defines specific button code to wait for\n");
		return 0;
	}
	selectRemote(&context, vBoxType);
	if (argc >= 2)
	{
		wait4KeyCode = atoi(argv[1]);
		printf("[wait4button] Waiting for %i on %s\n", wait4KeyCode, ((RemoteControl_t *)context.r)->Name);
	}
	else
	{
		printf("Using %s\n", context.r->Name);
	}
	if (context.r->RemoteControl != NULL)
	{
		printf("[wait4button] RemoteControl Map:\n");
		printKeyMap(context.r->RemoteControl);
	}
	if (context.r->Frontpanel != NULL)
	{
		printf("[wait4button] Frontpanel Map:\n");
		printKeyMap(context.r->Frontpanel);
	}
	processSimple(&context_r, &context, argc, argv);
	return vCurrentCode;
}
// vim:ts=4

