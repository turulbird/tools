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

/******************** includes ************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "global.h"
#include "Ufs922.h"

extern int getUTCoffset(void);
extern int setUTCoffset(int UTC_offset);

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);
static int setIcon(Context_t *context, int which, int on);

/* ****************** constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cEVENT_DEVICE "/dev/input/event0"

#define cMAXCharsUFS922 16

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tUFS922Private;

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} tArgs;

tArgs vK922Args[] =
{
	{ "-e", "  --setTimer         * ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Set the most recent timer from E2 or Neutrino" },
	{ "", "                         ", "      to the frontcontroller and shutdown" },
	{ "", "                         ", "      Arg time date: Set frontcontroller wake up time to" },
	{ "", "                         ", "      time, shutdown, and wake up at given time" },
	{ "-d", "  --shutdown         * ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Shut down immediately" },
	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
	{ "-r", "  --reboot           * ", "Args: None" },
	{ "", "                         ", "      No arg: Reboot immediately" },
	{ "", "                         ", "      Arg time date: Reboot at given time/date" },
	{ "-g", "  --getTime          * ", "Args: None        Display currently set frontprocessor time" },
//	{ "-gs", " --getTimeAndSet      ", "Args: None" },
//	{ "", "                         ", "      Set system time to current frontprocessor time" },
//	{ "", "                         ", "      WARNING: system date will be 01-01-1970!" },
	{ "-gw", " --getWTime         * ", "Args: None        Get the current frontcontroller wake up time" },
	{ "-s", "  --setTime          * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor time" },
	{ "-sst", "--setSystemTime    * ", "Args: None        Set front processor time to system time" },
	{ "-st", " --setWakeTime      * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontcontroller wake up time" },
	{ "-p", "  --sleep            * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Reboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Args: text        Set text to frontpanel" },
	{ "-l", "  --setLed             ", "Args: LED# 1|0    Set an LED on or off" },
	{ "-led", "--setLedBrightness   ", "Args: int         int=brightness (0..7)" },
	{ "-i", "  --setIcon            ", "Args: icon# 1|0   Set an icon on or off" },
	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
	{ "-sf", " --setFan             ", "Arg : 0..255      Set fan speed" },
#if defined MODEL_SPECIFIC
//	{ "-ms", " --model_specific     ", "Args: int1 [int2] [int3] ... [int16]   (note: input in hex)" },
//	{ "", "                         ", "                  Model specific test function" },
#endif
	{ NULL, NULL, NULL }
};

/* ******************* helper/misc functions ****************** */

/*****************************************************
 *
 * Set the front processor driver to comaptible mode
 * for the next IOCTL command.
 *
 */
static void setMode(int fd)
{
	struct micom_ioctl_data micom;

	micom.u.mode.compat = 1;
	if (ioctl(fd, VFDSETMODE, &micom) < 0)
	{
		perror("setMode: ");
	}
}

/*****************************************************
 *
 * Calculate the time value which can be passed to
 * the micom FP. It is an MJD time (MJD = Modified
 * Julian Date). MJD is relative to GMT so theGMTTime
 * must be in GMT/UTC.
 *
 */
static void calcSetMicomTime(time_t theGMTTime, char *destString)
{
	struct tm *now_tm;
	int mjd;

	now_tm = gmtime(&theGMTTime);
	mjd = (int)modJulianDate(now_tm);

	if (disp)
	{
		printf("Calculated Time: %02d:%02d:%02d %02d-%02d-%04d (MJD=%d)\n",
			now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_tm->tm_mday,
			now_tm->tm_mon + 1, now_tm->tm_year + 1900, mjd);
	}
	destString[0] = (mjd >> 8);
	destString[1] = (mjd & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

/********************************************
 *
 * Convert MJDhms to time_t
 *
 */
static unsigned long calcGetMicomTime(char *destString)
{
	unsigned int  mjd   = ((destString[0] & 0xff) << 8) + (destString[1] & 0xff);
	unsigned long epoch = ((mjd - 40587) * 86400);  // 40587 is difference in days between MJD and Linux epochs

	unsigned int  hour  = destString[2] & 0xff;
	unsigned int  min   = destString[3] & 0xff;
	unsigned int  sec   = destString[4] & 0xff;

	epoch += (hour * 3600 + min * 60 + sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tUFS922Private *private = malloc(sizeof(tUFS922Private));
	int vFd;

	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tUFS922Private));

	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int usage(Context_t *context, char *prg_name, char *cmd_name)
{
	int i;

	printf("Usage:\n\n");
	printf("%s argument [optarg1] [optarg2]\n", prg_name);
	for (i = 0; ; i++)
	{
		if (vK922Args[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vK922Args[i].arg) == 0) || (strstr(vK922Args[i].arg_long, cmd_name) != NULL))
		{
			printf("%s   %s   %s\n", vK922Args[i].arg, vK922Args[i].arg_long, vK922Args[i].arg_description);
		}
	}
	printf("Options marked * should be the only calling argument.\n");
	printf("All times and dates are in local time.\n");
	return 0;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{  // -s command: OK
	struct micom_ioctl_data vData;
	int gmt_offset;

	gmt_offset = getUTCoffset();
	calcSetMicomTime(*theGMTTime - gmt_offset, vData.u.time.time);
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("setTime");
		return -1;
	}
	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{ // -g command: OK
	char fp_time[8];
	time_t curTimeFP;
	int gmt_offset;
	struct tm *ts_gmt;

	/* Get FP time */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("getTime");
		return -1;
	}

	if (fp_time[0] != '\0')
	{
		curTimeFP = (time_t)calcGetMicomTime(fp_time);  // UTC
		gmt_offset = getUTCoffset();
		if (disp)
		{
			ts_gmt = gmtime(&curTimeFP);		
			printf("Current FP time: %02d:%02d:%02d %02d-%02d-%04d (UTC), UTC offset: %+d seconds\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
				ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900, gmt_offset);
		}
		*theGMTTime = curTimeFP + gmt_offset;  // return local time
		// write UTC offset to /proc/stb/fp/rtc_offset
		if (setUTCoffset(gmt_offset))
		{
			return -1;
		}
	}
	else
	{
		fprintf(stderr, "Error reading time from FP\n");
		*theGMTTime = 0;
	}
	return 0;
}

static int setSTime(Context_t *context, time_t *theGMTTime)
{  // -sst command: OK
	time_t curTime;
	char fp_time[8];
	time_t curTimeFP;
	struct tm *ts_gmt;
	int gmt_offset;

	time(&curTime); // get system time in UTC
	gmt_offset = getUTCoffset();
	if (disp)
	{
		ts_gmt = gmtime(&curTime);
		printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
			ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);
	}
	curTime += gmt_offset;
	setTime(context, &curTime); // set FP clock to system time in UTC

	/* Read FP time back */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Gettime");
		return -1;
	}
	curTimeFP = (time_t)calcGetMicomTime(fp_time);
	ts_gmt = gmtime(&curTimeFP);
	printf("Front panel time set to: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
		ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);

	// write UTC offset to /proc/stb/fp/rtc_offset
	if (setUTCoffset(gmt_offset))
	{
		return -1;
	}
	return 0;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{  // -e command: OK
	struct micom_ioctl_data vData;
	
	time_t curTime    = 0;
	time_t curTimeFP  = 0;
	time_t wakeupTime = 0;
	struct tm *ts;
	struct tm *tsFP;
	struct tm *tsWakeupTime;
	int gmt_offset;

	// Get current system time
	time(&curTime);
	gmt_offset = getUTCoffset();

	// Get current Frontpanel time
	getTime(context, &curTimeFP);  // get FP time (local!)
	curTimeFP -= gmt_offset;  // convert to UTC
	if (disp)
	{
		tsFP = gmtime(&curTimeFP);
		printf("Current FP time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
			tsFP->tm_hour, tsFP->tm_min, tsFP->tm_sec,
			tsFP->tm_mday, tsFP->tm_mon + 1, tsFP->tm_year + 1900);
	}
	/* set FP time if system time is more than 5 minutes off */
	if (((curTime - curTimeFP) > 300) || ((curTimeFP - curTime) > 300))
	{
		printf("Time difference between FP and system is %+d seconds\n", (int)(curTimeFP - curTime));
		setTime(context, &curTime); // sync fp clock
		curTimeFP = curTime;
		ts = gmtime(&curTimeFP);
		printf("Front panel time corrected, set to: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
			ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	}

	if (theGMTTime == NULL)  // if no argument supplied
	{
		wakeupTime = read_timers_utc(curTime);  // get 1st timer
	}
	else
	{
		wakeupTime = *theGMTTime - gmt_offset;  // argument supplied is assumed to be local time
	}

	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX) || (curTime > wakeupTime) || (curTime < (wakeupTime - 25920000)))
	{
		/* clear timer */
		printf("No timers set or 1st timer more than 300 days ahead,\nor all timer(s) in the past.\n");
		vData.u.standby.time[0] = '\0';
		wakeupTime = LONG_MAX;
	}
	else // wake up time valid and in the coming 300 days
	{
		// Show wakeup time
		tsWakeupTime = gmtime(&wakeupTime);
		printf("Wake up time: %02d:%02d:%02d %02d-%02d-%04d (UTC, offset: %+d seconds)\n",
			tsWakeupTime->tm_hour, tsWakeupTime->tm_min, tsWakeupTime->tm_sec,
			tsWakeupTime->tm_mday, tsWakeupTime->tm_mon + 1, tsWakeupTime->tm_year + 1900, gmt_offset);

		calcSetMicomTime(wakeupTime, vData.u.standby.time);
		if (disp)
		{
			printf("Setting FP wake up Time to %d (MJD) - %02d:%02d:%02d (UTC)\n",
				(vData.u.standby.time[0] << 8) + vData.u.standby.time[1], vData.u.standby.time[2],
				vData.u.standby.time[3], vData.u.standby.time[4] );
		}
	}
	if (disp)
	{
		printf("Entering Deep Standby.\n");
	}
	fflush(stdout);
	fflush(stderr);
	sleep(2);
	if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
	{
		perror("Shut down");
		return -1;
	}
	return 0;
}

static int getWTime(Context_t *context, time_t *theGMTTime)
{ // -gw command
	char fp_wtime[8];

	/* front controller wake up time */
	if (ioctl(context->fd, VFDGETWAKEUPTIME, &fp_wtime) < 0)
	{
		perror("getWtime");
		return -1;
	}
	/* if we get the fp wake up time time */
	if (fp_wtime[0] != '\0')
	{
		/* current front controller time */
		*theGMTTime = (time_t)calcGetMicomTime(fp_wtime);
	}
	else
	{
		fprintf(stderr, "Error reading time from FP\n");
		*theGMTTime = 0;
	}
	return 0;
}

static int setWTime(Context_t *context, time_t *theGMTTime)
{  // -st command, to be tested
	struct micom_ioctl_data vData;
	
	time_t curTime    = 0;
	time_t curTimeFP  = 0;
	time_t wakeupTime = 0;
	struct tm *ts;
	struct tm *tsFP;
	struct tm *tsWakeupTime;
	int gmt_offset;

	// Get current system time
	time(&curTime);
	if (disp)
	{
		ts = gmtime(&curTime);
		printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
			ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	}
	gmt_offset = getUTCoffset();

	// Get current Frontpanel time
	getTime(context, &curTimeFP);  // local!
	curTimeFP -= gmt_offset;
	if (disp)
	{
		tsFP = gmtime(&curTimeFP);
		printf("Current FP time: %02d:%02d:%02d %02d-%02d-%04d (UTC, offset: %+d seconds)\n",
			tsFP->tm_hour, tsFP->tm_min, tsFP->tm_sec, tsFP->tm_mday, tsFP->tm_mon + 1, tsFP->tm_year + 1900, gmt_offset);
	}
	/* set FP time to system time if more than 5 minutes off */
	if (((curTime - curTimeFP) > 300) || ((curTimeFP - curTime) > 300))
	{
		printf("Time difference between FP and system is %+d seconds\n", (int)(curTimeFP - curTime));
		setTime(context, &curTime); // sync fp clock
		curTimeFP = curTime;
		ts = gmtime(&curTime);
		printf("Front panel time corrected, set to: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
			ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	}

	if (theGMTTime == NULL)
	{
		wakeupTime = read_timers_utc(curTime);  // timers are stored in UTC
	}
	else
	{
		wakeupTime = *theGMTTime - gmt_offset;  // get specified time (assumed local)
	}

	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX) || (curTime > wakeupTime) || (curTime < (wakeupTime - 25920000)))
	{
		printf("No timers set or 1st timer more than 300 days ahead,\nor all timer(s) in the past.\n");
		/* clear timer */
		vData.u.standby.time[0] = '\0';
		wakeupTime = LONG_MAX;
	}
	else
	{
		if (disp)
		{
			// Show wakeup time
			tsWakeupTime = gmtime(&wakeupTime);
			printf("Wake up time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
				tsWakeupTime->tm_hour, tsWakeupTime->tm_min, tsWakeupTime->tm_sec,
				tsWakeupTime->tm_mday, tsWakeupTime->tm_mon + 1, tsWakeupTime->tm_year + 1900);
		}
		calcSetMicomTime(wakeupTime, vData.u.standby.time);
		printf("FP Wakeup Time set to %d (MJD) %02d:%02d:%02d (UTC)\n",
			(((vData.u.standby.time[0] & 0xff) << 8) + (vData.u.standby.time[1] & 0xff)), vData.u.standby.time[2],
			vData.u.standby.time[3], vData.u.standby.time[4]);
		if (ioctl(context->fd, VFDSETWAKEUPTIME, &vData) < 0)
		{
			perror("setWtime");
			return -1;
		}
	}
	return 0;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{  // -d command: OK
	time_t curTime;
	int gmt_offset;

	if (*shutdownTimeGMT == -1)  // if no argument supplied
	{
		return (setTimer(context, NULL));  // shutdown immediately
	}

	gmt_offset = getUTCoffset();
	*shutdownTimeGMT -= gmt_offset;  // supplied time is assumed to be local
	time(&curTime);
	if (disp)
	{
		printf("Waiting until moment of shut down (in %d seconds)...\n", (int)(*shutdownTimeGMT - curTime));
	}
	while (1)
	{
		time(&curTime);
		if ((*shutdownTimeGMT - curTime < 10) && disp)
		{
			printf("Shutdown in %d seconds\n", (int)(*shutdownTimeGMT - curTime));
		}
		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent E2/Neutrino timer and shut down */
			if (disp)
			{
				printf("Shutting down\n");
			}
			return (setTimer(context, NULL));
		}
		usleep(1000000);  // wait a second
	}
	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{  // -r command: OK
	time_t curTime;
	struct micom_ioctl_data vData;
	int gmt_offset;

	gmt_offset = getUTCoffset();
	*rebootTimeGMT -= gmt_offset;  // supplied time is assumed to be local
	time(&curTime);
	if (disp)
	{
		printf("Waiting until moment of reboot (in %d seconds)...\n", (int)(*rebootTimeGMT - curTime));
	}
	while (1)
	{
		time(&curTime);
		if ((*rebootTimeGMT - curTime < 10) && disp)
		{
			printf("Reboot in %d seconds\n", (int)(*rebootTimeGMT - curTime));
		}
		if (curTime >= *rebootTimeGMT)
		{
			if (disp)
			{
				printf("Rebooting\n");
			}
			if (ioctl(context->fd, VFDREBOOT, &vData) < 0)
			{
				perror("reboot");
				return -1;
			}
		}
		usleep(1000000);
	}
	return 0;
}

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{  // -p command: to be tested
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, i, rd;
	struct tm *ts;
	char output[cMAXCharsUFS922 + 1];
	struct input_event ev[64];
	tUFS922Private *private = (tUFS922Private *)((Model_t *)context->m)->private;
	int gmt_offset;

	vFd = open(cEVENT_DEVICE, O_RDWR);

	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cEVENT_DEVICE);
		perror("");
		return -1;
	}
	gmt_offset = getUTCoffset();
	*wakeUpGMT -= gmt_offset;
	
//	printf("%s 1\n", __func__);
	while (sleep)
	{
		time(&curTime);
		ts = localtime(&curTime);
		if (curTime >= *wakeUpGMT)
		{
			sleep = 0;
		}
		else
		{
			FD_ZERO(&rfds);
			FD_SET(vFd, &rfds);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			retval = select(vFd + 1, &rfds, NULL, NULL, &tv);
			if (retval > 0)
			{
				rd = read(vFd, ev, sizeof(struct input_event) * 64);

				if (rd < (int) sizeof(struct input_event))
				{
					continue;
				}
				for (i = 0; i < rd / sizeof(struct input_event); i++)
				{
					if (ev[i].type == EV_SYN)
					{
					}
					else if (ev[i].type == EV_MSC && (ev[i].code == MSC_RAW || ev[i].code == MSC_SCAN))
					{
					}
					else
					{
						if (ev[i].code == 116)
						{
							sleep = 0;
						}
					}
				}
			}
		}
		if (private->display)
		{
			strftime(output, cMAXCharsUFS922 + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{  // -t command: OK
	char vHelp[128];

	strncpy(vHelp, theText, cMAXCharsUFS922);
	vHelp[cMAXCharsUFS922] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int setLed(Context_t *context, int which, int on)
{  // -l command: OK
	struct micom_ioctl_data vData;

	vData.u.led.led_nr = which;
	vData.u.led.on = on;
	setMode(context->fd);
	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("setLed");
		return -1;
	}
	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{ // -i command : OK
	struct micom_ioctl_data vData;

	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	setMode(context->fd);
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("setIcon");
		return -1;
	}
	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{  // -b command : OK
	struct micom_ioctl_data vData;

	if (brightness < 0 || brightness > 7)
	{
		return -1;
	}
	vData.u.brightness.level = brightness;
	setMode(context->fd);
	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("setBrightness");
		return -1;
	}
	return 0;
}

static int setLight(Context_t *context, int on)
{  // -L command: OK
	if (on)
	{
		setBrightness(context, 7);
	}
	else
	{
		setBrightness(context, 0);
	}
	return 0;
}

static int getWakeupReason(Context_t *context, eWakeupReason *reason)
{  // -w command: OK
	int mode;

	/* front controller wake up mode */
	if (ioctl(context->fd, VFDGETWAKEUPMODE, &mode) < 0)
	{
		perror("Get wake up reason");
		return -1;
	}
	if (mode != '\0')  // if we get a fp wake up reason
	{
		*reason = mode & 0xff;
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  // echo unknown
	}
	return 0;
}

static int getVersion(Context_t *context, int *version)
{  // -v command: OK
	char strVersion[8];

	/* front controller version info */
	if (ioctl(context->fd, VFDGETVERSION, &strVersion) < 0)
	{
		perror("getVersion");
		return -1;
	}
	/* if we get the fp version */
	if (strVersion[0] != '\0')
	{
		*version = (strVersion[1] * 100) + strVersion[2];
		if (disp)
		{
			printf("FP version = %d.%02d\n", strVersion[1], strVersion[2]);
		}
	}
	else
	{
		fprintf(stderr, "Error reading version from frontprocessor\n");
		*version = 0;
	}
	return 0;
}

static int Clear(Context_t *context)
{  // -c command: OK
	int i;
	char string[cMAXCharsUFS922 + 1];
	struct micom_ioctl_data vData;

	memset(string, 0x20, sizeof(string));
	string[cMAXCharsUFS922] = '\0';
	write(context->fd, string, strlen(string));

	for (i = 1; i <= 6 ; i++)
	{
		setLed(context, i, 0);
	}
	vData.u.icon.icon_nr = 17;
	vData.u.icon.on = 0;
	setMode(context->fd);
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Clear");
		return -1;
	}
	return 0;
}

static int setLedBrightness(Context_t *context, int brightness)
{  // -led command: OK
	struct micom_ioctl_data vData;

	if (brightness < 0 || brightness > 7)
	{
		return -1;
	}
	vData.u.brightness.level = brightness;
	setMode(context->fd);
	if (ioctl(context->fd, VFDLEDBRIGHTNESS, &vData) < 0)
	{
		perror("setLedBrightness");
		return -1;
	}
	return 0;
}

static int setFan(Context_t *context, int speed)
{  // -sf command: OK
	struct micom_ioctl_data vData;

	if (speed < 0 || speed > 255)
	{
		return -1;
	}
	vData.u.fan.speed = speed;
//	setMode(context->fd);
	if (ioctl(context->fd, VFDSETFAN, &vData) < 0)
	{
		perror("setfan");
		return -1;
	}
	return 0;
}

static int Exit(Context_t *context)
{  // OK
	tUFS922Private *private = (tUFS922Private *)((Model_t *)context->m)->private;

	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

Model_t UFS922_model =
{
	.Name             = "Kathrein UFS922 frontpanel control utility",
	.Type             = Ufs922,
	.Init             = init,
	.Clear            = Clear,
	.Usage            = usage,
	.SetTime          = setTime,
	.GetTime          = getTime,
	.SetTimer         = setTimer,
	.GetWTime         = getWTime,
	.SetWTime         = setWTime,
	.SetSTime         = setSTime,
	.Shutdown         = shutdown,
	.Reboot           = reboot,
	.Sleep            = Sleep,
	.SetText          = setText,
	.SetLed           = setLed,
	.SetIcon          = setIcon,
	.SetBrightness    = setBrightness,
	.GetWakeupReason  = getWakeupReason,
	.SetLight         = setLight,
	.SetLedBrightness = setLedBrightness,
	.GetVersion       = getVersion,
	.SetRF            = NULL,
	.SetFan           = setFan,
	.SetDisplayTime   = NULL,
	.SetTimeMode      = NULL,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = NULL,
#endif
	.Exit             = Exit
};
// vim:ts=4
