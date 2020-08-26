/*
 * VitaminHD5000.c
 *
 * (c) 2009 dagobert@teamducktales
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

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cVitaminHD5000CommandLen 5

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120
};

static tButton cButtonVitaminHD5000[] =
{
	{ "TV/STB",      "5B", KEY_AUX },
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "TEXT",        "48", KEY_TEXT },
	{ "SUBTITLE",    "01", KEY_SUBTITLE },
	{ "SIGNAL",      "1F", KEY_F3 },
	{ "CAPTURE",     "53", KEY_PROGRAM },
	{ "BOOK/M",      "1E", KEY_F5 },
	{ "BOOK/J",      "02", KEY_SCREEN },
	{ "PREVIOUS",    "05", KEY_PREVIOUS },
	{ "REWIND",      "06", KEY_REWIND },
	{ "FASTFORWARD", "0A", KEY_FASTFORWARD },
	{ "NEXT",        "0E", KEY_NEXT },
	{ "REPEAT",      "08", KEY_AGAIN },
	{ "PLAY/PAUSE",  "50", KEY_PLAY },
	{ "STOP",        "52", KEY_STOP },
	{ "RECORD",      "0F", KEY_RECORD },
	{ "MENU",        "1C", KEY_MENU },
	{ "EXIT",        "4A", KEY_EXIT },
	{ "UP",          "18", KEY_UP },
	{ "CHUP",        "03", KEY_CHANNELUP },
	{ "VOLUP",       "40", KEY_VOLUMEUP },
	{ "LEFT",        "41", KEY_LEFT },
	{ "OK",          "45", KEY_OK },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "CHDOWN",      "07", KEY_CHANNELDOWN},
	{ "VOLDOWN",     "44", KEY_VOLUMEDOWN },
	{ "DOWN",        "46", KEY_DOWN },
	{ "INFO",        "1D", KEY_INFO },
	{ "EPG",         "1A", KEY_EPG },
	{ "RECALL",      "14", KEY_BACK },
	{ "FAV",         "4C", KEY_FAVORITES },
	{ "SAT",         "17", KEY_SAT },
	{ "MOSAIC",      "15", KEY_WWW },
	{ "FEED",        "16", KEY_F11 },
	{ "TV/RADIO",    "19", KEY_TV2 },
	{ "PLAYLIST",    "42", KEY_FILE },
	{ "RED",         "09", KEY_RED },
	{ "GREEN",       "0D", KEY_GREEN },
	{ "YELLOW",      "00", KEY_YELLOW },
	{ "BLUE",        "0C", KEY_BLUE },
	{ "1",           "56", KEY_1 },
	{ "2",           "5A", KEY_2 },
	{ "3",           "5E", KEY_3 },
	{ "4",           "55", KEY_4 },
	{ "5",           "59", KEY_5 },
	{ "6",           "5D", KEY_6 },
	{ "7",           "54", KEY_7 },
	{ "8",           "58", KEY_8 },
	{ "9",           "5C", KEY_9 },
	{ "F1",          "11", KEY_F1 },
	{ "0",           "1B", KEY_0 },
	{ "F2",          "12", KEY_F2 },
	{ "",            "",   KEY_NULL },
};

/* ***************** our fp button assignment **************** */

static tButton cButtonVitaminHD5000Frontpanel[] =
{
	{ "FRONT_LEFT",  "05", KEY_LEFT },
	{ "FRONT_OK",    "06", KEY_OK },
	{ "FRONT_RIGHT", "02", KEY_RIGHT },
	{ "FRONT_UP",    "04", KEY_UP },
	{ "FRONT_MENU",  "01", KEY_MENU },
	{ "FRONT_DOWN",  "03", KEY_DOWN },
	{ "FRONT_POWER", "00", KEY_POWER },
	{ "",            "",   KEY_NULL}
};


static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	vFd = open(rcDeviceName, O_RDWR);

	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 vitamin_hd5000] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vFd;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
	unsigned char vData[cVitaminHD5000CommandLen];
	int vCurrentCode = -1;

//	printf("%s >\n", __func__);
	while (1)
	{
		read(context->fd, vData, cVitaminHD5000CommandLen);
		printf("[evremote2 vitamin_hd5000] Read: %02X %02X %02X %02X %02X \n", vData[0], vData[1], vData[2], vData[3], vData[4]);

		/*-----------------------------------------
		* Vitamin HD5000 Remote Control
		*
		* PRESS  : 20 01 FE xx 00
		* REPEAT : 20 01 FE xx 01
		* RELEASE: 20 01 FE xx 02
		*-----------------------------------------*/
		if (vData[0] == 0x20)
		{
			if ((vData[1] == 0x01) && (vData[2] == 0xFE) && (vData[4] == 0x00))
			{
				vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[3]);
			}
		}
		/*-----------------------------------------
		 * Vitamin HD5000 Front panel
		 *
		 * PRESS  : 10 xx 00 00 00
		 * RELEASE: 10 xx 02 00 00
		 *-----------------------------------------*/
		else if ((vData[0] == 0x10) && (vData[2] == 0x00))
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
		}
		else
		{
			continue;
		}
		if (vCurrentCode != 0)
		{
			unsigned int vNextKey = vData[4];
			vCurrentCode += (vNextKey << 16);
			break;
		}
	}
//	printf("%s < %08X\n", __func__, vCurrentCode);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	/* noop: is handled from fp itself */
	return 0;
}

RemoteControl_t VitaminHD5000_RC =
{
	"Vitamin HD5000 RemoteControl",
	VitaminHD5000,
	cButtonVitaminHD5000,
	cButtonVitaminHD5000Frontpanel,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t VitaminHD5000_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

