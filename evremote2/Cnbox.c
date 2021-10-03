/*
 * Cnbox.c (for Atemio 520HD, Opticum HD 9600 series)
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

static tButton cButtonCnbox[] =  // order is same as on HD 9600 RC
{
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "HELP",        "11", KEY_HELP },  // Labeled MEDIA on Atemio520
	{ "TV_FORMAT",   "07", KEY_ZOOM },  // Labeled RES on Atemio520
	{ "TVRADIO",     "14", KEY_TV2 },
	{ "MODE",        "1a", KEY_SWITCHVIDEOMODE },  // Labeled PIP on Atemio520
	{ "1",           "56", KEY_1 },
	{ "2",           "5A", KEY_2 },
	{ "3",           "5E", KEY_3 },
	{ "4",           "55", KEY_4 },
	{ "5",           "59", KEY_5 },
	{ "6",           "5D", KEY_6 },
	{ "7",           "54", KEY_7 },
	{ "8",           "58", KEY_8 },
	{ "9",           "5C", KEY_9 },
	{ "MENU",        "4A", KEY_MENU },
	{ "0",           "1B", KEY_0 },
	{ "RECALL",      "15", KEY_BACK },  // Labeled BACK on Atemio520
	{ "REWIND",      "1E", KEY_REWIND },
	{ "STOP",        "10", KEY_STOP },
	{ "PLAYPAUSE",   "12", KEY_PLAYPAUSE },  // Labeled PLAY on Atemio520
	{ "FASTFORWARD", "01", KEY_FASTFORWARD },
	{ "RECORD",      "1C", KEY_RECORD },
	{ "UP",          "18", KEY_UP },
	{ "EXIT",        "1D", KEY_EXIT },
	{ "LEFT",        "41", KEY_LEFT },
	{ "OK",          "45", KEY_OK },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "INFO",        "06", KEY_INFO },
	{ "DOWN",        "46", KEY_DOWN },
	{ "EPG",         "09", KEY_EPG },
	{ "ARCHIVE",     "17", KEY_FILE },
	{ "SLOW",        "0D", KEY_SLOW },
	{ "PREVIOUS",    "02", KEY_PREVIOUS },
	{ "NEXT",        "42", KEY_NEXT },
	{ "RED",         "5B", KEY_RED },
	{ "GREEN",       "1F", KEY_GREEN },
	{ "YELLOW",      "08", KEY_YELLOW },
	{ "BLUE",        "03", KEY_BLUE },
	{ "FAV",         "00", KEY_FAVORITES },
	{ "STATUS",      "0C", KEY_HELP },  // Labeled HELP on Atemio520
	{ "OPTION",      "19", KEY_TEXT },  // Labeled TEXT on Atemio520
	{ "FIND",        "16", KEY_SUBTITLE },  // Labeled SUBTITLE on Atemio520
// Keys present only on the Atemio 520 remote
	{ "VOLUMEUP",    "22", KEY_VOLUMEUP },
	{ "VOLUMEDOWN",  "23", KEY_VOLUMEDOWN },
	{ "CHANNELUP",   "20", KEY_CHANNELUP },
	{ "CHANNELDOWN", "21", KEY_CHANNELDOWN },
	{ "PLUGIN",      "60", KEY_P },
	{ "PAUSE",       "13", KEY_PAUSE },
	{ "AUDIO",       "61", KEY_AUDIO },
	{ "SHOOT",       "62", KEY_S },
	{ "WWW",         "63", KEY_WWW },
	{ "SLEEP",       "24", KEY_PROGRAM },
	{ "",            "",   KEY_NULL }
};

/* ***************** our fp button assignment **************** */

/* NOTE on front panel keys: These are all handled by the front
 * processor and yield exactly the same codes as the
 * corresponding remote control key. The front panel keys can
 * therefore not be distinguished from the remote control ones.
 */
#if 0
static tButton cButtonCnboxFrontpanel[] =
{
	{ "POWER",       "5F", KEY_POWER },
	{ "OK",          "45", KEY_OK },
	{ "MENU",        "4A", KEY_MENU },
	{ "LEFT",        "41", KEY_LEFT },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "UP",          "18", KEY_UP },
	{ "DOWN",        "46", KEY_DOWN },
	{ "",            "",   KEY_NULL }
};
#endif

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
	/* noop: there are no controllable LEDs or icons on this model */
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
	NULL, //	cButtonCnboxFrontpanel,
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

