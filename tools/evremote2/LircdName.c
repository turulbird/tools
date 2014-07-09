/*
 * LircdName.c
 *
 * (c) 2014 duckbox project
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

#define REPEATDELAY 350 // ms
#define REPEATFREQ 55 // ms
#define KEYPRESSDELAY 250 // ms

static tLongKeyPressSupport cLongKeyPressSupport =
{
	REPEATDELAY, REPEATFREQ
};

static long long GetNow(void)
{
#define MIN_RESOLUTION 1 // ms
	static bool initialized = false;
	static bool monotonic = false;
	struct timespec tp;

	if (!initialized)
	{
		// check if monotonic timer is available and provides enough accurate resolution:
		if (clock_getres(CLOCK_MONOTONIC, &tp) == 0)
		{
			//long Resolution = tp.tv_nsec;
			// require a minimum resolution:
			if (tp.tv_sec == 0 && tp.tv_nsec <= MIN_RESOLUTION * 1000000)
			{
				if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
				{
					monotonic = true;
				}
			}
		}

		initialized = true;
	}

	if (monotonic)
	{
		if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
			return (long long)(tp.tv_sec) * 1000 + tp.tv_nsec / 1000000;

		monotonic = false;
		// fall back to gettimeofday()
	}

	struct timeval t;

	if (gettimeofday(&t, NULL) == 0)
		return (long long)(t.tv_sec) * 1000 + t.tv_usec / 1000;

	return 0;
}


/* Key is recognized by name of it in lircd.conf */
static tButton cButtons_LircdName[] =
{

	{"KEY_OK"		, "00", KEY_OK},
	{"KEY_POWER"        , "01", KEY_POWER},
	{"KEY_PROGRAM"	, "02", KEY_PROGRAM}, 
	{"KEY_EPG"		, "03", KEY_EPG},
	{"KEY_PVR"		, "04", KEY_PVR},
	{"KEY_HELP"       	, "05", KEY_HELP},

	{"KEY_OPTION"	, "06", KEY_OPTION},
	{"KEY_UP"		, "07", KEY_UP},
	{"KEY_VOLUMEUP"	, "08", KEY_VOLUMEUP},
	{"KEY_PAGEUP"	, "09", KEY_PAGEUP},
	{"KEY_1"        	, "0c", KEY_1},
	{"KEY_GOTO"       	, "0f", KEY_GOTO},

	{"KEY_PAGEDOWN"	, "20", KEY_PAGEDOWN},
	{"KEY_DOWN"        	, "21", KEY_DOWN},
	{"KEY_MUTE"		, "22", KEY_MUTE},
	{"KEY_HOME"		, "23", KEY_HOME},
	{"KEY_TEXT"		, "24", KEY_TEXT},

	{"KEY_MENU"    	, "25", KEY_MENU},
	{"KEY_RED"		, "26", KEY_RED},
	{"KEY_VOLUMEDOWN"	, "28", KEY_VOLUMEDOWN},

	{"KEY_7"        	, "30", KEY_7},
	{"KEY_8"        	, "31", KEY_8},
	{"KEY_9"        	, "32", KEY_9},
	{"KEY_0"        	, "33", KEY_0},
	{"KEY_MEDIA"        , "34", KEY_MEDIA},

	{"KEY_STOP"		, "35", KEY_STOP},
	{"KEY_REWIND"	, "36", KEY_REWIND},
	{"KEY_PAUSE"	, "37", KEY_PAUSE},
	{"KEY_PLAY"		, "38", KEY_PLAY},

	{"KEY_3"        	, "40", KEY_3},
	{"KEY_4"        	, "41", KEY_4},
	{"KEY_5"        	, "42", KEY_5},
	{"KEY_6"        	, "43", KEY_6},
	{"KEY_MODE"		, "44", KEY_MODE},
	{"KEY_YELLOW"	, "45", KEY_YELLOW},

	{"KEY_RIGHT"       	, "50", KEY_RIGHT},
	{"KEY_LEFT"       	, "51", KEY_LEFT},
	{"KEY_GREEN"	, "52", KEY_GREEN},
	{"KEY_2"        	, "53", KEY_2},
	{"KEY_BLUE"		, "54", KEY_BLUE},

	{"KEY_FASTFORWARD"	, "60", KEY_FASTFORWARD},
	{"KEY_RECORD"	, "61", KEY_RECORD},
	{"KEY_LIST"		, "62", KEY_LIST},
	{"KEY_SUBTITLE"	, "63", KEY_SUBTITLE},

//------long

	{"KEY_OK"		, "40", KEY_OK},
	{"KEY_POWER"        , "41", KEY_POWER},
	{"KEY_PROGRAM"	, "42", KEY_PROGRAM},
	{"KEY_EPG"		, "43", KEY_EPG},
	{"KEY_PVR"		, "44", KEY_PVR},
	{"KEY_HELP"       	, "45", KEY_HELP},
	{"KEY_OPTION"	, "46", KEY_OPTION},
	{"KEY_UP"		, "47", KEY_UP},
	{"KEY_VOLUMEUP"	, "48", KEY_VOLUMEUP},
	{"KEY_PAGEUP"	, "49", KEY_PAGEUP},
	{"KEY_1"        	, "4c", KEY_1},
	{"KEY_GOTO"       	, "4f", KEY_GOTO},

	{"KEY_PAGEDOWN"	, "60", KEY_PAGEDOWN},
	{"KEY_DOWN"        	, "61", KEY_DOWN},
	{"KEY_MUTE"		, "62", KEY_MUTE},
	{"KEY_HOME"		, "63", KEY_HOME},
	{"KEY_TEXT"		, "64", KEY_TEXT},

	{"KEY_MENU"    	, "65", KEY_MENU},
	{"KEY_RED"		, "66", KEY_RED},
	{"KEY_VOLUMEDOWN"	, "68", KEY_VOLUMEDOWN},

	{"KEY_7"        	, "70", KEY_7},
	{"KEY_8"        	, "71", KEY_8},
	{"KEY_9"        	, "72", KEY_9},
	{"KEY_0"        	, "73", KEY_0},
	{"KEY_MEDIA"        , "74", KEY_MEDIA},

	{"KEY_STOP"		, "75", KEY_STOP},
	{"KEY_REWIND"	, "76", KEY_REWIND},
	{"KEY_PAUSE"	, "77", KEY_PAUSE},
	{"KEY_PLAY"		, "78", KEY_PLAY},

	{"KEY_3"        	, "80", KEY_3},
	{"KEY_4"        	, "81", KEY_4},
	{"KEY_5"        	, "82", KEY_5},
	{"KEY_6"        	, "83", KEY_6},
	{"KEY_MODE"		, "84", KEY_MODE},
	{"KEY_YELLOW"	, "85", KEY_YELLOW},

	{"KEY_RIGHT"       	, "90", KEY_RIGHT},
	{"KEY_LEFT"       	, "91", KEY_LEFT},
	{"KEY_GREEN"	, "92", KEY_GREEN},
	{"KEY_2"        	, "93", KEY_2},
	{"KEY_BLUE"		, "94", KEY_BLUE},

	{"KEY_FASTFORWARD"	, "a0", KEY_FASTFORWARD},
	{"KEY_RECORD"	, "a1", KEY_RECORD},
	{"KEY_LIST"		, "a2", KEY_LIST},
	{"KEY_SUBTITLE"	, "a3", KEY_SUBTITLE},

	{""               	, ""  , KEY_NULL},
};
/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un  vAddr;

static int LastKeyCode = -1;
static char LastKeyName[30];
static long long LastKeyPressedTime;


static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vAddr.sun_family = AF_UNIX;

	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	if (access("/var/run/lirc/lircd", F_OK) == 0)
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	else
	{
		strcpy(vAddr.sun_path, "/dev/lircd");
	}

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
	char vData[3];
	const int cSize = 128;
	int vCurrentCode = -1;
	char *buffer;
	//When LIRC in XMP, we need to find key by name -> codes are a bit strange
	char KeyName[30];
	int count;
	tButton *cButtons = cButtons_LircdName;

	long long LastTime;

	memset(vBuffer, 0, 128);

	//wait for new command
	read(context->fd, vBuffer, cSize);

	if (sscanf(vBuffer, "%*x %x %29s", &count, KeyName) != 2)  // '29' in '%29s' is LIRC_KEY_BUF-1!
	{
		printf("[LircdName RCU] Warning: unparseable lirc command: %s\n", vBuffer);
		return 0;
	}

	vCurrentCode = getInternalCodeLircKeyName(cButtons, KeyName);

	//printf("LastKeyPressedTime: %lld\n", LastKeyPressedTime);
	//printf("CurrKeyPressedTime: %lld\n", GetNow());
	//printf("         diffMilli: %lld\n", GetNow() - LastKeyPressedTime);

	if (vCurrentCode != 0)
	{
		//time checking
		if ((vBuffer[17] == '0') && (vBuffer[18] == '0'))
		{
			if ((LastKeyCode == vCurrentCode) && (GetNow() - LastKeyPressedTime < REPEATDELAY))   // (diffMilli(LastKeyPressedTime, CurrKeyPressedTime) <= REPEATDELAY) )
			{
				printf("[LircdName RCU] skiping same key coming in too fast %lld ms\n", GetNow() - LastKeyPressedTime);
				return 0;
			}
			else if (GetNow() - LastKeyPressedTime < KEYPRESSDELAY)
			{
				printf("[LircdName RCU] skiping different keys coming in too fast %lld ms\n", GetNow() - LastKeyPressedTime);
				return 0;
			}
			else
				printf("[RCU LircdName] key code: %s, KeyName: '%s', after %lld ms, LastKey: '%s', count: %i -> %s\n", vData, KeyName, GetNow() - LastKeyPressedTime, LastKeyName, count, &vBuffer[0]);
		}
		else if (GetNow() - LastKeyPressedTime < REPEATFREQ)
		{
			printf("[LircdName RCU] skiping repeated key coming in too fast %lld ms\n", GetNow() - LastKeyPressedTime);
			return 0;
		}

		LastKeyCode = vCurrentCode;
		LastKeyPressedTime = GetNow();
		strcpy(LastKeyName, KeyName);
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}

		//printf("[LircdName RCU] nextflag: nf -> %i\n", nextflag);
		vCurrentCode += (nextflag << 16);
	}
	else
		printf("[RCU LircdName] unknown key -> %s\n", &vBuffer[0]);

	return vCurrentCode;


}

static int pNotification(Context_t *context, const int cOn)
{
	/* noop: is handled from fp itself */
	return 0;
}

RemoteControl_t LircdName_RC =
{
	"LircdName Universal RemoteControl",
	LircdName,
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification,
	cButtons_LircdName,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport,
};
