/*
 * Homecast.c
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
#include "Homecast.h"

#define HS8100_PREDATA  0xA2A2
#define HS5101_PREDATA  0x0404

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140
};

static tButton cButtonsHchs8100[] =  // HS8100 & HS9000
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
	{ "EXIT",        "AF", KEY_EXIT },
	{ "0BUTTON",     "FF", KEY_0 },
	{ "MUTE",        "2F", KEY_MUTE },
	{ "RED",         "CF", KEY_RED },
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
//	{ "REPEAT",      "DD", KEY_REPEAT },
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

static tButton cButtonsHchs5101[] =  // HS5101
{
	{ "POWER",       "A7", KEY_POWER },       //   58
	{ "TV_RADIO",    "C7", KEY_TV2 },         //   38
	{ "TV_STB",      "47", KEY_TV },          //   B8
	{ "RED",         "7F", KEY_RED },         //   80
	{ "GREEN",       "BF", KEY_GREEN },       //   40
	{ "YELLOW",      "3F", KEY_YELLOW },      //   C0
	{ "BLUE",        "DF", KEY_BLUE },        //   20
	{ "MENU",        "5F", KEY_MENU },        //   A0
	{ "EPG",         "9F", KEY_EPG },         //   60
	{ "INFO",        "1F", KEY_INFO },        //   E0
	{ "UP",          "77", KEY_UP },          //   88
	{ "LEFT",        "57", KEY_LEFT },        //   A8
	{ "OK",          "37", KEY_OK },          //   C8
	{ "RIGHT",       "D7", KEY_RIGHT },       //   28
	{ "DOWN",        "F7", KEY_DOWN },        //   08
	{ "VOLUMEUP",    "2F", KEY_VOLUMEUP },    //   D0
	{ "EXIT",        "B7", KEY_EXIT },        //   48
	{ "CHANNELUP",   "0F", KEY_CHANNELUP },   //   F0
	{ "VOLUMEDOWN",  "4F", KEY_VOLUMEDOWN },  //   B0
	{ "CHANNELDOWN", "CF", KEY_CHANNELDOWN }, //   30
	{ "1BUTTON",     "7D", KEY_1 },           //   82
	{ "2BUTTON",     "BD", KEY_2 },           //   42
	{ "3BUTTON",     "3D", KEY_3 },           //   C2
	{ "4BUTTON",     "9D", KEY_4 },           //   62
	{ "5BUTTON",     "5D", KEY_5 },           //   A2
	{ "6BUTTON",     "DD", KEY_6 },           //   22
	{ "7BUTTON",     "87", KEY_7 },           //   78
	{ "8BUTTON",     "67", KEY_8 },           //   98
	{ "9BUTTON",     "E7", KEY_9 },           //   18
	{ "FUNC",        "6D", KEY_HELP },        //   92
	{ "0BUTTON",     "8D", KEY_0 },           //   72
	{ "MUTE",        "8F", KEY_MUTE },        //   70
	{ "",            "",   KEY_NULL },
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;


int homecaststring2bin(const char *string)
{
	int predata;
	int i;

	predata = 0;
	for (i = 0; i < strlen(string); i++)
	{
		predata = predata << 4;
		predata += string[i] & 0x0f;
		if (string[i] > '9')
		{
			predata += 9;
		}
	}
	return predata;
}

static tButton *homecastGetButton(int predata)
{
	tButton	*pButtons = cButtonsHchs8100;  // HS8100 remote control is default

	printf("[evremote2 Homecast] Predata: 0x%04x\n", predata);
	if (predata == HS5101_PREDATA)
	{
		pButtons = cButtonsHchs5101;
	}
	return pButtons;
}

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

//	printf("[evremote2 Homecast] %s >\n", __func__);
	vAddr.sun_family = AF_UNIX;

	// in new lircd its moved to /var/run/lirc/lircd by default and need use key to run as old version
	if (access("/var/run/lirc/lircd", F_OK) == 0)
	{
//		printf("[evremote2 Homecast] %s using /var/run/lirc/lircd\n", __func__);
		strcpy(vAddr.sun_path, "/var/run/lirc/lircd");
	}
	else
	{
//		printf("[evremote2 Homecast] %s using /dev/lircd\n", __func__);
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
	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 Homecast] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{ // TODO: handle repeat key when key is held (RC only sends headers as repeat with HS8100)
	char      vBuffer[128];
	char      vData[10];
	const int cSize = 128;
	int       vCurrentCode = -1;
//	int       i;
	int       predata;
	tButton   *cButtons;

	memset(vBuffer, 0, sizeof(vBuffer));
	// wait for new command
	read(context->fd, vBuffer, cSize);

#if 0
	printf("Received data:");
	for (i = 0; i < 30; i++)
	{
		printf(" 0x%02x", vBuffer[i]);
	}
	printf("\n");
	for (i = 0; i < 30; i++)
	{
		printf("%c", vBuffer[i]);
	}
	printf("\n");
#endif
	// get predata
	vData[0] = vBuffer[8];
	vData[1] = vBuffer[9];
	vData[2] = vBuffer[10];
	vData[3] = vBuffer[11];
	vData[4] = '\0';
	predata = homecaststring2bin(vData);
	cButtons = homecastGetButton(predata);  // get predata dependent table address

	// parse and send key event
    vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';
	printf("[evremote2 Homecast] keyvalue = 0x%s\n", vData);
	vCurrentCode = getInternalCode(cButtons, vData);

	if (vCurrentCode == KEY_POWER)
	{
		struct vfd_ioctl_data fpData;
		int ioctl_fd = -1;

		printf("[evremote2 Homecast] KEY_POWER pressed\n");
		ioctl_fd = open("/dev/vfd", O_RDONLY);
		fpData.data[0] = 0;
		ioctl(ioctl_fd, VFDSETMODE, &fpData);
		fpData.data[0] = vCurrentCode;
		ioctl(ioctl_fd, VFDSETREMOTEKEY, &fpData);
		close(ioctl_fd);
	}
	vData[0] = vBuffer[17];  // get repeat counter
	vData[1] = vBuffer[18];
//	vData[2] = '\0';
	printf("[evremote2 Homecast] repeat count = %d\n", atoi(vData));
	// determine long press (works currently only with HS5101 remote)
	if (vCurrentCode != 0)
	{
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}
		vCurrentCode += (nextflag << 16);
	}
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	struct vfd_ioctl_data vData;
	int ioctl_fd = -1;

	vData.data[0] = ICON_CIRCLE_0;
	vData.data[4] = cOn ? 1 : 0;

	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vData);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Homecast_RC =
{
	"Homecast HS5101/8100 RemoteControl",
	Homecast,
	cButtonsHchs8100,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Homecast_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4
