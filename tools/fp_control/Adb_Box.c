/*
 * Adb_Box.c
 *
 * (c) 2010 duckbox project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* ******************* includes ************************ */
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

#include "global.h"
#include "Adb_Box.h"

static int setText(Context_t *context, char *theText);

/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"

#define cMAXCharsADB_BOX 16

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tADB_BOXPrivate;

/* ******************* helper/misc functions ****************** */

/* Calculate the time value which we can pass to
 * the adb_box fp. its a mjd time (mjd=modified
 * julian date). mjd is relativ to gmt so theGMTTime
 * must be in GMT/UTC.
 */
void setAdb_BoxTime(time_t theGMTTime, char *destString)
{
	/* from u-boot adb_box */
	struct	tm *now_tm;
	now_tm = gmtime(&theGMTTime);
	printf("Set Time (UTC): %02d:%02d:%02d %02d-%02d-%04d\n",
		   now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900);
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;
	destString[0] = (mjd_int >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

unsigned long getAdb_BoxTime(char *adb_boxTimeString)
{
	unsigned int    mjd     = ((adb_boxTimeString[1] & 0xFF) * 256) + (adb_boxTimeString[2] & 0xFF);
	unsigned long   epoch   = ((mjd - 40587) * 86400);
	unsigned int    hour    = adb_boxTimeString[3] & 0xFF;
	unsigned int    min     = adb_boxTimeString[4] & 0xFF;
	unsigned int    sec     = adb_boxTimeString[5] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd,
		   epoch, hour, min, sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tADB_BOXPrivate *private = malloc(sizeof(tADB_BOXPrivate));
	int vFd;
	printf("%s\n", __func__);
	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tADB_BOXPrivate));
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int usage(Context_t *context, char *prg_name, char *cmd_name)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{
	struct adb_box_ioctl_data vData;
	printf("%s\n", __func__);
	setAdb_BoxTime(*theGMTTime, vData.u.time.time);
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("setTime: ");
		return -1;
	}
	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	char fp_time[8];
	fprintf(stderr, "Waiting for current time from fp...\n");
	/* front controller time */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("getTime: ");
		return -1;
	}
	/* if we get the fp time */
	if (fp_time[0] != '\0')
	{
		fprintf(stderr, "Success reading time from fp\n");
		/* current front controller time */
		*theGMTTime = (time_t) getAdb_BoxTime(fp_time);
	}
	else
	{
		fprintf(stderr, "Error reading time from fp\n");
		*theGMTTime = 0;
	}
	return 0;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{
	struct adb_box_ioctl_data vData;
	time_t curTime;
	time_t wakeupTime;
	struct tm *ts;
	time(&curTime);
	ts = localtime(&curTime);
	fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
			ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	if (theGMTTime == NULL)
	{
		wakeupTime = read_timers_utc(curTime);
	}
	else
	{
		wakeupTime = *theGMTTime;
	}
	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
	{
		/* nothing to do for e2 */
		fprintf(stderr, "no e2 timer found clearing fp wakeup time, Goodbye...\n");
		vData.u.standby.time[0] = '\0';
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standby: ");
			return -1;
		}
	}
	else
	{
		unsigned long diff;
		char fp_time[8];
		fprintf(stderr, "Waiting for current time from fp ...\n");
		/* front controller time */
		if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
		{
			perror("gettime: ");
			return -1;
		}
		/* difference from now to wake up */
		diff = (unsigned long int) wakeupTime - curTime;
		/* if we get the fp time */
		if (fp_time[0] != '\0')
		{
			fprintf(stderr, "Success reading time from fp\n");
			/* current front controller time */
			curTime = (time_t) getAdb_BoxTime(fp_time);
		}
		else
		{
			fprintf(stderr, "Error reading time, assuming localtime\n");
			/* noop current time already set */
		}
		wakeupTime = curTime + diff;
		setAdb_BoxTime(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standby: ");
			return -1;
		}
	}
	return 0;
}

static int getWTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	time_t curTime;
	/* shutdown immediately */
	if (*shutdownTimeGMT == -1)
	{
		return (setTimer(context, NULL));
	}
	while (1)
	{
		time(&curTime);
		/*printf("curTime = %d, shutdown %d\n", curTime, *shutdownTimeGMT);*/
		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent e2 timer and bye bye */
			return (setTimer(context, NULL));
		}
		usleep(100000);
	}
	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;
	struct adb_box_ioctl_data vData;
	while (1)
	{
		time(&curTime);
		if (curTime >= *rebootTimeGMT)
		{
			if (ioctl(context->fd, VFDREBOOT, &vData) < 0)
			{
				perror("reboot: ");
				return -1;
			}
		}
		usleep(100000);
	}
	return 0;
}

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	time_t curTime;
	int sleep = 1;
	int vFd = 0;
	fd_set rfds;
	struct timeval tv;
	int retval;
	struct tm *ts;
	char output[cMAXCharsADB_BOX + 1];
	tADB_BOXPrivate *private = (tADB_BOXPrivate *)((Model_t *)context->m)->private;
#if 0
	printf("%s\n", __func__);
	vFd = open(cRC_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cRC_DEVICE);
		perror("");
		return -1;
	}
#endif
	printf("%s 1\n", __func__);
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
				sleep = 0;
			}
		}
		if (private->display)
		{
			strftime(output, cMAXCharsADB_BOX + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{
	char vHelp[cMAXCharsADB_BOX + 1];
	strncpy(vHelp, theText, cMAXCharsADB_BOX);
	vHelp[cMAXCharsADB_BOX] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int setLed(Context_t *context, int which, int on)
{
	struct adb_box_ioctl_data vData;
	vData.u.led.led_nr = which;
	vData.u.led.on = on;
	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("setLed: ");
		return -1;
	}
	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{
	struct adb_box_ioctl_data vData;
	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("setIcon: ");
		return -1;
	}
	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	struct adb_box_ioctl_data vData;
	if (brightness < 0 || brightness > 7)
	{
		return -1;
	}
	vData.u.brightness.level = brightness;
	printf("%d\n", context->fd);
	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("setBrightness: ");
		return -1;
	}
	return 0;
}

static int setLight(Context_t *context, int on)
{
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

static int Exit(Context_t *context)
{
	tADB_BOXPrivate *private = (tADB_BOXPrivate *)((Model_t *)context->m)->private;
	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

static int Clear(Context_t *context)
{
	struct adb_box_ioctl_data vData;
	if (ioctl(context->fd, VFDDISPLAYCLR, &vData) < 0)
	{
		perror("Clear: ");
		return -1;
	}
	return 0;
}

Model_t Adb_Box_model =
{
	.Name             = "B4Team ADB_BOX frontpanel control utility",
	.Type             = Adb_Box,
	.Init             = init,
	.Clear            = Clear,
	.Usage            = usage,
	.SetTime          = setTime,
	.GetTime          = getTime,
	.SetTimer         = setTimer,
	.GetWTime         = getWTime,
	.SetWTime         = NULL,
	.Shutdown         = shutdown,
	.Reboot           = reboot,
	.Sleep            = Sleep,
	.SetText          = setText,
	.SetLed           = setLed,
	.SetIcon          = setIcon,
	.SetBrightness    = setBrightness,
	.GetWakeupReason  = NULL,
	.SetLight         = setLight,
	.SetLedBrightness = NULL,
	.GetVersion       = NULL,
	.SetRF            = NULL,
	.SetFan           = NULL,
	.SetDisplayTime   = NULL,
	.SetTimeMode      = NULL,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = NULL,
#endif
	.Exit             = Exit
};
