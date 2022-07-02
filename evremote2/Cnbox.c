/*
 * Cnbox.c (for Atemio AM 520 HD, Sogno HD 800 V3, Opticum HD 9600 series)
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

/*****************************************************
 *
 * Although the three remote controls supported by
 * this subdriver transmit the same codes, some keys
 * labeled differently so in some cases a different
 * key code is to be reported or the same remote
 * control key value.
 *
 * The distinction is made upon driver start by
 * reading out /proc/stb/info/model.
 *
 ****************************************************/

static tButton cButtonAm520[] =  // order is same as on AM 520 HD V2 remote
{
	{ "POWER",       "5F", KEY_POWER },
	{ "1",           "56", KEY_1 },
	{ "2",           "5A", KEY_2 },
	{ "3",           "5E", KEY_3 },
	{ "4",           "55", KEY_4 },
	{ "5",           "59", KEY_5 },
	{ "6",           "5D", KEY_6 },
	{ "7",           "54", KEY_7 },
	{ "8",           "58", KEY_8 },
	{ "9",           "5C", KEY_9 },
	{ "FAV",         "00", KEY_FAVORITES },
	{ "0",           "1B", KEY_0 },
	{ "TEXT",        "19", KEY_TEXT },
	{ "RED",         "5B", KEY_RED },
	{ "GREEN",       "1F", KEY_GREEN },
	{ "YELLOW",      "08", KEY_YELLOW },
	{ "BLUE",        "03", KEY_BLUE },
	{ "EPG",         "09", KEY_EPG },
	{ "UP",          "18", KEY_UP },
	{ "INFO",        "06", KEY_INFO },
	{ "LEFT",        "41", KEY_LEFT },
	{ "OK",          "45", KEY_OK },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "EXIT",        "1D", KEY_EXIT },
	{ "DOWN",        "46", KEY_DOWN },
	{ "RECALL",      "15", KEY_BACK },  // labeled BACK
	{ "MUTE",        "57", KEY_MUTE },
	{ "MENU",        "4A", KEY_MENU },
	{ "TVRADIO",     "14", KEY_TV2 },
	{ "VOLUMEUP",    "22", KEY_VOLUMEUP },
	{ "FILELIST",    "11", KEY_FILE },  // labeled MEDIA
	{ "CHANNELUP",   "20", KEY_CHANNELUP },
	{ "VOLUMEDOWN",  "23", KEY_VOLUMEDOWN },
	{ "PLUGIN",      "60", KEY_P },
	{ "CHANNELDOWN", "21", KEY_CHANNELDOWN },
	{ "REWIND",      "1E", KEY_REWIND },
	{ "PLAY",        "12", KEY_PLAY },
	{ "FASTFORWARD", "01", KEY_FASTFORWARD },
	{ "RECORD",      "1C", KEY_RECORD },
	{ "STOP",        "10", KEY_STOP },
	{ "PAUSE",       "13", KEY_PAUSE },
	{ "PIP",         "1a", KEY_SCREEN },
	{ "FIND",        "16", KEY_SUBTITLE },  // labeled SUBTITLE
	{ "AUDIO",       "61", KEY_AUDIO },
	{ "SHOOT",       "62", KEY_S },
	{ "WWW",         "63", KEY_WWW },
	{ "HELP",        "0C", KEY_HELP },
	{ "SLEEP",       "24", KEY_PROGRAM },
	{ "MODE",        "07", KEY_SWITCHVIDEOMODE },  // labeled RES
	{ "",            "",   KEY_NULL }
};

static tButton cButtonOpt9600[] =  // order is same as on HD 9600 RC
{
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "HELP",        "11", KEY_HELP },
	{ "V.FORMAT",    "07", KEY_ZOOM },
	{ "TVRADIO",     "14", KEY_TV2 },
	{ "MODE",        "1a", KEY_SWITCHVIDEOMODE },
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
	{ "RECALL",      "15", KEY_BACK },
	{ "REWIND",      "1E", KEY_REWIND },
	{ "STOP",        "10", KEY_STOP },
	{ "PLAYPAUSE",   "12", KEY_PLAYPAUSE },
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
	{ "STATUS",      "0C", KEY_HELP },
	{ "OPTION",      "19", KEY_TEXT },
	{ "FIND",        "16", KEY_SUBTITLE },
};

static tButton cButtonAm520V1[] =  // order is same as on AM 520 HD V1 remote, to be checked
{
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "MEDIA",       "11", KEY_FILE },
	{ "TV_FORMAT",   "07", KEY_ZOOM },  // labeled RES
	{ "TVRADIO",     "14", KEY_TV2 },
	{ "PIP",         "1a", KEY_SCREEN },
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
	{ "TEXT",        "19", KEY_TEXT },
	{ "CHANNELUP",   "20", KEY_CHANNELUP },
	{ "INFO",        "06", KEY_INFO },
	{ "EPG",         "09", KEY_EPG },
	{ "VOLUMEUP",    "22", KEY_VOLUMEUP },
	{ "CHANNELDOWN", "21", KEY_CHANNELDOWN },
	{ "UP",          "18", KEY_UP },
	{ "VOLUMEDOWN",  "23", KEY_VOLUMEDOWN },
	{ "LEFT",        "41", KEY_LEFT },
	{ "OK",          "45", KEY_OK },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "EXIT",        "1D", KEY_EXIT },
	{ "DOWN",        "46", KEY_DOWN },
	{ "RECALL",      "15", KEY_BACK },
	{ "RED",         "5B", KEY_RED },
	{ "GREEN",       "1F", KEY_GREEN },
	{ "YELLOW",      "08", KEY_YELLOW },
	{ "BLUE",        "03", KEY_BLUE },
	{ "ARCHIVE",     "17", KEY_FILE },
	{ "RECORD",      "1C", KEY_RECORD },
	{ "PREVIOUS",    "02", KEY_PREVIOUS },
	{ "NEXT",        "42", KEY_NEXT },
	{ "REWIND",      "1E", KEY_REWIND },
	{ "STOP",        "10", KEY_STOP },
	{ "PLAYPAUSE",   "12", KEY_PLAYPAUSE },
	{ "FASTFORWARD", "01", KEY_FASTFORWARD },
	{ "SLOW",        "0D", KEY_SLOW },
	{ "FAV",         "00", KEY_FAVORITES },
	{ "HELP",        "0C", KEY_HELP },
	{ "SLEEP",       "24", KEY_TIME },
	{ "FIND",        "16", KEY_SUBTITLE },  // Labeled SUBTITLE
	{ "",            "",   KEY_NULL }
};

static tButton cButtonSognoHD800[] =  // order is same as on Sogno HD 800 V3 remote
{
	{ "POWER",       "5F", KEY_POWER },
	{ "MUTE",        "57", KEY_MUTE },
	{ "PORTAL",      "11", KEY_FILE },
	{ "V.FORMAT",    "07", KEY_ZOOM },  // labeled TVformat
	{ "MODE",        "14", KEY_SWITCHVIDEOMODE },  // labeled MODE
	{ "SETUP",       "1a", KEY_SCREEN },  // used as PIP
	{ "1",           "56", KEY_1 },
	{ "2",           "5A", KEY_2 },
	{ "3",           "5E", KEY_3 },
	{ "4",           "55", KEY_4 },
	{ "5",           "59", KEY_5 },
	{ "6",           "5D", KEY_6 },
	{ "7",           "54", KEY_7 },
	{ "8",           "58", KEY_8 },
	{ "9",           "5C", KEY_9 },
	{ "PREVIOUS",    "4A", KEY_PREVIOUS },
	{ "0",           "1B", KEY_0 },
	{ "NEXT",        "19", KEY_NEXT },
	{ "CHANNELUP",   "20", KEY_CHANNELUP },
	{ "INFO",        "06", KEY_INFO },
	{ "EPG",         "09", KEY_EPG },
	{ "VOLUMEUP",    "22", KEY_VOLUMEUP },
	{ "CHANNELDOWN", "21", KEY_CHANNELDOWN },
	{ "VOLUMEDOWN",  "23", KEY_VOLUMEDOWN },
	{ "UP",          "18", KEY_UP },
	{ "LEFT",        "41", KEY_LEFT },
	{ "OK",          "45", KEY_OK },
	{ "RIGHT",       "49", KEY_RIGHT },
	{ "DOWN",        "46", KEY_DOWN },
	{ "MENU",        "1D", KEY_MENU },
	{ "EXIT",        "15", KEY_EXIT },
	{ "RED",         "5B", KEY_RED },
	{ "GREEN",       "1F", KEY_GREEN },
	{ "YELLOW",      "08", KEY_YELLOW },
	{ "BLUE",        "03", KEY_BLUE },
	{ "AUDIO",       "17", KEY_AUDIO },
	{ "FIND",        "1C", KEY_SUBTITLE },
	{ "TEXT",        "02", KEY_TEXT },
	{ "SLEEP",       "42", KEY_PROGRAM },
	{ "REWIND",      "1E", KEY_REWIND },
	{ "STOP",        "10", KEY_STOP },
	{ "PLAYPAUSE",   "12", KEY_PLAYPAUSE },
	{ "FASTFORWARD", "01", KEY_FASTFORWARD },
	{ "TV",          "00", KEY_TV },
	{ "RECORD",      "0C", KEY_RECORD },
	{ "FILELIST",    "24", KEY_FILE },
	{ "RADIO",       "16", KEY_RADIO },
	{ "",            "",   KEY_NULL }
};

/* ***************** our fp button assignment **************** */

/* NOTE on front panel keys: These are all handled by the front
 * processor and yield exactly the same codes as the
 * corresponding remote control key. The front panel keys can
 * therefore not be distinguished from the remote control ones.
 */

void Get_Model(Context_t *context)
{
	char procModel[32];
	int len;
	int fn;

//	printf("[evremote2 cnbox] %s >\n", __func__);
	context->r->RemoteControl = cButtonAm520;
	fn = open("/proc/stb/info/model", O_RDONLY);

	if (fn > -1)
	{
		len = read(fn, procModel, sizeof(procModel) - 1);
		if (len > 0)
		{
			procModel[len] = 0;
			if (strncmp(procModel, "atemio520", 9) == 0)  // use Atemio AM 520 HD V2
			{
				printf("[evremote2 cnbox] Using Atemio AM 520 HD V2 mapping.\n");
			}
			else if (strncmp(procModel, "atemio520v1", 11) == 0)  // use Atemio AM 520 HD V1
			{
				context->r->RemoteControl = cButtonAm520V1;
				printf("[evremote2 cnbox] Using Atemio AM 520 HD V1 mapping.\n");
			}
			else if (strncmp(procModel, "opt9600", 7) == 0)  // use Opticum/Orton HD 9600 series
			{
				context->r->RemoteControl = cButtonOpt9600;
				printf("[evremote2 cnbox] Using Opticum/Orton HD 9600 series mapping.\n");
			}
			else if (strncmp(procModel, "sognohd", 7) == 0)  // use / Sogno HD 800 V3
			{
				context->r->RemoteControl = cButtonSognoHD800;
				printf("[evremote2 cnbox] Using Sogno HD 800 V3 mapping.\n");
			}
		}
		close(fn);
	}
	else
	{
		printf("No model detected, defaulting to Atemio AM 520 HD V2 mapping.\n");
	}
//	printf("[evremote2 cnbox] %s <\n", __func__);
}

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	tCNBOXPrivate *private = malloc(sizeof(tCNBOXPrivate));

	Get_Model(context);
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
	int vCurrentCode = -1;

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
		vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[2] & ~0x80);
		if (vCurrentCode != 0)
		{
			break;
		}
	} /* for later use we make a dummy while loop here */
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	/* noop: there are no controllable LEDs or icons on these models */
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
	"CreNova Remote2 RemoteControl",
	CNBox,
	cButtonAm520,
	NULL, //	cButtonCnboxFrontpanel,
	NULL,
	1,  // supports long key press
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

