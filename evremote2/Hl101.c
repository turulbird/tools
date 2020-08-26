/*
 * HL101.c
 *
 * (c) 2010 duckbox project
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

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Hl101.h"

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120
};

/* Spider Box HL-101 RCU */
static tButton cButtonsSpiderboxHL101[] =
{
	// Note order is same as on Opticum 9500HD remote
	{ "POWER",       "f7", KEY_POWER },

	{ "TIMER",       "b7", KEY_PROGRAM },
	{ "U",           "d7", KEY_U },
	{ "V.FORMAT",    "e7", KEY_ZOOM },
	{ "MUTE",        "77", KEY_MUTE },

	{ "TV/SAT",      "37", KEY_AUX },
	{ "TV/RADIO",    "2f", KEY_TV2 },
	{ "FIND",        "17", KEY_SUBTITLE },
	{ "FAV",         "85", KEY_FAVORITES },

	{ "1",           "7f", KEY_1 },
	{ "2",           "bf", KEY_2 },
	{ "3",           "3f", KEY_3 },

	{ "4",           "df", KEY_4 },
	{ "5",           "5f", KEY_5 },
	{ "6",           "9f", KEY_6 },

	{ "7",           "1f", KEY_7 },
	{ "8",           "ef", KEY_8 },
	{ "9",           "6f", KEY_9 },

	{ "MENU",        "af", KEY_MENU },
	{ "0",           "ff", KEY_0 },
	{ "INFO",        "25", KEY_INFO },

	{ "EPG",         "4f", KEY_EPG },
	{ "EXIT",        "cf", KEY_EXIT },

	{ "UP/P+",       "67", KEY_UP },
	{ "LEFT/V-",     "27", KEY_LEFT },
	{ "OK/LIST",     "47", KEY_OK },
	{ "RIGHT/V+",    "c7", KEY_RIGHT },
	{ "DOWN/P-",     "a7", KEY_DOWN },

	{ "RECALL",      "0f", KEY_BACK },
	{ "RECORD",      "8f", KEY_RECORD },

	{ "PLAY",        "57", KEY_PLAY },
	{ "REWIND",      "97", KEY_REWIND },
	{ "PAUSE",       "87", KEY_PAUSE },
	{ "FASTFORWARD", "9d", KEY_FASTFORWARD },

	{ "STOP",        "d5", KEY_STOP },
	{ "SLOW",        "cd", KEY_SLOW },
	{ "PREVIOUS",    "95", KEY_PREVIOUS },
	{ "NEXT",        "55", KEY_NEXT },

	{ "ARCHIVE",     "15", KEY_FILE },
	{ "PIPSWAP",     "e5", KEY_OPTION },
	{ "PLAYMODE",    "65", KEY_W },
	{ "USB",         "a5", KEY_MEDIA },

	{ "AUDIO",       "35", KEY_AUDIO },
	{ "SAT",         "b5", KEY_SAT },
	{ "F1",          "07", KEY_F1 },
	{ "F2",          "2d", KEY_F2 },

	{ "RED",         "3d", KEY_RED },
	{ "GREEN",       "fd", KEY_GREEN },
	{ "YELLOW",      "6d", KEY_YELLOW },
	{ "BLUE",        "8d", KEY_BLUE },

//	{ "SELECT",      "47", KEY_EPG },
	{ "",            "",   KEY_NULL },
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;


static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vAddr.sun_family = AF_UNIX;
	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	vHandle = socket(AF_UNIX, SOCK_STREAM, 0);
	if (vHandle == -1)
	{
		perror("socket");
		return -1;
	}

	if (connect(vHandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		perror("connect");
		return -1;
	}
	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
	char vBuffer[128];
	char vData[10];
	const int cSize = 128;
	int vCurrentCode  = -1;

	memset(vBuffer, 0, 128);
	// wait for new command
	read(context->fd, vBuffer, cSize);

	// parse and send key event
	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';

	printf("[evremote2 hl101] key: %s -> %s\n", vData, &vBuffer[0]);
	vCurrentCode = getInternalCode(context->r->RemoteControl, vData);

	if (vCurrentCode != 0)
	{
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}
		vCurrentCode += (nextflag << 16);
	}
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{

	struct proton_ioctl_data vfd_data;
	int ioctl_fd = -1;

	vfd_data.u.icon.icon_nr = 35;
	vfd_data.u.icon.on = cOn ? 1 : 0;
	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Hl101_RC =
{
	"HL101 RemoteControl",
	Hl101,
	cButtonsSpiderboxHL101,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};

BoxRoutines_t Hl101_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

