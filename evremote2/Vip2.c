/*
 * Vip2.c
 *
 * (c) 2010 duckbox project
 * (c) 2020 Audioniek
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
// TODO: add switching between RC1 and RC2

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
#include "Vip2.h"

#define	EDISION_RC1_PREDATA 0x9966  // equals Spark rc09
#define	EDISION_RC2_PREDATA 0x11EE  // equals Spark rc05

//#define STB_ID_HL101       "03:05:08"
#define STB_ID_VIP1_V1      "03:00:08"
#define STB_ID_VIP2         "03:05:08"

char VendorStbID[] = "00:00:00\0";

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120
};

/* Edision argus-vip2 RCU */
static tButton cButtonsEdisionVip2[] =
{	                                                //   yyyy = 0x9966 for RC1, and 0x11EE for RC2
	{ "POWER",       "25",  KEY_POWER },            // 0xyyyyDA25
	{ "RC12",        "A5",  KEY_R },                // 0xyyyy5AA5
	{ "V.FORMAT",    "AD",  KEY_SWITCHVIDEOMODE },  // 0xyyyy52AD
	{ "TIME",        "8D",  KEY_PROGRAM },          // 0xyyyy728D
	{ "MUTE",        "85",  KEY_MUTE },             // 0xyyyy7A85
	{ "Tms",         "E5",  KEY_TIME },             // 0xyyyy1AE5
	{ "PIP",         "ED",  KEY_SCREEN },           // 0xyyyy12ED
	{ "F1",          "CD",  KEY_F1 },               // 0xyyyy32CD
	{ "TV/SAT",      "C5",  KEY_AUX },              // 0xyyyy3AC5
	{ "1",           "B5",  KEY_1 },                // 0xyyyy4AB5
	{ "2",           "95",  KEY_2 },                // 0xyyyy6A95
	{ "3",           "BD",  KEY_3 },                // 0xyyyy42BD
	{ "4",           "F5",  KEY_4 },                // 0xyyyy0AF5
	{ "5",           "D5",  KEY_5 },                // 0xyyyy2AD5
	{ "6",           "FD",  KEY_6 },                // 0xyyyy02FD
	{ "7",           "35",  KEY_7 },                // 0xyyyyCA35
	{ "8",           "15",  KEY_8 },                // 0xyyyyEA15
	{ "9",           "3D",  KEY_9 },                // 0xyyyyC23D
	{ "TV/RADIO",    "77",  KEY_TV2 },              // 0xyyyy8877
	{ "0",           "57",  KEY_0 },                // 0xyyyyA857
	{ "RECALL",      "7F",  KEY_BACK },             // 0xyyyy807F
	{ "FIND",        "9D",  KEY_SUBTITLE },         // 0xyyyy629D
	{ "VOL-",        "DD",  KEY_VOLUMEDOWN },       // 0xyyyy22DD
	{ "PAGEDOWN",    "5F",  KEY_CHANNELDOWN },      // 0xyyyyA05F
	{ "SAT",         "1D",  KEY_SAT },              // 0xyyyyE21D
	{ "RECORD",      "45",  KEY_RECORD },           // 0xyyyyBA45
	{ "VOL+",        "C7",  KEY_VOLUMEUP },         // 0xyyyy38C7
	{ "PAGEUP",      "07",  KEY_CHANNELUP },        // 0xyyyyF807
	{ "FAV",         "87",  KEY_FAVORITES },        // 0xyyyy7887
	{ "MENU",        "65",  KEY_MENU },             // 0xyyyy9A65
	{ "INFO",        "A7",  KEY_INFO },             // 0xyyyy58A7
	{ "UP",          "27",  KEY_UP },               // 0xyyyyD827
	{ "LEFT",        "6D",  KEY_LEFT },             // 0xyyyy926D
	{ "OK",          "2F",  KEY_OK },               // 0xyyyyD02F
	{ "RIGHT",       "AF",  KEY_RIGHT },            // 0xyyyy50AF
	{ "DOWN",        "0F",  KEY_DOWN },             // 0xyyyyF00F
	{ "EXIT",        "4D",  KEY_EXIT },             // 0xyyyyB24D
	{ "EDIVISION",   "8F",  KEY_EPG },              // 0xyyyy708F
	{ "FOLDER",      "75",  KEY_FILE },             // 0xyyyy8A75
	{ "STOP",        "F7",  KEY_STOP },             // 0xyyyy08F7
	{ "PAUSE",       "37",  KEY_PAUSE },            // 0xyyyyC837
	{ "PLAY",        "B7",  KEY_PLAY },             // 0xyyyy48B7
	{ "PREVIOUS",    "55",  KEY_PREVIOUS },         // 0xyyyyAA55
	{ "NEXT",        "D7",  KEY_NEXT },             // 0xyyyy28D7
	{ "REWIND",      "17",  KEY_REWIND },           // 0xyyyyE817
	{ "FASTFORWARD", "97",  KEY_FASTFORWARD },      // 0xyyyy6897
	{ "STEP_BACK",   "5D",  KEY_SLOW },             // 0xyyyyA25D
	{ "STEP_FWD",    "DF",  KEY_F },                // 0xyyyy20DF
	{ "PLAYMODE",    "1F",  KEY_W },                // 0xyyyyE01F
	{ "USB",         "9F",  KEY_MEDIA },            // 0xyyyy609F
	{ "RED",         "7D",  KEY_RED },              // 0xyyyy827D
	{ "GREEN",       "FF",  KEY_GREEN },            // 0xyyyy00FF
	{ "YELLOW",      "3F",  KEY_YELLOW },           // 0xyyyyC03F
	{ "BLUE",        "BF",  KEY_BLUE },             // 0xyyyy40BF
	{ "",            "",    KEY_NULL }              // end of table
};

/* ***************** front panel button assignment **************** *
 *
 * Front panel keys are handled by the front panel driver
 */

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;


void Get_StbID()
{
	char *pch;
	int fn = open("/proc/cmdline", O_RDONLY);

	if (fn > -1)
	{
		char procCmdLine[1024];
		int len = read(fn, procCmdLine, sizeof(procCmdLine) - 1);
		if (len > 0)
		{
			procCmdLine[len] = 0;
			pch = strstr(procCmdLine, "STB_ID=");
			if (pch != NULL)
			{
				strncpy(VendorStbID, pch + 7, 8);
				printf("[evremote2 vip2] Vendor STB-ID = %s\n", VendorStbID);
			}
			else
			{
				printf("[evremote2 vip2] No Vendor STB-ID found\n");
			}
		}
		close(fn);
	}
}

int String2bin(const char *string)
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

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

	printf("[evremote2 vip2] %s >\n", __func__);
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
	if (! access("/etc/.rccode", F_OK))
	{
		char buf[10];
		int val;
		FILE* fd;

		fd = fopen("/etc/.rccode", "r");
		if (fd != NULL)
		{
			if (fgets(buf, sizeof(buf), fd) != NULL)
			{
				val = atoi(buf);
				if (val > 0 && val < 3)
				{
					cLongKeyPressSupport.rc_code = val;
					printf("[evremote2 vip2] Selected RC Code: %d\n", cLongKeyPressSupport.rc_code);
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
	printf("[evremote2 vip2] Period = %d, delay = %d, rc code = %d\n", cLongKeyPressSupport.period, cLongKeyPressSupport.delay, cLongKeyPressSupport.rc_code);
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
	char vData[10];
	const int cSize = 128;
	int vCurrentCode = -1;
	struct vfd_ioctl_data data;
	int rc;
	int predata;
	int ioctl_fd = -1;
	tButton *cButtons = cButtonsEdisionVip2;

	memset(vBuffer, 0, 128);
	// wait for new command
	rc = read(context->fd, vBuffer, cSize);
	if (rc <= 0)
	{
		return -1;
	}
	// determine predata (RC1 or RC2)
	vData[0] = vBuffer[8];
	vData[1] = vBuffer[9];
	vData[2] = vBuffer[10];
	vData[3] = vBuffer[11];
	vData[4] = '\0';
	predata = String2bin(vData);

	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';
//	printf("[evremote2 vip2] Key: %s -> %s\n", vData, &vBuffer[0]);
	printf("[evremote2 vip2] Key: %s\n", vData);
	vCurrentCode = getInternalCode(cButtons, vData);

	if (((vData[0] == 'A') || (vData[0] == 'a')) && vData[1] == '5')
	{
		if (vBuffer[17] == '0' && vBuffer[18] == '0')  // RC12 key pressed
		{
			if (! access("/etc/.rccode", F_OK))
			{
				char buf[2];
				int fd;

				memset(buf, 0, sizeof(buf));

				fd = open("/etc/.rccode", O_RDWR);
				if (fd >= 0)
				{
					if (read(fd, buf, 1) == 1)
					{
						rc = 0x31;
						if (predata == EDISION_RC2_PREDATA)
						{
							rc++;
						}
						buf[0] = rc;
						lseek (fd, 0, SEEK_SET);
						if (write(fd, buf, 1) == 1)
						{
							context->r->LongKeyPressSupport->rc_code = rc & 0x03;
							rc = 0x31;
						}
					}
					else
					{
						context->r->LongKeyPressSupport->rc_code = 1;  // set default RC code
						rc = 0x31;
					}
					printf("[evremote2 vip2] RC Code set to: %c\n", rc);
	
					data.length = sprintf((char *)data.data, "Code RC%c\n", rc);
					data.length--;
					data.data[data.length] = 0;
					ioctl_fd = open("/dev/vfd", O_RDONLY);
					ioctl(ioctl_fd, VFDDISPLAYCHARS, &data);
					close(ioctl_fd);
				}	
				else
				{
					context->r->LongKeyPressSupport->rc_code = 1;  // set default RC code
				}
				close(fd);
			}
		}
		vCurrentCode = 0;  // ignore RC12 key
	}
	else
	{
//		printf("[evremote2 vip2] predata = 0x%04x, rc code = %d\n", predata, context->r->LongKeyPressSupport->rc_code);
		if ((context->r->LongKeyPressSupport->rc_code == 1 && predata == EDISION_RC1_PREDATA)
		||  (context->r->LongKeyPressSupport->rc_code == 2 && predata == EDISION_RC2_PREDATA))
		{
			if (vCurrentCode != 0)
			{
				static int nextflag = 0;
		
				if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
				{
					nextflag++;
				}
				vCurrentCode += (nextflag << 16);
			}
		}
		else
		{
//			printf("[evremote2 vip2] Mismatch: predata = 0x%04x, rc code = %d\n", predata, context->r->LongKeyPressSupport->rc_code);
			vCurrentCode = 0;  // ignore key
		}
	}
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	struct aotom_ioctl_data vfd_data;
	int ioctl_fd = -1;

	vfd_data.u.icon.on = cOn ? 1 : 0;
	vfd_data.u.icon.icon_nr = 35;
	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
	close(ioctl_fd);
	return 0;
}

RemoteControl_t Vip2_RC =
{
	"Vip2 RemoteControl",
	Vip2,
	cButtonsEdisionVip2,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Vip2_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

