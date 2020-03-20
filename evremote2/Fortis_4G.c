/*
 * Fortis_4G.c
 *
 * (c) 2010 duckbox project
 * (c) 2019 Audioniek
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
#include "Fortis_4G.h"

/* ***************** some constants **************** */

//#define	FORTIS_PREDATA "409F"
#define cFortisDataLen 128

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 140
};

static tButton cButtonFortis[] =
{
	{"MUTE",        "CF", KEY_MUTE},
	{"POWER",       "AF", KEY_POWER},
	{"VFORMAT",     "8F", KEY_ZOOM},
	{"RESOLUTION",  "0F", KEY_SCREEN},
	{"1",           "77", KEY_1},
	{"2",           "B7", KEY_2},
	{"3",           "37", KEY_3},
	{"4",           "D7", KEY_4},
	{"5",           "57", KEY_5},
	{"6",           "97", KEY_6},
	{"7",           "17", KEY_7},
	{"8",           "E7", KEY_8},
	{"9",           "67", KEY_9},
	{"INFO",        "9F", KEY_INFO},
	{"0",           "F7", KEY_0},
	{"RECALL",      "6F", KEY_BACK},
	{"VOLUMEUP",    "8D", KEY_VOLUMEUP},
	{"MENU",        "DF", KEY_MENU},
	{"CHANNELUP",   "85", KEY_PAGEUP},
	{"VOLUMEDOWN",  "0D", KEY_VOLUMEDOWN},
	{"EXIT",        "C7", KEY_EXIT},
	{"CHANNELDOWN", "05", KEY_PAGEDOWN},
	{"UP",          "FF", KEY_UP},
	{"LEFT",        "3F", KEY_LEFT},
	{"OK",          "07", KEY_OK},
	{"RIGHT",       "BF", KEY_RIGHT},
	{"DOWN",        "7F", KEY_DOWN},
	{"UPUP",        "3D", KEY_TEEN},
	{"EPG",         "EF", KEY_EPG},
	{"DOWNDOWN",    "DD", KEY_TWEN},
	{"REWIND",      "E5", KEY_REWIND},
	{"PLAY",        "55", KEY_PLAY},
	{"FASTFORWARD", "C5", KEY_FASTFORWARD},
	{"PREVIOUS",    "F5", KEY_PREVIOUS},
	{"RECORD",      "95", KEY_RECORD},
	{"NEXT",        "CD", KEY_NEXT},
	{"PLAYLIST",    "FD", KEY_FILE},
	{"PAUSE",       "1F", KEY_PAUSE},
	{"STOP",        "D5", KEY_STOP},
	{"CHECK",       "BD", KEY_HELP},
	{"RED",         "2D", KEY_RED},
	{"GREEN",       "AD", KEY_GREEN},
	{"YELLOW",      "6D", KEY_YELLOW},
	{"BLUE",        "ED", KEY_BLUE},
	{"PIP",         "75", KEY_SCREEN},
	{"PIP_SWAP",    "B5", KEY_GOTO},
	{"PIP_LIST",    "35", KEY_AUX},
	{"SLEEP",       "87", KEY_TIME},
	{"FAV",         "7D", KEY_FAVORITES},
	{"TVRADIO",     "A7", KEY_TV2},
	{"SUBTITLE",    "2F", KEY_SUBTITLE},
	{"TEXT",        "4F", KEY_TEXT},
	{"",            "",   KEY_NULL}
};

/* ***************** front panel button assignment **************** *
 *
 * Front panel keys are handled by the front panel driver
 */

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;

#if 0
static tButton *pFortis_4GGetButton(char *pData)
{
	tButton	*pButtons = cButtonFortis;

	return pButtons;
}
#endif

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	printf("[evremote2 fortis_4g] %s >\n", __func__);
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
	if (argc >= 2)
	{
		cLongKeyPressSupport.period = atoi(argv[1]);
	}
	if (argc >= 3)
	{
		cLongKeyPressSupport.delay = atoi(argv[2]);
	}
	printf("[evremote2 fortis 4g] Period = %d, delay = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay);
	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
#if 0
	unsigned char vData[cFortisDataLen];
	eKeyType vKeyType = RemoteControl;
	int vCurrentCode = -1;
	static int vNextKey = 0;
//	static char vOldButton = 0;

	while (1)
	{
		read(context->fd, vData, cFortisDataLen);
 #if 0
		printf("[evremote2 fortis_4g] (len %d): ", n);

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
 #if 0
		else if (vData[2] == 0x51)
		{
			vKeyType = FrontPanel;
		}
 #endif
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
//				printf("[evremote2 fortis_4g] nextFlag %d\n", vNextKey);
				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
 #if 0
		else
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[3]);

			if (vCurrentCode != 0)
			{
				vNextKey = (vOldButton != vData[3] ? vNextKey + 1 : vNextKey) % 0x100;
//				printf("[evremote2 fortis] nextFlag %d\n", vNextKey);
				vCurrentCode += (vNextKey << 16);
				break;
			}
		}
 #endif
	} /* for later use we make a dummy while loop here */
#else
#endif
	char      vBuffer[cFortisDataLen];
	char      vData[10];
	const int cSize = cFortisDataLen;
	int       vCurrentCode = -1;
	int       rc;
	tButton   *cButtons = cButtonFortis;
//	char      vKeyName[20];

	memset(vBuffer, 0, cFortisDataLen);
	// wait for new command
	rc = read(context->fd, vBuffer, cSize);

	if (rc <= 0)
	{
		return -1;
	}
#if 0	// Get predata
	vData[0] = vBuffer[8];
	vData[1] = vBuffer[9];
	vData[2] = vBuffer[10];
	vData[3] = vBuffer[11];
	vData[4] = '\0';
	printf("vBuffer = %s, predata = %s", vBuffer, vData);
	cButtons = pFortis_4GGetButton(vData);  // get table
#endif
//	cButtons = pFortis_4GGetButton();  // get table
	cButtons = cButtonFortis;

	vData[0] = vBuffer[14];  // get key
	vData[1] = vBuffer[15];  // code digits
	vData[2] = '\0';

#if 0
	for (rc = 20; rc < (20 + cFortisDataLen); rc++)
	{
		if (vBuffer[rc] == 0x20)
		{
			break;
		}
		else
		{
			vKeyName[rc - 20] = vBuffer[rc];
		}
	}
	vKeyName[rc - 20 + 1] = '\0';

	printf("[evremote2 fortis 4g] key: 0x%s -> %s\n", vData, vKeyName);
#else
	printf("[evremote2 fortis 4g] key: %s -> %s\n", vData, &vBuffer[0]);
#endif
	vCurrentCode = getInternalCode(cButtons, vData);

	if (vCurrentCode != 0)
	{
		static int nextflag = 0;

		if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
		{
			nextflag++;
		}
		vCurrentCode += (nextflag << 16);
	}
//	printf("[evremote2 fortis 4g] vCurrentCode: 0x%0x\n", vCurrentCode);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	int  ioctl_fd = -1;
	int  vFd;
	char vName[24];
	int  vLen;

	vFd = open("/proc/stb/info/model", O_RDONLY);
	vLen = read(vFd, vName, 23);
	close(vFd);

	if (vLen > 0)
	{
		vName[vLen - 1] = '\0';

//		printf("Notification: receiver = %s\n", vName);
#if 1
		if (!strncasecmp(vName, "dp6010", 6)
		||  !strncasecmp(vName, "dp7001", 6)
		||  !strncasecmp(vName, "dp7050", 6))
		{  // LED models, to be tested
			struct fortis_4g_ioctl_data vfd_data;

//			vfd_data.u.mode.compat = 1;
//			ioctl(ioctl_fd, VFDSETMODE, &vfd_data);  // switch to nuvoton mode
			vfd_data.u.led.led_nr = LED_GREEN;
			vfd_data.u.led.level = (cOn ? 1 : 0);
//			data.data[] = LED_GREEN;
//			data.data[] = (cOn ? 1 : 0);
			if (!cOn)
			{
				usleep(300000);
			}
			ioctl_fd = open("/dev/vfd", O_RDONLY);
			ioctl(ioctl_fd, VFDSETLED, &vfd_data);
//			ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &data);
			close(ioctl_fd);
		}
		else if (!strncasecmp(vName, "dp2010", 6)
		||       !strncasecmp(vName, "dp7000", 6)
		||       !strncasecmp(vName, "ep8000", 6)
		||       !strncasecmp(vName, "epp8000", 7)
		||       !strncasecmp(vName, "gpv8000", 7))
#endif
		{  // VFD models
			struct vfd_ioctl_data data;

			data.data[0] = ICON_DOT;
			data.data[4] = (cOn ? 1 : 0);
			if (!cOn)
			{
				usleep(300000);
			}
			ioctl_fd = open("/dev/vfd", O_RDONLY);
			ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &data);
			close(ioctl_fd);
		}
	}
	return 0;
}

RemoteControl_t Fortis_4G_RC =
{
	"Fortis 4G LIRC RemoteControl",
	Fortis_4G,
	cButtonFortis,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Fortis_4G_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4
