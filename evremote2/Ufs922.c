/*
 * Ufs922.c
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
#include "Ufs922.h"

/* ***************** some constants **************** */

#define rcDeviceName "/dev/rc"
#define cUFS922CommandLen 8

typedef struct
{
	int toggleFeedback;
	int disableFeedback;
} tUFS922Private;

int wheelmode = 0;  // default is list mode

/* ***************** our key assignment **************** */

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120,
};

static tButton cButtonUFS922[] =
{  // Table is for RC670
	{ "HELP",            "81", KEY_HELP },
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
	{ "ARCHIV",          "46", KEY_FILE },
	{ "LEFT",            "5A", KEY_LEFT },
	{ "OK",              "5C", KEY_OK },
	{ "RIGHT",           "5B", KEY_RIGHT },
	{ "EXIT",            "55", KEY_EXIT },
	{ "DOWN",            "59", KEY_DOWN },
	{ "MEDIA",           "D5", KEY_MEDIA },
	{ "REWIND",          "21", KEY_REWIND },
	{ "PLAY",            "38", KEY_PLAY },
	{ "FASTFORWARD",     "20", KEY_FASTFORWARD },
	{ "PAUSE",           "39", KEY_PAUSE },
	{ "RECORD",          "37", KEY_RECORD },
	{ "STOP",            "31", KEY_STOP },
	{ "",                "",   KEY_NULL }
};

/* ***************** our fp button assignment **************** */

static tButton cButtonUFS922ListFrontpanel[] =
{
	{ "FP_REC",          "80", KEY_RECORD },
	{ "FP_STOP",         "0D", KEY_STOP },
	{ "FP_AUX",          "20", KEY_AUX },
	{ "FP_TV_R",         "08", KEY_TV2 },

	{ "FP_WHEEL_PRESS",  "04", KEY_OK },
	{ "FP_WHEEL_LEFT",   "0F", KEY_UP },
	{ "FP_WHEEL_RIGHT",  "0E", KEY_DOWN },
	{ "",                "",   KEY_NULL }
};

static tButton cButtonUFS922VolumeFrontpanel[] =
{
	{ "FP_REC",          "80", KEY_RECORD },
	{ "FP_STOP",         "0D", KEY_STOP },
	{ "FP_AUX",          "20", KEY_AUX },
	{ "FP_TV_R",         "08", KEY_TV2 },

	{ "FP_WHEEL_PRESS",  "04", KEY_OK },
	{ "FP_WHEEL_LEFT",   "0F", KEY_VOLUMEDOWN },
	{ "FP_WHEEL_RIGHT",  "0E", KEY_VOLUMEUP },
	{ "",                "",   KEY_NULL }
};


#if 0
static int ufs922SetRemote(unsigned int code)
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
#endif

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vFd;
	tUFS922Private *private = malloc(sizeof(tUFS922Private));

	context->r->private = private;
	vFd = open(rcDeviceName, O_RDWR);
	memset(private, 0, sizeof(tUFS922Private));
	if (argc >= 2)
	{
		private->toggleFeedback = atoi(argv[1]);
	}
	else
	{
		private->toggleFeedback = 1;
	}
	if (argc >= 3)
	{
		private->disableFeedback = atoi(argv[2]);
	}
	else
	{
		private->disableFeedback = 1;
	}
	printf("[evremote2 ufs922] Toggle = %d, disable feedback = %d\n", private->toggleFeedback, private->disableFeedback);
	if (argc >= 4)
	{
		cLongKeyPressSupport.period = atoi(argv[3]);
	}
	if (argc >= 5)
	{
		cLongKeyPressSupport.delay = atoi(argv[4]);
	}
#if 0
	if (! access("/etc/.rccode", F_OK))
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
					printf("[evremote2 ufs922] Selected RC Code: %d\n", cLongKeyPressSupport.rc_code);
					ufs922SetRemote(cLongKeyPressSupport.rc_code);
				}
			}
			fclose(fd);
		}
	}
	else
	{
		ufs922SetRemote(1);  // default to 1
	}
#endif
	printf("[evremote2 ufs922] Period = %d, delay = %d, rc_code = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay, cLongKeyPressSupport.rc_code);
	printf("[evremote2 ufs922] private->disableFeedback = %d\n", private->disableFeedback);

	if (private->disableFeedback == 0)
	{
		struct micom_ioctl_data vfd_data;
		int ioctl_fd;

		ioctl_fd = open("/dev/vfd", O_RDONLY);
		vfd_data.u.led.led_nr = 6;  // wheel symbols
		vfd_data.u.led.on = 1;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		vfd_data.u.led.led_nr = (wheelmode ? 5 : 2);  // wheel text: VOLUME on in wheelmode = 1
		vfd_data.u.led.on = 1;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		vfd_data.u.led.led_nr = (wheelmode ? 2 : 5);  // wheel text: LIST on in wheelmode = 0
		vfd_data.u.led.on = 0;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}
	return vFd;
}

static int pShutdown(Context_t *context)
{
	tUFS922Private *private = (tUFS922Private *)context->r->private;

	close(context->fd);
	if (private->disableFeedback == 0)
	{
		struct micom_ioctl_data vfd_data;
		int ioctl_fd = open("/dev/vfd", O_RDONLY);

		vfd_data.u.led.led_nr = 6;
		vfd_data.u.led.on = 0;
		ioctl(ioctl_fd, VFDSETLED, &vfd_data);
		close(ioctl_fd);
	}
	free(private);
	return 0;
}

static int pRead(Context_t *context)
{
	unsigned char vData[cUFS922CommandLen];
	eKeyType vKeyType = RemoteControl;
	int vCurrentCode = -1;
//	int rc = 1;
	int ioctl_fd = -1;
	struct micom_ioctl_data vfd_data;

//	printf("%s >\n", __func__);
	while (1)
	{
		read(context->fd, vData, cUFS922CommandLen);

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
#if 0
			/* mask out for rc codes
			 * possible 0 to 3 for remote controls 1 to 4
			 * given in /etc for example via console: echo 2 > /etc/.rccode
			 * will be read and rc_code = 2 is used ( press then back + 2 simultanessly on remote to fit it there)
			 * default is rc_code = 1 ( like back + 1 on remote )	*/
			rc = ((vData[4] & 0x30) >> 4) + 1;
			printf("[evremote2 ufs922] RC code: %d\n", rc);
			if (rc == ((RemoteControl_t *)context->r)->LongKeyPressSupport->rc_code)
			{
				vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[1]);
			}
			else
			{
				break;
			}
#else
			vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[1]);
#endif
		}
		else
		{
			vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
#if 0
			printf("[evremote_ufs922] Frontpanel key, vCurrentCode = 0x%03x\n", vCurrentCode);
			if (vCurrentCode == KEY_OK)
			{
				printf("[evremote_ufs922] Toggle wheel mode\n");
				wheelmode = (wheelmode ? 0 : 1);
				vfd_data.u.led.led_nr = (wheelmode ? 5 : 2);
				vfd_data.u.led.on = 0;
				ioctl_fd = open("/dev/vfd", O_RDONLY);
				ioctl(ioctl_fd, VFDSETMODE, &vfd_data);
				ioctl(ioctl_fd, VFDSETLED, &vfd_data);
				usleep(10000);
				vfd_data.u.led.led_nr = (wheelmode ? 2 : 5);
				vfd_data.u.led.on = 1;
				ioctl_fd = open("/dev/vfd", O_RDONLY);
				ioctl(ioctl_fd, VFDSETMODE, &vfd_data);
				ioctl(ioctl_fd, VFDSETLED, &vfd_data);
				close(ioctl_fd);
				if (wheelmode)
				{
					printf("[[evremote_ufs922] Wheel in Volume mode\n");
					context->r->Frontpanel = cButtonUFS922VolumeFrontpanel;
				}
				else
				{
					printf("[evremote_ufs922] Wheel in List mode\n");
					context->r->Frontpanel = cButtonUFS922ListFrontpanel;
				}
			}
#endif
		}
		if (vCurrentCode != 0)
		{
			unsigned int vNextKey = vData[4];
			vCurrentCode += (vNextKey << 16);
			break;
		}
	}
//	printf("%s <\n", __func__);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	int ioctl_fd = -1;
	struct micom_ioctl_data vfd_data;
	tUFS922Private *private = (tUFS922Private *)((RemoteControl_t *)context->r)->private;

//	if (private->disableFeedback == 0)
//	{
//		printf("[evremote_ufs922] Notification is %s (%d)\n", (private->disableFeedback != 0 ? "off": "on"), private->disableFeedback);
//		return 0;
//	}
	vfd_data.u.led.led_nr = 6;  // wheel
	if (cOn)
	{
		vfd_data.u.led.on = (private->toggleFeedback ? 0 : 1);
	}
	else
	{
		usleep(100000);
		vfd_data.u.led.on = (private->toggleFeedback ? 1 : 0);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
//	vfd_data.u.mode.compat = 1;
//	ioctl(ioctl_fd, VFDSETMODE, &vfd_data);
	printf("[evremote_ufs922] Set LED %d to %s\n", vfd_data.u.led.led_nr, (vfd_data.u.led.on == 0 ? "off" : "on"));
	ioctl(ioctl_fd, VFDSETLED, &vfd_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t UFS922_RC =
{
	"Kathrein UFS922 RemoteControl",
	Ufs922,
	cButtonUFS922,
	cButtonUFS922ListFrontpanel,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t UFS922_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4
