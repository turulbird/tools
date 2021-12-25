/*
 * Ufs910_1W.c
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
#include "Ufs910_1W.h"

#define OLD  // uncomment for support of old vfd driver
#if defined OLD
#define RCdevice "/dev/ttyAS1"
#else
#define RCdevice "/dev/rc"
#endif

#define UFS910_1W_LONGKEY

#ifdef UFS910_1W_LONGKEY
static tLongKeyPressSupport cLongKeyPressSupport =
{
	20, 106
};
#endif

static tButton cButtonUFS910[] =  // order is the same as on RC660
{
	{ "MUTE",          "0D", KEY_MUTE },
	{ "POWER",         "0C", KEY_POWER },
	{ "1",             "01", KEY_1 },
	{ "2",             "02", KEY_2 },
	{ "3",             "03", KEY_3 },
	{ "4",             "04", KEY_4 },
	{ "5",             "05", KEY_5 },
	{ "6",             "06", KEY_6 },
	{ "7",             "07", KEY_7 },
	{ "8",             "08", KEY_8 },
	{ "9",             "09", KEY_9 },
	{ "MENU",          "54", KEY_MENU },
	{ "0",             "00", KEY_0 },
	{ "TEXT",          "3C", KEY_TEXT },
	{ "RED",           "6D", KEY_RED },
	{ "GREEN",         "6E", KEY_GREEN },
	{ "YELLOW",        "6F", KEY_YELLOW },
	{ "BLUE",          "70", KEY_BLUE },
	{ "VOLUMEUP",      "10", KEY_VOLUMEUP },
	{ "INFO",          "0F", KEY_INFO },
	{ "CHANNELUP",     "1E", KEY_CHANNELUP },
	{ "VOLUMEDOWN",    "11", KEY_VOLUMEDOWN },
	{ "UP",            "58", KEY_UP },
	{ "CHANNELDOWN",   "1F", KEY_CHANNELDOWN },
	{ "LEFT",          "5A", KEY_LEFT },
	{ "OK",            "5C", KEY_OK },
	{ "RIGHT",         "5B", KEY_RIGHT },
	{ "EXIT",          "55", KEY_EXIT },
	{ "DOWN",          "59", KEY_DOWN },
	{ "EPG",           "4C", KEY_EPG },
	{ "REWIND",        "21", KEY_REWIND },
	{ "PLAY",          "38", KEY_PLAY },
	{ "FASTFORWARD",   "20", KEY_FASTFORWARD },
	{ "PAUSE",         "39", KEY_PAUSE },
	{ "RECORD",        "37", KEY_RECORD },
	{ "STOP",          "31", KEY_STOP },
/* Front panel keys */
	{ "VFORMAT_FRONT", "4A", KEY_SWITCHVIDEOMODE },
	{ "MENU_FRONT",    "49", KEY_MENU },
	{ "EXIT_FRONT",    "4B", KEY_EXIT },
	{ "STANDBY_FRONT", "48", KEY_POWER },  // currently 1W models only
	{ "OPTIONS_FRONT", "47", KEY_HELP },

// Long table, compared to the above, these have bit 7 set 
	{ "LMUTE",         "8D", KEY_MUTE },
	{ "LSTANDBY",      "8C", KEY_POWER },
	{ "L1",            "81", KEY_1 },
	{ "L2",            "82", KEY_2 },
	{ "L3",            "83", KEY_3 },
	{ "L4",            "84", KEY_4 },
	{ "L5",            "85", KEY_5 },
	{ "L6",            "86", KEY_6 },
	{ "L7",            "87", KEY_7 },
	{ "L8",            "88", KEY_8 },
	{ "L9",            "89", KEY_9 },
	{ "LMENU",         "D4", KEY_MENU },
	{ "L0",            "80", KEY_0 },
	{ "LTEXT",         "BC", KEY_TEXT },
	{ "LRED",          "ED", KEY_RED },
	{ "LGREEN",        "EE", KEY_GREEN },
	{ "LYELLOW",       "EF", KEY_YELLOW },
	{ "LBLUE",         "F0", KEY_BLUE },
	{ "LVOLUMEUP",     "90", KEY_VOLUMEUP },
	{ "LINFO",         "8F", KEY_INFO },
	{ "LCHANNELUP",    "9E", KEY_CHANNELUP },
	{ "LVOLUMEDOWN",   "91", KEY_VOLUMEDOWN },
	{ "LUP",           "D8", KEY_UP },
	{ "LCHANNELDOWN",  "9F", KEY_CHANNELDOWN },
	{ "LLEFT",         "DA", KEY_LEFT },
	{ "LOK",           "DC", KEY_OK },
	{ "LRIGHT",        "DB", KEY_RIGHT },
	{ "LEXIT",         "D5", KEY_EXIT },
	{ "LDOWN",         "D9", KEY_DOWN },
	{ "LEPG",          "CC", KEY_EPG },
	{ "LREWIND",       "A1", KEY_REWIND },
	{ "LPLAY",         "B8", KEY_PLAY },
	{ "LFASTFORWARD",  "A0", KEY_FASTFORWARD },
	{ "LPAUSE",        "B9", KEY_PAUSE },
	{ "LRECORD",       "B7", KEY_RECORD },
	{ "LSTOP",         "B1", KEY_STOP },
	{ "",              "",   KEY_NULL }  // end of table
	// Front panel keys do not have long equivalents
};

static int vFd;
#ifndef UFS910_1W_LONGKEY
#else
static int gNextKey = 0;
#endif


#if defined OLD
static int setTemFlagsKathrein(int fd)
{
	struct termios old_io;
	struct termios new_io;

	if ((tcgetattr(fd, &old_io)) == 0)
	{
		new_io = old_io;

		printf("[evremote2 ufs910_1w] Setting new flags\n");
		/* c_iflags ->input flags */
		new_io.c_iflag &= ~(IMAXBEL | BRKINT | ICRNL);
		/* c_lflags ->local flags*/
		new_io.c_lflag &= ~(ECHO | IEXTEN);
		/* c_oflags ->output flags*/
		new_io.c_oflag &= ~(ONLCR);
		/* c_cflags ->constant flags*/
		new_io.c_cflag = B19200;
		tcsetattr(fd, TCSANOW, &new_io);
	}
	else
	{
		printf("[evremote2 ufs910_1w] Error setting raw mode.\n");
	}
	return 0;
}
#endif

static int pInit(Context_t *context, int argc, char *argv[])
{
	vFd = open(RCdevice, O_RDWR);
#if defined OLD
	setTemFlagsKathrein(vFd);
#endif
	return vFd;
}

static int pShutdown(Context_t *context)
{
	close(vFd);
	return 0;
}

#ifndef UFS910_1W_LONGKEY
static int pRead(Context_t *context)
{
	char vData[3];
	const int cSize = 3;
	int vCurrentCode = -1;

	// wait for new command
	read(vFd, vData, cSize);
	// parse and send key event
	vData[2] = '\0';
	vCurrentCode = getInternalCode(context->r->RemoteControl, vData);
	return vCurrentCode;
}
#else
static int pRead(Context_t *context)
{
	char vData[3];
	const int cSize = 3;
	int vCurrentCode = -1;

	// wait for new command
	read(vFd, vData, cSize);
	// parse and send key event
	vData[2] = '\0';
	vCurrentCode = getInternalCode(context->r->RemoteControl, vData);
	if ((vCurrentCode & 0x80) == 0)  // new key
	{
		gNextKey++;
		gNextKey %= 20;
	}
	vCurrentCode += (gNextKey << 16);
	return vCurrentCode;
}
#endif

static int pNotification(Context_t *context, const int cOn)
{
#if defined OLD
	if (cOn)
	{
		write(vFd, "1\n1\n1\n1\n", 8);
	}
	else
	{
		usleep(50000);
		write(vFd, "A\nA\nA\nA\n", 8);
	}
#else
	struct ufs910_fp_ioctl_data vfd_data;
	int ioctl_fd = -1;

	vfd_data.u.led.on = cOn ? 1 : 0;
	vfd_data.u.led.led_nr = LED_GREEN;

	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDSETLED, &vfd_data);
	close(ioctl_fd);
#endif
	return 0;
}

RemoteControl_t Ufs910_1W_RC =
{
	"Kathrein UFS910 (1W) RemoteControl",
	Ufs910_1W,
	cButtonUFS910,
	NULL,
	NULL,
#ifndef UFS910_1W_LONGKEY
	0,
	NULL
#else
	1,
	&cLongKeyPressSupport
#endif
};

BoxRoutines_t Ufs910_1W_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4
