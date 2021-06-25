/*
 * Ufs913.c
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
#include "Ufs913.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cUFS913CommandLen 8

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120, 1
};

static tButton cButtonUFS913[] =  // order is same as on RC671
{
	{ "TVRADIO",         "81", KEY_TV2 },
	{ "POWER",           "0C", KEY_POWER },
	{ "1",               "01", KEY_1 },
	{ "2",               "02", KEY_2 },
	{ "3",               "03", KEY_3 },
	{ "4",               "04", KEY_4 },
	{ "5",               "05", KEY_5 },
	{ "6",               "06", KEY_6 },
	{ "7",               "07", KEY_7 },
	{ "8",               "08", KEY_8 },
	{ "9",               "09", KEY_9 },
	{ "MENU",            "54", KEY_MENU },
	{ "0",               "00", KEY_0 },
	{ "TEXT",            "3C", KEY_TEXT },
	{ "VOLUMEDOWN",      "11", KEY_VOLUMEDOWN },
	{ "CHANNELUP",       "1E", KEY_CHANNELUP },
	{ "VOLUMEUP",        "10", KEY_VOLUMEUP },
	{ "MUTE",            "0D", KEY_MUTE },
	{ "CHANNELDOWN",     "1F", KEY_CHANNELDOWN },
	{ "INFO",            "0F", KEY_INFO },
	{ "RED",             "6D", KEY_RED },
	{ "GREEN",           "6E", KEY_GREEN },
	{ "YELLOW",          "6F", KEY_YELLOW },
	{ "BLUE",            "70", KEY_BLUE },
	{ "EPG",             "CC", KEY_EPG },
	{ "UP",              "58", KEY_UP },
	{ "MEDIA",           "46", KEY_MEDIA },
	{ "LEFT",            "5A", KEY_LEFT },
	{ "OK",              "5C", KEY_OK },
	{ "RIGHT",           "5B", KEY_RIGHT },
	{ "EXIT",            "55", KEY_EXIT },
	{ "DOWN",            "59", KEY_DOWN },
	{ "PORTAL",          "D5", KEY_WWW },
	{ "REWIND",          "21", KEY_REWIND },
	{ "PLAY",            "38", KEY_PLAY },
	{ "FASTFORWARD",     "20", KEY_FASTFORWARD },
	{ "PAUSE",           "39", KEY_PAUSE },
	{ "RECORD",          "37", KEY_RECORD },
	{ "STOP",            "31", KEY_STOP },
	{ "",                "",   KEY_NULL }
};

/* ***************** our fp button assignment **************** */

static tButton cButtonUFS913Frontpanel[] =
{
	{ "FP_MEDIA",        "80", KEY_MEDIA },
	{ "FP_ON_OFF",       "01", KEY_POWER },
	{ "FP_MINUS",        "04", KEY_DOWN },
	{ "FP_PLUS",         "02", KEY_UP },
	{ "FP_TV_R",         "08", KEY_TV2 },
	{ "",                "",   KEY_NULL }
};


static int ufs913SetRemote(unsigned int code)
{
	int vfd_fd = -1;
	struct
	{
		unsigned char start;
		unsigned char data[64];
		unsigned char length;
	} data;

	data.start = 0x00;
	data.data[0] = code & 0x07;
	data.length = 1;

	vfd_fd = open("/dev/vfd", O_RDWR);
	if (vfd_fd)
	{
		ioctl(vfd_fd, VFDSETRCCODE, &data);
		close(vfd_fd);
	}
	return 0;
}

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
	if (!access("/etc/.rccode", F_OK))
	{
		char buf[10];
		int val;
		FILE* fd;
		fd = fopen("/etc/.rccode", "r");
		if (fd != NULL)
		{
			if (fgets (buf, sizeof(buf), fd) != NULL)
			{
				val = atoi(buf);
				if (val > 0 && val < 5)
				{
					cLongKeyPressSupport.rc_code = val;
					printf("[evremote2 ufs913] Selected RC Code: %d\n", cLongKeyPressSupport.rc_code);
					ufs913SetRemote(cLongKeyPressSupport.rc_code);
				}
			}
			fclose(fd);
		}
	}
	printf("[evremote2 ufs913] Period = %d, delay = %d, rc_code = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay, cLongKeyPressSupport.rc_code);
	return vFd;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
	unsigned char vData[cUFS913CommandLen];
	eKeyType vKeyType = RemoteControl;
	int vCurrentCode = -1;
	int rc = 1;

//	printf("%s >\n", __func__);
	while (1)
	{
		read(context->fd, vData, cUFS913CommandLen);
		if (vData[0] == 0xD2)
		{
			vKeyType = RemoteControl;
		}
		else if (vData[0] == 0xD1)
		{
			vKeyType = FrontPanel;
		}
		else
		{
			continue;
		}
		if (vKeyType == RemoteControl)
		{
			/* mask out for rc codes
			 * possible 0 to 3 for remote controls 1 to 4
			 * given in /etc for example via console: echo 2 > /etc/.rccode
			 * will be read and rc_code = 2 is used ( press then back + 2 simultanessly on remote to fit it there)
			 * default is rc_code = 1 ( like back + 1 on remote )	*/
			rc = ((vData[4] & 0x30) >> 4) + 1;
			printf("[evremote2 ufs913] RC code: %d\n", rc);
			if (rc == ((RemoteControl_t *)context->r)->LongKeyPressSupport->rc_code)
			{
				vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[1]);
			}
			else
			{
				break;
			}
		}
		else
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
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

RemoteControl_t UFS913_RC =
{
	"Kathrein UFS913 RemoteControl",
	Ufs913,
	cButtonUFS913,
	cButtonUFS913Frontpanel,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t UFS913_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

