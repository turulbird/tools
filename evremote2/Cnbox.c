/*
 * Cnbox.c (for Atemio 520HD & 530HD)
 *
 * (c) 2009 teamducktales
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
#include <linux/input.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Cnbox.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cCNBOXDataLen 128

typedef struct
{
} tCNBOXPrivate;

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140
};

static tButton cButtonCnbox[] =
{
	{ "MEDIA",       "07", KEY_MEDIA },
	{ "ARCHIVE",     "17", KEY_ARCHIVE },
	{ "MENU",        "4A", KEY_MENU },
	{ "RED",         "5B", KEY_RED },
	{ "GREEN",       "1F", KEY_GREEN },
	{ "YELLOW",      "08", KEY_YELLOW },
	{ "BLUE",        "03", KEY_BLUE },
	{ "EXIT",        "1D", KEY_EXIT },
	{ "BACK",        "15", KEY_HOME },
	{ "TEXT",        "03", KEY_TEXT },
	{ "EPG",         "09", KEY_EPG },
	{ "REWIND",      "1E", KEY_REWIND },
	{ "FASTFORWARD", "01", KEY_FASTFORWARD },
	{ "PLAY",        "12", KEY_PLAY },
//	{ "PAUSE",       "",   KEY_PAUSE },
	{ "RECORD",      "1C", KEY_RECORD },
	{ "STOP",        "10", KEY_STOP },
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "CHANNELUP",   "20", KEY_CHANNELUP },
	{ "CHANNELDOWN", "21", KEY_CHANNELDOWN },
	{ "VOLUMEUP",    "22", KEY_VOLUMEUP },
	{ "VOLUMEDOWN",  "23", KEY_VOLUMEDOWN },
	{ "INFO",        "11", KEY_INFO },
	{ "OK",          "45", KEY_OK },
	{ "UP",          "18", KEY_UP },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "DOWN",        "46", KEY_DOWN },
	{ "LEFT",        "41", KEY_LEFT },
	{ "RECALL",      "15", KEY_BACK },
	{ "INFO",        "06", KEY_INFO },
	{ "ZOOM",        "1a", KEY_ZOOM },
	{ "FIND",        "16", KEY_PC },
	{ "RESOLUTION",  "07", KEY_SCREEN },
	{ "TVRADIO",     "14", KEY_MODE },
	{ "SLOW",        "0D", KEY_SLOW },
	{ "FILE",        "17", KEY_FILE },
	{ "FAV",         "00", KEY_FAVORITES },
	{ "CHECK",       "42", KEY_SELECT },
	{ "UPUP",        "43", KEY_PAGEUP },
	{ "DOWNDOWN",    "44", KEY_PAGEDOWN },
	{ "NEXT",        "02", KEY_NEXT },
	{ "PREVIOUS",    "42", KEY_PREVIOUS },
	{ "OPTION",      "19", KEY_OPTION },
	{ "STATUS",      "0C", KEY_TV2 },
	{ "SLEEP",       "24", KEY_PROGRAM },
	{ "0",           "1B", KEY_0 },
	{ "1",           "56", KEY_1 },
	{ "2",           "5A", KEY_2 },
	{ "3",           "5E", KEY_3 },
	{ "4",           "55", KEY_4 },
	{ "5",           "59", KEY_5 },
	{ "6",           "5D", KEY_6 },
	{ "7",           "54", KEY_7 },
	{ "8",           "58", KEY_8 },
	{ "9",           "5C", KEY_9 },
	{ "",            "",   KEY_NULL }
};

/* ***************** our fp button assignment **************** */

static tButton cButtonCnboxFrontpanel[] =
{
	{ "POWER",       "00", KEY_POWER },
	{ "OK",          "06", KEY_OK },
	{ "MENU",        "05", KEY_MENU },
	{ "VOLUMEUP",    "03", KEY_VOLUMEUP },
	{ "VOLUMEDOWN",  "04", KEY_VOLUMEDOWN },
	{ "CHANNELUP",   "01", KEY_CHANNELUP },
	{ "CHANNELDOWN", "02", KEY_CHANNELDOWN },
	{ "",            "",   KEY_NULL }
};

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	tCNBOXPrivate *private = malloc(sizeof(tCNBOXPrivate));

	context->r->private = private;
	memset(private, 0, sizeof(tCNBOXPrivate));
	vFd = open(rcDeviceName, O_RDWR);
	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}

	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 cnbox] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vFd;
}

static int pRead(Context_t *context)
{
	unsigned char vData[cCNBOXDataLen];
	eKeyType vKeyType = RemoteControl;
	int vCurrentCode = -1;
	static int vNextKey = 0;
	static char vOldButton = 0;

	while (1)
	{
		read(context->fd, vData, cCNBOXDataLen);
#if 0
		printf("[evremote2 cnbox] (len %d): ", n);

		for (vLoop = 0; vLoop < n; vLoop++)
		{
			printf("[evremote2 cnbox] 0x%02X ", vData[vLoop]);
		}
		printf("\n");
#endif
		if (vData[0] == 0xF1)
		{
			vKeyType = RemoteControl;
		}
		else
		{
			continue;
		}
		if (vKeyType == RemoteControl)
		{
			vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[2] & ~0x80);
			if (vCurrentCode != 0)
			{
#if 0
				vNextKey = (vData[5] & 0x80 == 0 ? vNextKey + 1 : vNextKey) % 0x100;
//				printf("[evremote2 cnbox] nextFlag %d\n", vNextKey);
				vCurrentCode += (vNextKey << 16);
#endif
				break;
			}
		}
		else
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[3]);
			if (vCurrentCode != 0)
			{
				vNextKey = (vOldButton != vData[3] ? vNextKey + 1 : vNextKey) % 0x100;
//				printf("[evremote2 cnbox] nextFlag %d\n", vNextKey);
				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
	} /* for later use we make a dummy while loop here */
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	/* noop: is handled from fp itself */
	return 0;
}

static int pShutdown(Context_t *context)
{
	tCNBOXPrivate *private = (tCNBOXPrivate *)((RemoteControl_t *)context->r)->private;

	close(context->fd);
	free(private);
	return 0;
}

RemoteControl_t CNBOX_RC =
{
	"Crenova Remote2 RemoteControl",
	CNBox,
	cButtonCnbox,
	cButtonCnboxFrontpanel,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t CNBOX_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

