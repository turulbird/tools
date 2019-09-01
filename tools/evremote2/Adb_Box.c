/*
 * Adb_Box.c
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
#include "Adb_Box.h"

#define REPEATDELAY 110  // ms
#define REPEATFREQ 45  // ms
#define KEYPRESSDELAY 200  // ms

#define ADB_BOX_LONGKEY

#ifdef ADB_BOX_LONGKEY
static tLongKeyPressSupport cLongKeyPressSupport =
{
//	250, 45 - b4t
	REPEATDELAY, REPEATFREQ
};
#endif

static long long GetNow(void)
{
#define MIN_RESOLUTION 1  // ms
	static bool initialized = false;
	static bool monotonic = false;
	struct timespec tp;
	struct timeval t;

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
		{
			return (long long)(tp.tv_sec) * 1000 + tp.tv_nsec / 1000000;
		}
		monotonic = false;
		// fall back to gettimeofday()
	}
	if (gettimeofday(&t, NULL) == 0)
	{
		return (long long)(t.tv_sec) * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

/* ADB_BOX XMP mode RCU '193f442a1d8307XY' */
static tButton cButtonsADB_BOX_XMP[] =
{
	{"XMP_POWER",         "01", KEY_POWER},
	{"XMP_VOD",           "34", KEY_MEDIA},  // VOD
	{"XMP_GOTO",          "0f", KEY_GOTO},  // N.Button
	{"XMP_EPG",           "03", KEY_EPG},
	{"XMP_HOUSE",         "04", KEY_PVR},  // HOUSE
	{"XMP_INFO",          "05", KEY_INFO},
	{"XMP_APP",           "02", KEY_TIME},  // APP
	{"XMP_OPT",           "06", KEY_SUBTITLE},  // OPT
	{"XMP_VOLUMEUP",      "08", KEY_VOLUMEUP},
	{"XMP_VOLUMEDOWN",    "28", KEY_VOLUMEDOWN},
	{"XMP_PAGEUP",        "09", KEY_PAGEUP},
	{"XMP_PAGEDOWN",      "20", KEY_PAGEDOWN},
	{"XMP_OK",            "00", KEY_OK},
	{"XMP_UP",            "07", KEY_UP},
	{"XMP_DOWN",          "21", KEY_DOWN},
	{"XMP_LEFT",          "51", KEY_LEFT},
	{"XMP_RIGHT",         "50", KEY_RIGHT},
	{"XMP_BACK",          "23", KEY_HOME},  // BACK
	{"XMP_STOP",          "35", KEY_STOP},
	{"XMP_REWIND",        "36", KEY_REWIND},
	{"XMP_FASTFORWARD",   "60", KEY_FASTFORWARD},
	{"XMP_PLAY",          "38", KEY_PLAY},
	{"XMP_PAUSE",         "37", KEY_PAUSE},
	{"XMP_RECORD",        "61", KEY_RECORD},
	{"XMP_MUTE",          "22", KEY_MUTE},
	{"XMP_ATSIGN",        "44", KEY_TV2},  // @, use as TV/RADIO switch
	{"XMP_TEXT",          "24", KEY_TEXT},
	{"XMP_LIST",          "62", KEY_LIST},
	{"XMP_RED",           "26", KEY_RED},
	{"XMP_GREEN",         "52", KEY_GREEN},
	{"XMP_YELLOW",        "45", KEY_YELLOW},
	{"XMP_BLUE",          "54", KEY_BLUE},
	{"XMP_1",             "0c", KEY_1},
	{"XMP_2",             "53", KEY_2},
	{"XMP_3",             "40", KEY_3},
	{"XMP_4",             "41", KEY_4},
	{"XMP_5",             "42", KEY_5},
	{"XMP_6",             "43", KEY_6},
	{"XMP_7",             "30", KEY_7},
	{"XMP_8",             "31", KEY_8},
	{"XMP_9",             "32", KEY_9},
	{"XMP_0",             "33", KEY_0},
	{"XMP_SETUP",         "25", KEY_MENU},  // SETUP
	{"XMP_STAR",          "63", KEY_HELP},  // STAR
	{"",                  "",   KEY_NULL},

//------long - this table cannot be right as the same value in the
//             short table is sometimes assigned to a different key
#if 0
	{"XMP_POWER",         "41", KEY_POWER},
	{"XMP_VOD",           "74", KEY_MEDIA},  // VOD
	{"XMP_GOTO",          "4f", KEY_GOTO},  // N.Button
	{"XMP_EPG",           "43", KEY_EPG},
	{"XMP_HOUSE",         "44", KEY_PVR},  // HOME
	{"XMP_INFO",          "45", KEY_INFO},
	{"XMP_APP",           "42", KEY_PROGRAM},  // APP
	{"XMP_OPTION",        "46", KEY_OPTION},  // OPT
	{"XMP_VOLUMEUP",      "48", KEY_VOLUMEUP},
	{"XMP_VOLUMEDOWN",    "68", KEY_VOLUMEDOWN},
	{"XMP_PAGEUP",        "49", KEY_PAGEUP},
	{"XMP_PAGEDOWN",      "60", KEY_PAGEDOWN},
	{"XMP_OK",            "40", KEY_OK},
	{"XMP_UP",            "47", KEY_UP},
	{"XMP_DOWN",          "61", KEY_DOWN},
	{"XMP_LEFT",          "91", KEY_LEFT},
	{"XMP_RIGHT",         "90", KEY_RIGHT},
	{"XMP_BACK",          "63", KEY_HOME},  // BACK
	{"XMP_STOP",          "75", KEY_STOP},
	{"XMP_REWIND",        "76", KEY_REWIND},
	{"XMP_FASTFORWARD",   "a0", KEY_FASTFORWARD},
	{"XMP_PLAY",          "78", KEY_PLAY},
	{"XMP_PAUSE",         "77", KEY_PAUSE},
	{"XMP_RECORD",        "a1", KEY_RECORD},
	{"XMP_MUTE",          "62", KEY_MUTE},
	{"XMP_AT",            "84", KEY_MODE},  // @
	{"XMP_TEXT",          "64", KEY_TEXT},
	{"XMP_LIST",          "a2", KEY_LIST},
	{"XMP_RED",           "66", KEY_RED},
	{"XMP_GREEN",         "92", KEY_GREEN},
	{"XMP_YELLOW",        "85", KEY_YELLOW},
	{"XMP_BLUE",          "94", KEY_BLUE},
	{"XMP_1",             "73", KEY_0},
	{"XMP_2",             "4c", KEY_1},
	{"XMP_3",             "93", KEY_2},
	{"XMP_4",             "80", KEY_3},
	{"XMP_5",             "81", KEY_4},
	{"XMP_6",             "82", KEY_5},
	{"XMP_7",             "83", KEY_6},
	{"XMP_8",             "70", KEY_7},
	{"XMP_9",             "71", KEY_8},
	{"XMP_0",             "72", KEY_9},
	{"XMP_SETUP",         "65", KEY_MENU},  // SETUP
	{"XMP_STAR",          "a3", KEY_SUBTITLE},  // STAR
	{"",                  "",   KEY_NULL},
#endif
};

/* ADB_BOX RAW mode RCU '000000000000003b' */
static tButton cButtonsADB_BOX_RAW[] =
{
	{"POWER",               "01", KEY_POWER},
	{"VOD",                 "02", KEY_AUX},  // VOD
	{"GOTO",                "03", KEY_V},  // N.Button
	{"EPG",                 "04", KEY_EPG},
	{"HOME",                "05", KEY_BACK},  // HOME
	{"BACK",                "06", KEY_HOME},  // BACK
	{"INFO",                "07", KEY_INFO},
	{"OPTION",              "08", KEY_AUDIO},   // OPT
	{"VOLUMEUP",            "09", KEY_VOLUMEUP},
	{"VOLUMEDOWN",          "0a", KEY_VOLUMEDOWN},
	{"CHUP",                "0b", KEY_PAGEUP},
	{"CHDOWN",              "0c", KEY_PAGEDOWN},
	{"OK",                  "0d", KEY_OK},
	{"UP",                  "0e", KEY_UP},
	{"DOWN",                "0f", KEY_DOWN},
	{"LEFT",                "10", KEY_LEFT},
	{"RIGHT",               "11", KEY_RIGHT},
	{"STOP",                "12", KEY_STOP},
	{"REWIND",              "13", KEY_REWIND},
	{"FASTFORWARD",         "14", KEY_FASTFORWARD},
	{"PLAY",                "15", KEY_PLAY},
	{"PAUSE",               "16", KEY_PAUSE},
	{"RECORD",              "17", KEY_RECORD},
	{"MUTE",                "18", KEY_MUTE},
	{"AT",                  "19", KEY_TV2},  // TV/RADIO/@
	{"TEXT",                "1a", KEY_TEXT},
	{"LIST",                "1b", KEY_FAVORITES},
	{"RED",                 "1c", KEY_RED},
	{"GREEN",               "1d", KEY_GREEN},
	{"YELLOW",              "1e", KEY_YELLOW},
	{"BLUE",                "1f", KEY_BLUE},
	{"1",                   "20", KEY_1},
	{"2",                   "21", KEY_2},
	{"3",                   "22", KEY_3},
	{"4",                   "23", KEY_4},
	{"5",                   "24", KEY_5},
	{"6",                   "25", KEY_6},
	{"7",                   "26", KEY_7},
	{"8",                   "27", KEY_8},
	{"9",                   "28", KEY_9},
	{"0",                   "29", KEY_0},
	{"SETUP",               "2a", KEY_MENU},  // AUDIO/SETUP
	{"APP",                 "2b", KEY_TIME},  // TIMER/APP
	{"STAR",                "2c", KEY_HELP},  // STAR

//------long - Compared to the raw table, these have bit 6 set
	{"POWER",               "41", KEY_POWER},
	{"VOD",                 "42", KEY_AUX},
	{"N.Button",            "43", KEY_V},
	{"EPG",                 "44", KEY_EPG},
	{"HOME",                "45", KEY_BACK},  // HOME
	{"BACK",                "46", KEY_HOME},  // BACK
	{"INFO",                "47", KEY_INFO},  // THIS IS WRONG, SHOULD BE KEY_INFO
	{"OPT",                 "48", KEY_MENU},
	{"KEY_VOLUMEUP_LONG",   "49", KEY_VOLUMEUP},
	{"KEY_VOLUMEDOWN_LONG", "4a", KEY_VOLUMEDOWN},
	{"CHANNELUP",           "4b", KEY_PAGEUP},
	{"CHANNELDOWN",         "4c", KEY_PAGEDOWN},
	{"KEY_OK",              "4d", KEY_OK},
	{"KEY_UP_LONG",         "4e", KEY_UP},
	{"KEY_DOWN_LONG",       "4f", KEY_DOWN},
	{"KEY_LEFT_LONG",       "50", KEY_LEFT},
	{"KEY_RIGHT_LONG",      "51", KEY_RIGHT},
	{"STOP",                "52", KEY_STOP},
	{"REWIND",              "53", KEY_REWIND},
	{"FASTFORWARD",         "54", KEY_FASTFORWARD},
	{"PLAY",                "55", KEY_PLAY},
	{"PAUSE",               "56", KEY_PAUSE},
	{"RECORD",              "57", KEY_RECORD},
	{"MUTE",                "58", KEY_MUTE},
	{"TV/RADIO/@",          "59", KEY_TV2},  // WE USE TV2 AS TV/RADIO SWITCH BUTTON
	{"TEXT",                "5a", KEY_TEXT},
	{"LIST",                "5b", KEY_FAVORITES},
	{"KEY_RED",             "5c", KEY_RED},
	{"KEY_GREEN",           "5d", KEY_GREEN},
	{"KEY_YELLOW",          "5e", KEY_YELLOW},
	{"KEY_BLUE",            "5f", KEY_BLUE},
	{"KEY_1",               "60", KEY_1},
	{"KEY_2",               "61", KEY_2},
	{"KEY_3",               "62", KEY_3},
	{"KEY_4",               "63", KEY_4},
	{"KEY_5",               "64", KEY_5},
	{"KEY_6",               "65", KEY_6},
	{"KEY_7",               "66", KEY_7},
	{"KEY_8",               "67", KEY_8},
	{"KEY_9",               "68", KEY_9},
	{"KEY_9",               "69", KEY_9},
	{"AUDIO/SETUP",         "6a", KEY_AUDIO},
	{"TIMER/APP",           "6b", KEY_TIME},
	{"STAR",                "6c", KEY_HELP},
	{"",                    "",   KEY_NULL},
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;

static int LastKeyCode = -1;
static char LastKeyName[30];
static long long LastKeyPressedTime;


static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	vAddr.sun_family = AF_UNIX;

	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	if (access("/var/run/lirc/lircd", F_OK) == 0)
	{
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	}
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
	// When LIRC in XMP, we need to find key by name -> codes are a bit strange
	char KeyName[30];
	int count;
	int i;
	tButton *cButtons = cButtonsADB_BOX_XMP;

//	printf("[evremote2 adb_box] %s >\n", __func__);

	memset(vBuffer, 0, 128);

	// wait for new command
	read(context->fd, vBuffer, cSize);

	if (sscanf(vBuffer, "%*x %x %29s", &count, KeyName) != 2)  // '29' in '%29s' is LIRC_KEY_BUF-1!
	{
		printf("[evremote2 adb_box] Warning: unparseable lirc command [%s]\n", vBuffer);
		return 0;
	}

	// checking what RCU definition mode to use
	if (('1' == vBuffer[0]) && ('9' == vBuffer[1]) && ('3' == vBuffer[2]))
	{
		printf("[evremote2 adb_box] Using XMP table\n");
		cButtons = cButtonsADB_BOX_XMP;
		vData[0] = vBuffer[12];
		vData[1] = vBuffer[13];
		vData[2] = '\0';
	}
	else if (('0' == vBuffer[0]) && ('0' == vBuffer[1]) && ('0' == vBuffer[2]))
	{
		printf("[evremote2 adb_box] Using RAW table\n");
		cButtons = cButtonsADB_BOX_RAW;
		vData[0] = vBuffer[14];
		vData[1] = vBuffer[15];
		vData[2] = '\0';
	}
	else
	{
		printf("[evremote2 adb_box] Using unknown table\n");
		for (i = 0; i < 30; i++)
		{
			printf("vBuffer[%02d] = 0x%02x\n", i, vBuffer[i]);
		}
	}

	// vCurrentCode = getInternalCode((cButtons*)context->r->RemoteControl, vData);
	vCurrentCode = getInternalCode(cButtons, vData);

	// if no code found, let's try lirc KeyName
	if (vCurrentCode == 0)
	{
		printf("[evremote2 adb_box] No code found trying to match KeyName\n");
		// vCurrentCode = getInternalCodeLircKeyName((cButtons*)context->r->RemoteControl, KeyName);
		vCurrentCode = getInternalCodeLircKeyName(cButtons, KeyName);
	}
	printf("[evremote2 adb_box] LastKeyPressedTime: %lld\n", LastKeyPressedTime);
	printf("                    CurrKeyPressedTime: %lld\n", GetNow());
	printf("                             diffMilli: %lld\n", GetNow() - LastKeyPressedTime);
	if (vCurrentCode != 0)
	{
		//time checking
		if ((vBuffer[17] == '0') && (vBuffer[18] == '0'))
		{
			if ((LastKeyCode == vCurrentCode) && ((GetNow() - LastKeyPressedTime) < REPEATDELAY))   // (diffMilli(LastKeyPressedTime, CurrKeyPressedTime) <= REPEATDELAY) )
			{
				printf("[evremote2 adb_box] Skipping same key coming in too fast (%lld ms)\n", GetNow() - LastKeyPressedTime);
				return 0;
			}
			else if (GetNow() - LastKeyPressedTime < KEYPRESSDELAY)
			{
				printf("[evremote2 adb_box] Skipping different keys coming in too fast (%lld ms)\n", GetNow() - LastKeyPressedTime);
				return 0;
			}
			else
				printf("[evremote2 adb_box] key code: %s, KeyName: '%s', after %lld ms, LastKey: '%s', count: %i -> %s\n", vData, KeyName, GetNow() - LastKeyPressedTime, LastKeyName, count, &vBuffer[0]);
		}
		else if (GetNow() - LastKeyPressedTime < REPEATFREQ)
		{
			printf("[evremote2 adb_box] skipping repeated key coming in too fast (%lld ms)\n", GetNow() - LastKeyPressedTime);
			return 0;
		}
		static int nextflag = 0;

		LastKeyCode = vCurrentCode;
		LastKeyPressedTime = GetNow();
		strcpy(LastKeyName, KeyName);
		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}

//		printf("[evremote2 adb_box] nextflag: nf -> %i\n", nextflag);
		vCurrentCode += (nextflag << 16);
	}
	else
	{
		printf("[evremote2 adb_box] Unknown key: %s\n", &vBuffer[0]);
	}
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	int ioctl_fd = -1;
	struct adb_box_fp_ioctl_data fp_data;


	fp_data.u.led.led_nr = 4;  // alert LED
	fp_data.u.led.level = cOn ? 1 : 0;

	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDSETLED, &fp_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Adb_Box_RC =
{
	"Adb_Box Universal RemoteControl",
	Adb_Box,
	cButtonsADB_BOX_XMP,
	NULL,
	NULL,
#ifndef ADB_BOX_LONGKEY
	0,
	NULL
#else
	1,
	&cLongKeyPressSupport
#endif
};

BoxRoutines_t Adb_Box_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

