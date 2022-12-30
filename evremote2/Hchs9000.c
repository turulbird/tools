/*
 * Hcs9000.c
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
#include "Hchs9000.h"

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140
};


static tButton cButtonsHchs9000[] =
{
	{ "POWER",       "F7", KEY_POWER },
	{ "TV_RADIO",    "77", KEY_TV2 },
	{ "TEXT",        "B7", KEY_TEXT },
	{ "1BUTTON",     "7F", KEY_1 },
	{ "2BUTTON",     "BF", KEY_2 },
	{ "3BUTTON",     "3F", KEY_3 },
	{ "4BUTTON",     "DF", KEY_4 },
	{ "5BUTTON",     "5F", KEY_5 },
	{ "6BUTTON",     "9F", KEY_6 },
	{ "7BUTTON",     "1F", KEY_7 },
	{ "8BUTTON",     "EF", KEY_8 },
	{ "9BUTTON",     "6F", KEY_9 },
	{ "BACK",        "AF", KEY_BACK },
	{ "0BUTTON",     "FF", KEY_0 },
	{ "MUTE",        "2F", KEY_MUTE },
	{ "RED",         "7F", KEY_RED },
	{ "GREEN",       "4F", KEY_GREEN },
	{ "YELLOW",      "8F", KEY_YELLOW },
	{ "BLUE",        "0F", KEY_BLUE },
	{ "MENU",        "37", KEY_MENU },
	{ "EPG",         "D7", KEY_EPG },
	{ "INFO",        "57", KEY_INFO },
	{ "UP",          "97", KEY_UP },
	{ "LEFT",        "17", KEY_LEFT },
	{ "OK",          "87", KEY_OK },
	{ "RIGHT",       "07", KEY_RIGHT },
	{ "DOWN",        "FD", KEY_DOWN },
	{ "VOLUMEUP",    "7D", KEY_VOLUMEUP },
	{ "CHANNELUP",   "DD", KEY_CHANNELUP },
	{ "EXIT",        "3D", KEY_EXIT },
	{ "VOLUMEDOWN",  "BD", KEY_VOLUMEDOWN },
	{ "CHANNELDOWN", "5D", KEY_CHANNELDOWN },
	{ "PAUSE",       "9D", KEY_PAUSE },
	{ "PLAY",        "1D", KEY_PLAY },
	{ "REWIND",      "ED", KEY_REWIND },
	{ "STOP",        "6D", KEY_STOP },
	{ "FASTFORWARD", "F5", KEY_FASTFORWARD },
	{ "RECORD",      "75", KEY_RECORD },
	{ "SLOW",        "B5", KEY_SLOW },
	{ "LIST",        "35", KEY_FILE },
//	{ "REPEAT",      "95", KEY_REPEAT },
	{ "MARK",        "55", KEY_HELP },
	{ "JUMP",        "D5", KEY_GOTO },
	{ "PIP",         "A5", KEY_SCREEN },
//	{ "PIP_SWAP",    "B5", KEY_GOTO },
//	{ "PIP_MOVE",    "B5", KEY_GOTO },
	{ "SLEEP",       "45", KEY_PROGRAM },
//	{ "TV",          "47", KEY_TV },
//	{ "STB",         "B5", KEY_GOTO },
	{ "",            "",   KEY_NULL },
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	printf("[evremote2 hchs9000] %s >\n", __func__);
	vAddr.sun_family = AF_UNIX;
//	strcpy(vAddr.sun_path, "/var/run/lirc/lircd");

	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	if (access("/var/run/lirc/lircd", F_OK) == 0)
	{
		printf("[evremote2 hchs9000] %s using /var/run/lirc/lircd\n", __func__);
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	}
	else
	{
		printf("[evremote2 hchs9000] %s using /dev/lircd\n", __func__);
		strcpy(vAddr.sun_path, "/dev/lircd");
	}
	vHandle = socket(AF_UNIX, SOCK_STREAM, 0);
	if (vHandle == -1)
	{
		perror("socket");
		return -1;
	}
	printf("[evremote2 hchs9000] %s vHandle = 0x%x\n", __func__, vHandle);
	if (connect(vHandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		perror("connect");
		return -1;
	}
	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 hchs9000] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
	char      vBuffer[128];
	char      vData[10];
	const int cSize = 128;
	int       vCurrentCode = -1;
	int i;

	memset(vBuffer, 0, sizeof(vBuffer));
	// wait for new command
	read(context->fd, vBuffer, cSize);

	printf("Received data:");
	for (i = 0; i < 30; i++)
	{
		printf(" 0x%02x", vBuffer[i]);
	}
	printf("\n");

	// parse and send key event
	vData[0] = vBuffer[17];
	vData[1] = vBuffer[18];
	vData[2] = '\0';
    
	// prell, we could detect a long press here if we want
	if (atoi(vData) % 3 != 0)
	{
		return -1;
	}
	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';

	vCurrentCode = getInternalCode((tButton*)((RemoteControl_t*)context->r)->RemoteControl, vData);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	struct hchs9000_fp_ioctl_data vfd_data;
	int ioctl_fd = -1;

	vfd_data.u.icon.on = cOn ? 1 : 0;
	vfd_data.u.icon.icon_nr = ICON_CIRCLE_0;

	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Hchs9000_RC =
{
	"Homecast HS9000 RemoteControl",
	Hchs9000,
	cButtonsHchs9000,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Hchs9000_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4
