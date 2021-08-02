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
int press_count = 0;
int rotation_count = 0;
int wheel_press = 0;
int wheel_rotation = 0;
int long_press = 0;

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

	{ "FP_WHEEL_PRESS",  "04", KEY_OK },  // Suppressed, unless pressed long
	{ "FP_WHEEL_LEFT",   "0F", KEY_UP },  //	{ "FP_WHEEL_LEFT",   "0F", KEY_VOLUMEDOWN },
	{ "FP_WHEEL_RIGHT",  "0E", KEY_DOWN },  //	{ "FP_WHEEL_RIGHT",  "0E", KEY_VOLUMEUP },
	{ "",                "",   KEY_NULL }  // end of table
};


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
				}
				else
				{
					cLongKeyPressSupport.rc_code = 1;  // set default RC code
				}
			}
			fclose(fd);
		}
	}
	else
	{
		cLongKeyPressSupport.rc_code = 1;  // set default RC code
	}
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
	int rc;
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
			if (vData[1] == 0x74 || vData[1] == 0x75 || vData[1] == 0x76 || vData[1] == 0x77)  // set RC code received (BACK + 9 + 1..4)
			{
				rc = vData[1] - 0x73;
//				printf("[evremote2 ufs922] Change RC code command received (code = %d)\n", rc);

				if (! access("/etc/.rccode", F_OK))
				{
					char buf[2];
					int fd;

					memset(buf, 0, sizeof(buf));
					buf[0] = (rc | 0x30);

					fd = open("/etc/.rccode", O_WRONLY);
					if (fd >= 0)
					{
						if (write(fd, buf, 1) == 1)
						{
							context->r->LongKeyPressSupport->rc_code = rc;
						}
						else
						{
							context->r->LongKeyPressSupport->rc_code = rc = 1;  // set default RC code
						}
						printf("[evremote2 ufs922] RC Code set to: %d\n", rc);
					}
					else
					{
						context->r->LongKeyPressSupport->rc_code = 1;  // set default RC code
					}
					close(fd);
				}
			}
			/* mask out for rc codes
			 * possible 0 to 3 for remote controls 1 to 4
			 * given in /etc for example via console: echo 2 > /etc/.rccode
			 * will be read and rc_code = 2 is used ( press then back + 2 simultaniously on remote to fit it there)
			 * default is rc_code = 1 ( like back + 1 on remote )	*/
			rc = ((vData[4] & 0x30) >> 4) + 1;
//			printf("[evremote2 ufs922] RC code of received key: %d\n", rc);
			if (rc == context->r->LongKeyPressSupport->rc_code)
			{
				vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[1]);
			}
			else
			{
				break;
			}
			vCurrentCode = getInternalCodeHex(context->r->RemoteControl, vData[1]);
		}
		else
		{
			// handle wheel press
			if (vData[1] == 0x04)  // FP_WHEEL_PRESS
			{
				if (long_press == 1)  // if count down finished, wheel is still pressed -> echo key code
				{
//					printf("Long wheel press detected\n");
					long_press = 0;
					vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
					goto out;
				}
				long_press = 0;
				wheel_rotation = 0;
				rotation_count = 0;
				if (wheel_press == 0)
				{
//					printf("[evremote_ufs922] Toggle wheel mode\n");
					wheelmode = (wheelmode ? 0 : 1);
					vfd_data.u.led.led_nr = (wheelmode ? 2 : 5);
					vfd_data.u.led.on = 0;
					ioctl_fd = open("/dev/vfd", O_RDONLY);
					ioctl(ioctl_fd, VFDSETLED, &vfd_data);
					usleep(100000);
					vfd_data.u.led.led_nr = (wheelmode ? 5 : 2);
					vfd_data.u.led.on = 1;
					ioctl(ioctl_fd, VFDSETLED, &vfd_data);
					close(ioctl_fd);
//					printf("[[evremote_ufs922] Wheel in %s mode\n", (wheelmode ? "Volume" : "List"));
					wheel_press = 1;
					press_count = 5;
				}
				if (press_count)  // ignore long wheel presses
				{
					press_count--;
				}
				else
				{
					if (wheel_press)
					{
						long_press = 1;  // flag count down finished -> long press if next key is also FP_WHEEL_PRESS
					}
					wheel_press = 0;
				}
				break;
			}
			else  // handle wheel rotation
			{
				long_press = 0;
				wheel_press = 0;
				press_count = 0;
				if (vData[1] == 0x0e || vData[1] == 0x0f)  // wheel is rotated
				{
					if (wheel_rotation == 0)
					{
						vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
						rotation_count = 1;  // use every 2nd click
						if (wheelmode)  // if wheel in Volume mode
						{
							if (vData[1] == 0x0f)  // FP_WHEEL_LEFT
							{
								 vCurrentCode = KEY_VOLUMEDOWN;
							}
							else
							{
								rotation_count = 3;  // use every 4th click
								vCurrentCode = KEY_VOLUMEUP;
							}
						}
						wheel_rotation = 1;
					}
					if (rotation_count)
					{
						rotation_count--;
						break;  // ignore fast wheel rotations
					}
					else
					{
						wheel_rotation = 0;
					}
				}
				else  // handle other front panel keys
				{
					vCurrentCode = getInternalCodeHex(context->r->Frontpanel, vData[1]);
				}
			}			
			printf("[evremote_ufs922] Frontpanel key, vCurrentCode = 0x%02x\n", vCurrentCode);
		}

out:
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

#if 0
	if (private->disableFeedback == 0)
	{
		printf("[evremote_ufs922] Notification is %s (%d)\n", (private->disableFeedback != 0 ? "off": "on"), private->disableFeedback);
		return 0;
	}
#endif
	vfd_data.u.led.led_nr = 6;  // wheel symbols
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
//	printf("[evremote_ufs922] Set LED %d to %s\n", vfd_data.u.led.led_nr, (vfd_data.u.led.on == 0 ? "off" : "on"));
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
