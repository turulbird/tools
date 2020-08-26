/*
 * Ufs910_14W.c
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

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"

static tButton cButtonsKathrein[] =
{
	{ "MENU",          "54", KEY_MENU },
	{ "RED",           "6D", KEY_RED },
	{ "GREEN",         "6E", KEY_GREEN },
	{ "YELLOW",        "6F", KEY_YELLOW },
	{ "BLUE",          "70", KEY_BLUE },
	{ "EXIT",          "55", KEY_EXIT },
	{ "TEXT",          "3C", KEY_TEXT },
	{ "EPG",           "4C", KEY_EPG },
	{ "REWIND",        "21", KEY_REWIND },
	{ "FASTFORWARD",   "20", KEY_FASTFORWARD },
	{ "PLAY",          "38", KEY_PLAY },
	{ "PAUSE",         "39", KEY_PAUSE },
	{ "RECORD",        "37", KEY_RECORD },
	{ "STOP",          "31", KEY_STOP },
	{ "POWER",         "0C", KEY_POWER },
	{ "MUTE",          "0D", KEY_MUTE },
	{ "CHANNELUP",     "1E", KEY_CHANNELUP },
	{ "CHANNELDOWN",   "1F", KEY_CHANNELDOWN },
	{ "VOLUMEUP",      "10", KEY_VOLUMEUP },
	{ "VOLUMEDOWN",    "11", KEY_VOLUMEDOWN },
	{ "INFO",          "0F", KEY_INFO },
	{ "OK",            "5C", KEY_OK },
	{ "UP",            "58", KEY_UP },
	{ "RIGHT",         "5B", KEY_RIGHT },
	{ "DOWN",          "59", KEY_DOWN },
	{ "LEFT",          "5A", KEY_LEFT },
	{ "0",             "00", KEY_0 },
	{ "1",             "01", KEY_1 },
	{ "2",             "02", KEY_2 },
	{ "3",             "03", KEY_3 },
	{ "4",             "04", KEY_4 },
	{ "5",             "05", KEY_5 },
	{ "6",             "06", KEY_6 },
	{ "7",             "07", KEY_7 },
	{ "8",             "08", KEY_8 },
	{ "9",             "09", KEY_9 },
/* Front panel keys */
	{ "VFORMAT_FRONT", "4A", KEY_SWITCHVIDEOMODE },
	{ "MENU_FRONT",    "49", KEY_MENU },
	{ "EXIT_FRONT",    "4B", KEY_EXIT },
	{ "POWER_FRONT",   "48", KEY_POWER },
	{ "OPTIONS_FRONT", "47", KEY_HELP },

// Long table, compared to the above, these have bit 7 set 
	{ "LMENU",        "D4", KEY_MENU },
	{ "LRED",         "ED", KEY_RED },
	{ "LGREEN",       "EE", KEY_GREEN },
	{ "LYELLOW",      "EF", KEY_YELLOW },
	{ "LBLUE",        "F0", KEY_BLUE },
	{ "LEXIT",        "D5", KEY_EXIT },
	{ "LTEXT",        "BC", KEY_TEXT },
	{ "LEPG",         "CC", KEY_EPG },
	{ "LREWIND",      "A1", KEY_REWIND },
	{ "LFASTFORWARD", "A0", KEY_FASTFORWARD },
	{ "LPLAY",        "B8", KEY_PLAY },
	{ "LPAUSE",       "B9", KEY_PAUSE },
	{ "LRECORD",      "B7", KEY_RECORD },
	{ "LSTOP",        "B1", KEY_STOP },
	{ "LPOWER",       "8C", KEY_POWER },
	{ "LMUTE",        "8D", KEY_MUTE },
	{ "LCHANNELUP",   "9E", KEY_CHANNELUP },
	{ "LCHANNELDOWN", "9F", KEY_CHANNELDOWN },
	{ "LVOLUMEUP",    "90", KEY_VOLUMEUP },
	{ "LVOLUMEDOWN",  "91", KEY_VOLUMEDOWN },
	{ "LINFO",        "8F", KEY_INFO },
	{ "LOK",          "DC", KEY_OK },
	{ "LUP",          "D8", KEY_UP },
	{ "LRIGHT",       "DB", KEY_RIGHT },
	{ "LDOWN",        "D9", KEY_DOWN },
	{ "LLEFT",        "DA", KEY_LEFT },
	{ "L0",           "80", KEY_0 },
	{ "L1",           "81", KEY_1 },
	{ "L2",           "82", KEY_2 },
	{ "L3",           "83", KEY_3 },
	{ "L4",           "84", KEY_4 },
	{ "L5",           "85", KEY_5 },
	{ "L6",           "86", KEY_6 },
	{ "L7",           "87", KEY_7 },
	{ "L8",           "88", KEY_8 },
	{ "L9",           "89", KEY_9 },
	{ "",             "",   KEY_NULL },
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;

static int vFdLed;


static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vFdLed = open("/sys/class/leds/ufs910:green/brightness", O_WRONLY);
	vAddr.sun_family = AF_UNIX;
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
	int vCurrentCode = -1;

	// wait for new command
	read(context->fd, vBuffer, cSize);
	// parse and send key event
	vData[0] = vBuffer[17];
	vData[1] = vBuffer[18];
	vData[2] = '\0';
	//prell, we could detect a long press here if we want
	if (atoi(vData) % 3 != 0)
	{
		return -1;
	}
	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';
	vCurrentCode = getInternalCode(context->r->RemoteControl, vData);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	if (cOn)
	{
		// activate green led
		write(vFdLed, "1", 1);
	}
	else
	{
		// deactivate green led
		usleep(50000);
		write(vFdLed, "0", 1);
	}
	return 0;
}

RemoteControl_t Ufs910_14W_RC =
{
	"Kathrein UFS910 (14W) RemoteControl",
	Ufs910_14W,
	cButtonsKathrein,
	NULL,
	NULL,
	0,
	NULL
};

BoxRoutines_t Ufs910_14W_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

