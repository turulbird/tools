/*
 * evremote.c
 *
 * (c) 2009 donald@teamducktales
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
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include <semaphore.h>
#include <pthread.h>

#include "global.h"
#include "remotes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


static unsigned int gBtnPeriod  = 100;
static unsigned int gBtnDelay = 10;

static unsigned int cmdBtnPeriod  = 0;
static unsigned int cmdBtnDelay = 0;

static struct timeval profilerLast;

static unsigned int gKeyCode = 0;
static unsigned int gNextKey = 0;
static unsigned int gNextKeyFlag = 0xFF;

static sem_t keydown_sem;
static pthread_t keydown_thread;

static bool countFlag = false;

static unsigned char keydown_sem_helper = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int processSimple(Context_r_t *context_r, Context_t *context, int argc, char *argv[])
{
	int vCurrentCode = -1;

//	printf("[evremote2] %s >\n", __func__);
	if ((context_r->br)->Init)
	{
		context->fd = context_r->br->Init(context, argc, argv);
	}
	else
	{
		fprintf(stderr, "[evremote2] Subdriver does not support init function\n");
		exit(1);
	}
	if (context->fd < 0)
	{
		fprintf(stderr, "[evremote2] Error in device initialization\n");
		exit(1);
	}
	while (true)
	{
		// wait for new command
		if (context_r->br->Read)
		{
			vCurrentCode = context_r->br->Read(context);
		}
		if (vCurrentCode <= 0)
		{
			continue;
		}
		// activate visual notification
		if (context_r->br->Notification)
		{
			context_r->br->Notification(context, 1);		}
		// Check if tuxtxt is running
		if (checkTuxTxt(vCurrentCode) == false)
		{
			sendInputEvent(vCurrentCode);
		}
		// deactivate visual notification
		if (((BoxRoutines_t *)context_r->br)->Notification)
		{
			((BoxRoutines_t *)context_r->br)->Notification(context, 0);
		}
	}
	if (context_r->br->Shutdown)
	{
		context_r->br->Shutdown(context);
	}
	else
	{
		close(context->fd);
	}
//	printf("[evremote2] %s <\n", __func__);
	return 0;
}

static void sem_up(void)
{
	if (keydown_sem_helper == 0)
	{
//		printf("[evremote2] SEM up\n");
		sem_post(&keydown_sem);
		keydown_sem_helper = 1;
	}
}

static void sem_down(void)
{
	keydown_sem_helper = 0;
//	printf("[evremote2] SEM down\n");
	sem_wait(&keydown_sem);
}

int timeval_subtract(result, x, y)
struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec)
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000)
	{
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	/* Compute the time remaining to wait.
	   tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

static long diffMilli(struct timeval from, struct timeval to)
{
	struct timeval diff;

	timeval_subtract(&diff, &to, &from);
	return (long)(diff.tv_sec * 1000 + diff.tv_usec / 1000);
}

void *detectKeyUpTask(void *dummy);

int processComplex(Context_r_t *context_r, Context_t *context, int argc, char *argv[])
{
	int vCurrentCode = -1;
	unsigned int diffTime = 0;
	unsigned int waitTime = 0;
	int keyCount = 0;
	bool newKey = false;
	bool startFlag = false;

//	printf("[evremote2] %s >\n", __func__);
	if (context_r->br->Init)
	{
		context->fd = context_r->br->Init(context, argc, argv);
	}
	else
	{
		fprintf(stderr, "[evremote2] Subdriver does not support init function\n");
		exit(1);
	}
	if (context->fd < 0)
	{
		fprintf(stderr, "[evremote2] Error in device initialization\n");
		exit(1);
	}
	if (context->r->LongKeyPressSupport != NULL)
	{
		tLongKeyPressSupport lkps = *context->r->LongKeyPressSupport;
		gBtnPeriod = (cmdBtnPeriod) ? cmdBtnPeriod : lkps.period;
		gBtnDelay = (cmdBtnDelay) ? cmdBtnDelay : lkps.delay;
		printf("[evremote2] Using period = %d delay = %d\n", gBtnPeriod, gBtnDelay);
	}
	if (cmdBtnPeriod && cmdBtnDelay && (cmdBtnPeriod + cmdBtnDelay) < 200)
	{
		setInputEventRepeatRate(500, cmdBtnPeriod + cmdBtnDelay);
	}
	else
	{
		setInputEventRepeatRate(500, 200);
	}
	sem_init(&keydown_sem, 0, 0);
	pthread_create(&keydown_thread, NULL, detectKeyUpTask, context_r);

	struct timeval time;
	gettimeofday(&profilerLast, NULL);

	while (true)
	{
		unsigned int nextKeyFlag;

		// wait for new command
		if (context_r->br->Read)
		{
			vCurrentCode = context_r->br->Read(context);
		}
		if (vCurrentCode <= 0)
		{
			continue;
		}
		gKeyCode = vCurrentCode & 0xFFFF;
		nextKeyFlag = (vCurrentCode >> 16) & 0xFFFF;
		if (gNextKeyFlag != nextKeyFlag)
		{
			gNextKey++;
			gNextKey %= 20;
			gNextKeyFlag = nextKeyFlag;
			newKey = true;
		}
		else
		{
			newKey = false;
		}
		gettimeofday(&time, NULL);
		diffTime = (unsigned int)diffMilli(profilerLast, time);
//		printf("[evremote2] **** %12u %d ****\n", diffTime, gNextKey);
		profilerLast = time;

		if (countFlag)
		{
			waitTime += diffTime;
		}
		if (countFlag && newKey && gKeyCode == 0x74)
		{
			if (waitTime < 10000)  // reboot if pressing 5 times power within 10 seconds
			{
				keyCount += 1;
				printf("[evremote2] Power Count = %d\n", keyCount);
				if (keyCount >= 5)
				{
					countFlag = false;
					keyCount = 0;
					waitTime = 0;
					printf("[evremote2] > Emergency REBOOT !!!\n");
					fflush(stdout);
					system("init 6");
					sleep(4);
					reboot(LINUX_REBOOT_CMD_RESTART);
				}
			}
			else  // release reboot counter
			{
				countFlag = false;
				keyCount = 0;
				waitTime = 0;
			}
		}
		else if (countFlag && gKeyCode == 0x74)
		{
			countFlag = true;
		}
		else
		{
			countFlag = false;
		}
		if (startFlag && newKey && gKeyCode == 0x74 && diffTime < 1000)  // KEY_POWER > reboot counter enabled
		{
			countFlag = true;
			waitTime = diffTime;
			keyCount = 1;
			printf("[evremote2] Power Count = %d\n", keyCount);
		}
		if (gKeyCode == 0x160)  // KEY_OK > initiates reboot counter when pressing power within 1 second
		{
			startFlag = true;
		}
		else
		{
			startFlag = false;
		}
		sem_up();
	}
	if (context_r->br->Shutdown)
	{
		context_r->br->Shutdown(context);
	}
	else
	{
		close(context->fd);
	}
//	printf("[evremote2] %s <\n", __func__);
	return 0;
}

void *detectKeyUpTask(void *dummy)
{
	Context_r_t *context_r = (Context_r_t *)dummy;
	Context_t *context = (Context_t *)dummy;
	struct timeval time;

	while (1)
	{
		unsigned int keyCode = 0;
		unsigned int nextKey = 0;
		sem_down();  // Wait until the next keypress

		while (1)
		{
			int tux;

			tux = 0;
			keyCode = gKeyCode;
			nextKey = gNextKey;

			// activate visual notification
			if (context_r->br->Notification)
			{
				context_r->br->Notification(context, 1);
			}
			printf("[evremote2] KEY_PRESS   - %02X %d\n", keyCode, nextKey);

			// Check if tuxtxt is running
			tux = checkTuxTxt(keyCode);

			if (tux == false && !countFlag)
			{
				sendInputEventT(INPUT_PRESS, keyCode);
			}
			// usleep(gBtnDelay*1000);
			while (gKeyCode && nextKey == gNextKey)
			{
				unsigned int sleep;

				gettimeofday(&time, NULL);
				sleep = gBtnPeriod + gBtnDelay - diffMilli(profilerLast, time);

				if (sleep > (gBtnPeriod + gBtnDelay))
				{
					sleep = (gBtnPeriod + gBtnDelay);
				}
//				printf("[evremote2] ++++ %12u ms ++++\n", (unsigned int)diffMilli(profilerLast, time));
				gKeyCode = 0;
				usleep(sleep * 1000);
			}
			printf("[evremote2] KEY_RELEASE - %02X gKeycode=%02X nextKey=%02X gNextKey=%02X CAUSE: %s\n", keyCode, gKeyCode, nextKey, gNextKey, (gKeyCode == 0) ? "Timeout" : "New key");

			// Check if tuxtxt is running
			if (tux == false && !countFlag)
			{
				sendInputEventT(INPUT_RELEASE, keyCode);
			}
			// deactivate visual notification
			if (context_r->br->Notification)
			{
				context_r->br->Notification(context, 0);
			}
			gettimeofday(&time, NULL);
//			printf("[evremote2] ---- %12u ms ----\n", (unsigned int)diffMilli(profilerLast, time));
			if (nextKey == gNextKey)
			{
				break;
			}
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

int getKathreinUfs910BoxType()
{
	char vType;
	int vFdBox = open("/proc/boxtype", O_RDONLY);

	read(vFdBox, &vType, 1);
	close(vFdBox);
	return vType == '0' ? 0 : vType == '1' || vType == '3' ? 1 : -1;
}

int getModel()
{
	int vFd = -1;
	const int cSize = 128;
	char vName[129] = "LircdName";
	int vLen = -1;
	eBoxType vBoxType = Unknown;

	vFd = open("/proc/stb/info/model", O_RDONLY);
	vLen = read(vFd, vName, cSize);
	close(vFd);

	if (vLen > 0)
	{
		vName[vLen - 1] = '\0';
		printf("[evremote2] Model: '%s'\n", vName);
		if (!strncasecmp(vName, "ufs910", 6))
		{
			switch (getKathreinUfs910BoxType())
			{
				case 0:
				{
					vBoxType = Ufs910_1W;
					break;
				}
				case 1:
				{
					vBoxType = Ufs910_14W;
					break;
				}
				default:
				{
					vBoxType = Unknown;
					break;
				}
			}
		}
		else if (!strncasecmp(vName, "ufs922", 6))
		{
			vBoxType = Ufs922;
		}
		else if (!strncasecmp(vName, "ufc960", 6))
		{
			vBoxType = Ufc960;
		}
		else if (!strncasecmp(vName, "tf7700hdpvr", 11))
		{
			vBoxType = Tf7700;
		}
		else if (!strncasecmp(vName, "hl101", 5))
		{
			vBoxType = Hl101;
		}
		else if (!strncasecmp(vName, "vip1-v2", 7))
		{
			vBoxType = Vip2;
		}
		else if (!strncasecmp(vName, "vip2-v1", 7))
		{
			vBoxType = Vip2;
		}
		else if ((!strncasecmp(vName, "hdbox", 5))
		     ||  (!strncasecmp(vName, "atevio7500", 10))
		     ||  (!strncasecmp(vName, "octagon1008", 11))
		     ||  (!strncasecmp(vName, "hs7110", 6))
		     ||  (!strncasecmp(vName, "hs7420", 6))
		     ||  (!strncasecmp(vName, "hs7810a", 7))
		     ||  (!strncasecmp(vName, "hs7119", 6))
		     ||  (!strncasecmp(vName, "hs7429", 6))
		     ||  (!strncasecmp(vName, "hs7819", 6)))
		{
			vBoxType = Fortis;
		}
		else if ((!strncasecmp(vName, "dp2010", 6))  // Fortis 4G models use LIRC
		     ||  (!strncasecmp(vName, "dp6010", 6))
		     ||  (!strncasecmp(vName, "dp7000", 6))
		     ||  (!strncasecmp(vName, "dp7001", 6))
		     ||  (!strncasecmp(vName, "dp7050", 6))
		     ||  (!strncasecmp(vName, "ep8000", 6))
		     ||  (!strncasecmp(vName, "epp8000", 7))
		     ||  (!strncasecmp(vName, "fx6010", 6))
		     ||  (!strncasecmp(vName, "gpv8000", 7)))
		{
			vBoxType = Fortis_4G;
		}
		else if ((!strncasecmp(vName, "atemio520", 9))
		     ||  (!strncasecmp(vName, "atemio530", 9)))
		{
			vBoxType = CNBox;
		}
		else if (!strncasecmp(vName, "hs5101", 6))
		{
			vBoxType = Hs5101;
		}
		else if ((!strncasecmp(vName, "adb_box", 7))
		     ||  (!strncasecmp(vName, "sagemcom88", 10))
		     ||  (!strncasecmp(vName, "esi_88", 6))
		     ||  (!strncasecmp(vName, "esi88", 5))
		     ||  (!strncasecmp(vName, "dsi87", 5)))
		{
			vBoxType = Adb_Box;
		}
		else if ((!strncasecmp(vName, "ipbox9900", 9))
		     ||  (!strncasecmp(vName, "ipbox99", 7))
		     ||  (!strncasecmp(vName, "ipbox55", 7)))
		{
			vBoxType = Ipbox;
		}
		else if (!strncasecmp(vName, "ufs912", 5))
		{
			vBoxType = Ufs912;
		}
		else if (!strncasecmp(vName, "ufs913", 5))
		{
			vBoxType = Ufs912;
		}
		else if ((!strncasecmp(vName, "spark", 5))
		     ||  (!strncasecmp(vName, "spark7162", 9)))
		{
			vBoxType = Spark;
		}
		else if ((!strncasecmp(vName, "cuberevo", 8))
		     ||  (!strncasecmp(vName, "cuberevo-mini", 13))
		     ||  (!strncasecmp(vName, "cuberevo-mini2", 14))
		     ||  (!strncasecmp(vName, "cuberevo-mini-fta", 17))
		     ||  (!strncasecmp(vName, "cuberevo-250hd", 14))
		     ||  (!strncasecmp(vName, "cuberevo-2000hd", 15))
		     ||  (!strncasecmp(vName, "cuberevo-9500hd", 15))
		     ||  (!strncasecmp(vName, "cuberevo-3000hd", 14)))
		{
			vBoxType = Cuberevo;
		}
		else if (!strncasecmp(vName, "vitamin_hd5000", 14))
		{
			vBoxType = VitaminHD5000;
		}
		else /* for other boxes we use LircdName driver as a default */
		{
			vBoxType = LircdName;
		}
	}
	printf("[evremote2] vBoxType: %d (%s)\n", vBoxType, vName);
	return vBoxType;
}

void ignoreSIGPIPE()
{
	struct sigaction vAction;

	vAction.sa_handler = SIG_IGN;

	sigemptyset(&vAction.sa_mask);
	vAction.sa_flags = 0;
	sigaction(SIGPIPE, &vAction, (struct sigaction *)NULL);
}

int main(int argc, char *argv[])
{
	eBoxType vBoxType = Unknown;
	Context_t context;
	Context_r_t context_r;
	const int cMaxButtonExtension = 128;
	tButton vButtonExtension[cMaxButtonExtension];  // Up To 128 Extension Buttons
	int vButtonExtensionCounter;

	/* Dagobert: if tuxtxt closes the socket while
	 * we are writing a sigpipe occures which kills
	 * evremote. so lets ignore it ...
	 */
	ignoreSIGPIPE();

	if (argc >= 2
	&& (!strncmp(argv[1], "-h", 2) || !strncmp(argv[1], "--help", 6)))
	{
		printf("USAGE:\n");
		printf("evremote2 [[[useLircdName] <period>] <delay>] <IconNumber>]\n");
		printf("Parameters description:\n");
		printf("useLircdName - using key names defined in lircd.conf.\n");
		printf("               Can work with multiple RCs simultaneously.\n");
		printf("<period> - time of pressing a key.\n");
		printf("<delay> - delay between pressing keys. Increase if RC is too sensitive\n");
		printf("<IconNumber> - Number of blinking Icon\n");
		printf("No parameters - autoselection of RC driver with standard features.\n\n");
		return 0;
	}
	if (argc >= 2 && !strncmp(argv[1], "useLircdName", 12))
	{
		vBoxType = LircdName;
		if (argc >= 3)
		{
			cmdBtnPeriod = atoi(argv[2]);
		}
		if (argc >= 4)
		{
			cmdBtnDelay = atoi(argv[3]);
		}
	}
	else
	{
		vBoxType = getModel();
	}
	if (vBoxType != Unknown)
	{
		if (!getEventDevice())
		{
			printf("[evremote2] Unable to open event device\n");
			return 5;
		}
	}
	selectRemote(&context_r, &context, vBoxType);

	printf("[evremote2] Remote selected: %s\n", context.r->Name);

	if (context.r->RemoteControl != NULL)
	{
		printf("[evremote2] RemoteControl Map:\n");
		printKeyMap(context.r->RemoteControl);
	}
	if (context.r->Frontpanel != NULL)
	{
		printf("[evremote2] Frontpanel Map:\n");
		printKeyMap(context.r->Frontpanel);
	}
	vButtonExtensionCounter = 0;

	if (argc == 3 && !strncmp(argv[1], "-r", 2))
	{
		char vKeyName[64];
		char vKeyWord[64];
		int vKeyCode;
		char *vRemoteFile  = argv[2];
		FILE *vRemoteFileD = NULL;

		vRemoteFileD = fopen(vRemoteFile, "r");

		if (vRemoteFileD != NULL)
		{
			while (fscanf(vRemoteFileD, "%s %s %d", vKeyName, vKeyWord, &vKeyCode) == 3)
			{
				strncpy(vButtonExtension[vButtonExtensionCounter].KeyName, vKeyName, 20);
				strncpy(vButtonExtension[vButtonExtensionCounter].KeyWord, vKeyWord, 2);
				vButtonExtension[vButtonExtensionCounter].KeyCode = vKeyCode;
				vButtonExtensionCounter++;

				if (vButtonExtensionCounter + 1 == cMaxButtonExtension)
				{
					break;
				}
			}
			fclose(vRemoteFileD);
			strncpy(vButtonExtension[vButtonExtensionCounter].KeyName, "\0", 1);
			strncpy(vButtonExtension[vButtonExtensionCounter].KeyWord, "\0", 1);
			vButtonExtension[vButtonExtensionCounter].KeyCode = KEY_NULL;
			printf("[evremote2] RemoteControl Extension Map:\n");
			printKeyMap(vButtonExtension);
		}
	}
	if (vButtonExtensionCounter > 0)
	{
		context.r->RemoteControl = vButtonExtension;
	}
	// TODO
	//if (context.r->RemoteControl == NULL && vButtonExtensionCounter > 0)
	//{
	//	context.r->RemoteControl = vButtonExtension;
	//}
	//else if (vButtonExtensionCounter > 0)
	//{
	//	int vRemoteControlSize = sizeof(context.r->RemoteControl) / sizeof(tButton);
	//	int vRemoteControlExtSize = vButtonExtensionCounter;
	//
	//	context.r->RemoteControl = malloc((vRemoteControlSize + vRemoteControlExtSize - 1) * sizeof(tButton));
	//}
	//printf("[evremote2] RemoteControl Map:\n");
	//printKeyMap(context.r->RemoteControl);

	printf("[evremote2] Supports Long KeyPress: %s\n", context.r->supportsLongKeyPress == 0 ? "no" : "yes");

	if (context.r->supportsLongKeyPress)
	{
		processComplex(&context_r, &context, argc, argv);
	}
	else
	{
		processSimple(&context_r, &context, argc, argv);
	}
	return 0;
}
// vim:ts=4

