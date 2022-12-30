/*
 * Pace7241.c
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
#include "Pace7241.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cPaceDataLen 128

typedef struct
{
} tPacePrivate;

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140
};

#if 0
      begin codes
        KEY_0           0x00000000003a7fff
        KEY_1		    0x00000000003a7ffe
        KEY_2		    0x00000000003a7ffd
        KEY_3		    0x00000000003a7ffc
        KEY_4		    0x00000000003a7ffb
        KEY_5		    0x00000000003a7ffa
        KEY_6		    0x00000000003a7ff9
        KEY_7		    0x00000000003a7ff8
        KEY_8		    0x00000000003a7ff7
        KEY_9		    0x00000000003A7FF6
        KEY_0		    0x00000000003a7fff
        KEY_MODE	    0x00000000003a7fe9
        KEY_HOME	    0x00000000003a7fc4
        KEY_HOME	    0x00000000003a7fc3
        KEY_PAUSE	    0x00000000003a7fc5
        KEY_STOP	    0x00000000003a7fc7
        KEY_RECORD	    0x00000000003a7fc6
        KEY_VOLUMEUP    0x00000000003a7fc2
        KEY_PLAY	    0x00000000003a7fc9
        KEY_VOLUMEDOWN	0x00000000003a7fc1
        KEY_OK		    0x00000000003a7fd5
        KEY_DOWN	    0x00000000003a7fd9
        KEY_UP		    0x00000000003a7fdd
        KEY_RIGHT	    0x00000000003a7fdf
        KEY_LEFT	    0x00000000003a7fdb
        KEY_EPG		    0x00000000003a7fe5
        KEY_HELP	    0x00000000003a7fe0
        KEY_RED		    0x00000000003a7fd4
        KEY_GREEN	    0x00000000003a7fd3
        KEY_YELLOW	    0x00000000003a7fd2
        KEY_BLUE	    0x00000000003a7fd1
        KEY_HOME	    0x00000000003a7fd0	#E
        KEY_MUTE	    0x00000000003a7fe1
        KEY_MENU	    0x00000000003a7fe6
        KEY_HOME	    0x00000000003a7feb
        KEY_POWER	    0x00000000003a7ff0
        KEY_OPTION	    0x00000000003a7fe2	#SERW
        KEY_MEDIA	    0x00000000003a7fd6	#+
        KEY_LIST	    0x00000000003a7fea	#SAT ANT
        

        KEY_RIGHT&	    0x00000000003a7edf
        KEY_LEFT&	    0x00000000003a7edb
        KEY_DOWN&	    0x00000000003a7ed9
        KEY_UP&		    0x00000000003a7edd

      end codes
#endif
static tButton cButtonPace[] =
{
	{ "MUTE",        "0C", KEY_MUTE },
	{ "POWER",       "0A", KEY_POWER },
	{ "V.FORMAT",    "0E", KEY_ZOOM },
	{ "RESOLUTION",  "0F", KEY_SWITCHVIDEOMODE },
	{ "1",           "11", KEY_1 },
	{ "2",           "12", KEY_2 },
	{ "3",           "13", KEY_3 },
	{ "4",           "14", KEY_4 },
	{ "5",           "15", KEY_5 },
	{ "6",           "16", KEY_6 },
	{ "7",           "17", KEY_7 },
	{ "8",           "18", KEY_8 },
	{ "9",           "19", KEY_9 },
	{ "INFO",        "06", KEY_INFO },
	{ "0",           "10", KEY_0 },
	{ "RECALL",      "09", KEY_BACK },
	{ "VOLUMEUP",    "4E", KEY_VOLUMEUP },
	{ "MENU",        "04", KEY_MENU },
	{ "CHANNELUP",   "5E", KEY_CHANNELUP },
	{ "VOLUMEDOWN",  "4F", KEY_VOLUMEDOWN },
	{ "EXIT",        "1C", KEY_EXIT },
	{ "CHANNELDOWN", "5F", KEY_CHANNELDOWN },
	{ "UP",          "00", KEY_UP },
	{ "LEFT",        "03", KEY_LEFT },
	{ "OK",          "1F", KEY_OK },
	{ "RIGHT",       "02", KEY_RIGHT },
	{ "DOWN",        "01", KEY_DOWN },
	{ "PAGEUP",      "43", KEY_PAGEUP },
	{ "EPG",         "08", KEY_EPG },
	{ "PAGEDOWN",    "44", KEY_PAGEDOWN },
	{ "REWIND",      "58", KEY_REWIND },
	{ "PLAY",        "55", KEY_PLAY },
	{ "FASTFORWARD", "5C", KEY_FASTFORWARD },
	{ "PREVIOUS",    "50", KEY_PREVIOUS },
	{ "RECORD",      "56", KEY_RECORD },
	{ "NEXT",        "4C", KEY_NEXT },
	{ "PLAYLIST",    "40", KEY_FILE },
	{ "PAUSE",       "07", KEY_PAUSE },
	{ "STOP",        "54", KEY_STOP },
	{ "CHECK",       "42", KEY_HELP },
	{ "RED",         "4B", KEY_RED },
	{ "GREEN",       "4A", KEY_GREEN },
	{ "YELLOW",      "49", KEY_YELLOW },
	{ "BLUE",        "48", KEY_BLUE },
	{ "PIP",         "51", KEY_SCREEN },
	{ "PIP_SWAP",    "52", KEY_GOTO },
	{ "PIP_LIST",    "53", KEY_AUX },
	{ "SLEEP",       "1E", KEY_PROGRAM },
	{ "FAV",         "41", KEY_FAVORITES },
	{ "TVRADIO",     "1A", KEY_TV2 },
	{ "SUBTITLE",    "0B", KEY_SUBTITLE },
	{ "TEXT",        "0D", KEY_TEXT },
	{ "",            "",   KEY_NULL }
};

/* ***************** front panel button assignment **************** */

static tButton cButtonPaceFrontpanel[] =
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
	tPacePrivate *private = malloc(sizeof(tPacePrivate));

	context->r->private = private;
	memset(private, 0, sizeof(tPacePrivate));
	vFd = open(rcDeviceName, O_RDWR);
	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 pace7241] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vFd;
}

static int pRead(Context_t *context)
{
	unsigned char vData[cPaceDataLen];
	eKeyType vKeyType = RemoteControl;
	int vCurrentCode = -1;
	static int vNextKey = 0;
	static char vOldButton = 0;

	while (1)
	{
		read(context->fd, vData, cPaceDataLen);
#if 0
		printf("[evremote2 pace] (len %d): ", n);

		for (vLoop = 0; vLoop < n; vLoop++)
		{
			printf("0x%02X ", vData[vLoop]);
		}
		printf("\n");
#endif
		if ((vData[2] != 0x51) && (vData[2] != 0x63) && (vData[2] != 0x80))
		{
			continue;
		}
		if (vData[2] == 0x63)
		{
			vKeyType = RemoteControl;
		}
		else if (vData[2] == 0x51)
		{
			vKeyType = FrontPanel;
		}
		else
		{
			continue;
		}
		if (vKeyType == RemoteControl)
		{
			vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[5] & ~0x80);

			if (vCurrentCode != 0)
			{
				vNextKey = (vData[5] & (0x80 == 0 ? vNextKey + 1 : vNextKey)) % 0x100;
//				printf("[evremote2 pace] nextFlag %d\n", vNextKey);
				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
		else
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[3]);

			if (vCurrentCode != 0)
			{
				vNextKey = (vOldButton != vData[3] ? vNextKey + 1 : vNextKey) % 0x100;
//				printf("[evremote2 pace] nextFlag %d\n", vNextKey);
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
	tPacePrivate *private = (tPacePrivate *)context->r->private;

	close(context->fd);
	free(private);
	return 0;
}

RemoteControl_t Pace7241_RC =
{
	"Pace RemoteControl",
	Pace7241,
	cButtonPace,
	cButtonPaceFrontpanel,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Pace7241_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

