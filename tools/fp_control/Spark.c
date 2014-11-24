/*
 * Spark.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 ****************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------
 * 20130909 Audioniek       Get wake up reason made functional
 * 20130910 Audioniek       Set timer polished
 * 20130911 Audioniek       Shutdown polished
 * 20130911 Audioniek       Reboot built
 *
 ****************************************************************************/

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
#include "Spark.h"

static int Spark_setText(Context_t *context, char *theText);
int res;
/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
/* FIXME: Valid for Spark7162 only */
#define cMAXCharsSpark 8

typedef struct
{
	char    *arg;
	char    *arg_long;
	char    *arg_description;
} tArgs;

tArgs vHArgs[] =
{
	{ "-e", "  --setTimer           ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Set the most recent timer from e2 or neutrino" },
	{ "", "                         ", "      to the frontcontroller and shutdown" },
	{ "", "                         ", "      Arg time date: Set frontcontroller wake-up time to" },
	{ "", "                         ", "      time, shutdown, and wake up at given time" },
	{ "-d", "  --shutdown           ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Shut down immediately" },
	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
	{ "-r", "  --reboot             ", "Args: None" },
	{ "", "                         ", "      No arg:     Reboot immediately (= -e current time+3 date today" },
	{ "", "                         ", "      Arg time date: Reboot at given time/date (= -e time date)" },
	{ "-g", "  --getTime            ", "Args: None        Display currently set frontprocessor time" },
	{ "-gs", " --getTimeAndSet      ", "Args: None        Set system time to current frontprocessor time" },
	{ "", "                         ", "      WARNING: system date will be 01-01-1970!" },
	{ "-gt", " --getWTime           ", "Args: None        Get the current frontcontroller wake up time" },
	{ "-s", "  --setTime            ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor time" },
	{ "-st", " --setWTime           ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor wake up time" },
	{ "-sst", "--setSystemTime      ", "Args: None        Set frontprocessor time to current system time" },
//	{ "-p", "  --sleep              ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY\n\t\tReboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Arg : text        Show text in front panel display" },
	{ "-l", "  --setLed             ", "Args: LED# 0|1|2  LED#: off, on or blink" },
	{ "-i", "  --setIcon            ", "Args: icon# 0|1   Set an icon off or on" },
	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
//	{ "-tm"," --time_mode           ", "Arg : 0|1         Set 12/24 hour mode" },
//	{ "-gms", "--get_model_specific ", "Arg : int         Model specific get function" },
	{ "-ms ", "--set_model_specific ", "Args: long long   Model specific set function" },
	{ NULL, NULL, NULL }
};

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tSparkPrivate;

/* ******************* helper/misc functions ****************** */

/* Calculate the time value which we can pass to
    * the aotom fp. its a mjd time (mjd=modified
    * julian date). mjd is relative to gmt so theGMTTime
    * must be in GMT/UTC.
    */
void Spark_calcAotomTime(time_t theGMTTime, char *destString)
{
	/* from u-boot aotom */
	struct tm *now_tm;
	now_tm = localtime(&theGMTTime);
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;
	destString[0] = (mjd_int >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

unsigned long Spark_getAotomTime(char *aotomTimeString)
{
	unsigned int    mjd     = ((aotomTimeString[1] & 0xFF) * 256) + (aotomTimeString[2] & 0xFF);
	unsigned long   epoch   = ((mjd - 40587) * 86400); //01-01-1970
	unsigned int    hour    = aotomTimeString[3] & 0xFF;
	unsigned int    min     = aotomTimeString[4] & 0xFF;
	unsigned int    sec     = aotomTimeString[5] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd, epoch, hour, min, sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int Spark_init(Context_t *context)
{
	tSparkPrivate *private = malloc(sizeof(tSparkPrivate));
	int vFd;
//	printf("%s\n", __func__);
	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tSparkPrivate));
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement, disp);
	return vFd;
}

static int Spark_usage(Context_t *context, char *prg_name, char *cmd_name)
{
	int i;
	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "%s argument [optarg1] [optarg2]\n", prg_name);
	for (i = 0; ; i++)
	{
		if (vHArgs[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vHArgs[i].arg) == 0) || (strstr(vHArgs[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vHArgs[i].arg, vHArgs[i].arg_long, vHArgs[i].arg_description);
		}
	}
	return 0;
}

static int Spark_setTimer(Context_t *context, time_t *timerTime)
{
	//-e command, rewritten, tested on Spark7162
	struct aotom_ioctl_data vData;
	time_t curTime;
	time_t iTime;
	time_t wakeupTime;
	struct tm *ts;
	struct tm *tss;
	unsigned long   diff;
	int sday, smonth, syear;
	time(&curTime);  //get system time (UTC)
	tss = localtime(&curTime); //save it for the date
	sday = tss->tm_mday;
	smonth = tss->tm_mon + 1;
	syear = tss->tm_year + 1900;
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d\n", tss->tm_hour, tss->tm_min, tss->tm_sec, tss->tm_mday, tss->tm_mon + 1, tss->tm_year + 1900);
	if (timerTime == NULL) // -e no argument = shutdown on next e2/neutrino timer
	{
		wakeupTime = read_timers_utc(curTime); //get current 1st timer
	}
	else
	{
		wakeupTime = *timerTime; //get specified time
	}
	if ((wakeupTime == LONG_MAX) || (wakeupTime == -1)) // if no timers set
	{
		//if wake up time in the past
//		printf("There are no timers set!\n");
		return 0;
	}
	else
	{
		/* get front controller time */
		if (ioctl(context->fd, VFDGETTIME, &iTime) < 0)
		{
			perror("Get current fp time");
			return -1;
		}
		/* difference from now to wake up */
		diff = (unsigned long int) wakeupTime - curTime;
		if (iTime != '\0')
		{
			/* use current front controller time */
			curTime = iTime;
			ts = gmtime(&curTime);
			printf("Current fp");
		}
		else
		{
			printf("Error reading current fp time... assuming local system time\n");
			ts = localtime(&curTime);
			printf("Current system");
		}
		wakeupTime = curTime + diff;
		printf(" time: %02d:%02d:%02d %02d-%02d-%04d\n", ts->tm_hour, ts->tm_min, ts->tm_sec, sday, smonth, syear);
		if (curTime > wakeupTime)
		{
			printf("Wake up time in the past!\n");
			return 0;
		}
		ts = gmtime(&wakeupTime);
		printf("Wake up time: %02d:%02d:%02d", ts->tm_hour, ts->tm_min, ts->tm_sec);
//		if (diff > 86399)
		if ((tss->tm_mday == ts->tm_mday) && (tss->tm_mon == ts->tm_mon) && (tss->tm_year == ts->tm_year)) //today
		{
			printf(" (today)\n");
		}
		else
		{
			printf("\non          : %02d-%02d-%04d\n", sday, smonth, syear);
		}
//		if (tss->tm_mday > 1)
//		{
//			printf(" in %d day(s)", tss->tm_mday - 1);
//		}
//		if (tss->tm_mon != 0)
//		{
//			printf(", %d month(s)", tss->tm_mon);
//		}
//		if (tss->tm_year != 70)
//		{
//			printf(", %d year(s)", tss->tm_year - 70);
//		}
//		printf("\n");
		Spark_calcAotomTime(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &wakeupTime) < 0)
		{
			perror("Shut down until wake up time");
			return -1;
		}
	}
	return 0;
}

static int Spark_shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	//-d command, partially rewritten, tested on Spark7162
	struct aotom_ioctl_data vData;
	time_t curTime;
	time_t wakeupTime;
	struct tm *ts;
	/* shutdown immediately */
	if (*shutdownTimeGMT == -1)
	{
		if (ioctl(context->fd, VFDPOWEROFF, &vData) < 0)
		{
			perror("Shut down immediately");
			return -1;
		}
		return 0;
	}
	/* shutdown at given time */
	wakeupTime = *shutdownTimeGMT;
	ts = localtime(&wakeupTime);
	printf("Shut down time: %02d:%02d:%02d %02d-%02d-%04d\n",
		   ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	time(&curTime);
	ts = localtime(&curTime);
	if (curTime > *shutdownTimeGMT)
	{
		printf("Shut down time in the past!\n");
		return 0;
	}
	while (curTime < *shutdownTimeGMT)
	{
		time(&curTime);
		ts = localtime(&curTime);
		printf("Current time  : %02d:%02d:%02d %02d-%02d-%04d\r",
			   ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
		usleep(20000);
	}
	printf("\n\n");
	/* shut down */
	if (ioctl(context->fd, VFDPOWEROFF, &vData) < 0)
	{
		perror("Shutdown at given time");
		return -1;
	}
	return 0;
}

static int Spark_reboot(Context_t *context, time_t *rebootTimeGMT)
{
	//-r command, partially rewritten, tested on Spark7162
	// Note: aotom does not have a particular reboot command
	time_t curTime;
	/* reboot at a given time */
	if (*rebootTimeGMT != -1)
	{
		return (Spark_setTimer(context, rebootTimeGMT));
	}
	/* reboot immediately */
	time(&curTime);
	*rebootTimeGMT = curTime + 3;
	return (Spark_setTimer(context, rebootTimeGMT));
}

static int Spark_getTime(Context_t *context, time_t *theGMTTime)
{
	// -g command, adapted for spark, tested on spark7162
	struct tm *get_tm;
	time_t iTime;
	/* get front controller time */
	if (ioctl(context->fd, VFDGETTIME, &iTime) < 0)
	{
		perror("Get time");
		return -1;
	}
	/* if we get the fp time */
	if (iTime != '\0')
	{
//		fprintf(stderr, "Successfully read the time from fp\n");
		/* current frontcontroller time */
		*theGMTTime = iTime;
		get_tm = gmtime(&iTime);
		printf("Frontprocessor time: %02d:%02d:%02d %02d-%02d-%04d\n",
			   get_tm->tm_hour, get_tm->tm_min, get_tm->tm_sec, get_tm->tm_mday, get_tm->tm_mon + 1, get_tm->tm_year + 1900);
	}
	else
	{
		fprintf(stderr, "Error reading time from frontprocessor\n");
		*theGMTTime = 0;
		return -1;
	}
	return 1;
}

static int Spark_getWTime(Context_t *context, time_t *theGMTTime)
{
	//-gt command: VFDGETWAKEUPTIME not supported by older aotoms
	struct tm *get_tm;
	time_t iTime;
	/* front controller wake up time */
	if (ioctl(context->fd, VFDGETWAKEUPTIME, &iTime) < 0)
	{
		perror("Get wakeup time");
		return -1;
	}
	/* if we get the fp time */
	if (iTime != '\0')
	{
		/* current frontcontroller wake up time */
		*theGMTTime = iTime;
		get_tm = gmtime(&iTime);
		printf("Frontprocessor wakeup time: %02d:%02d:%02d %02d-%02d-%04d\n",
			get_tm->tm_hour, get_tm->tm_min, get_tm->tm_sec, get_tm->tm_mday, get_tm->tm_mon + 1, get_tm->tm_year + 1900);
	}
	else
	{
		fprintf(stderr, "Error reading wake up time from frontprocessor\n");
		*theGMTTime = 0;
		return -1;
	}
	return 1;
}

static int Spark_setTime(Context_t *context, time_t *theGMTTime)
{
	//-s command, tested on spark7162
	//FIXME: sets time to 00:59:59 if time string invalid, and date to 01-01-1970 if date string invalid
	struct aotom_ioctl_data vData;
	struct tm *set_tm;
	time_t iTime;
	Spark_calcAotomTime(*theGMTTime, vData.u.time.time);
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("Set time");
		return -1;
	}
	iTime = *theGMTTime;
	set_tm = localtime(&iTime);
	printf("Frontprocessor time set: %02d:%02d:%02d (date ignored)\n", set_tm->tm_hour, set_tm->tm_min, set_tm->tm_sec);
	return 1;
}

static int Spark_setWTime(Context_t *context, time_t *theGMTTime)
{
	//-st command, tested on spark7162
	struct tm *setwtm;
	time_t iTime;
	iTime = *theGMTTime;
	setwtm = localtime(&iTime);
	fprintf(stderr, "Setting wake up time to %02d:%02d:%02d %02d-%02d-%04d\n",
			setwtm->tm_hour, setwtm->tm_min, setwtm->tm_sec, setwtm->tm_mday, setwtm->tm_mon + 1, setwtm->tm_year + 1900);
	iTime += setwtm->tm_gmtoff;
	if (ioctl(context->fd, VFDSETPOWERONTIME, &iTime) < 0)
	{
		perror("Set wake up time");
		return -1;
	}
	return 0;
}

static int Spark_setSTime(Context_t *context, time_t *theGMTTime)
{
	//-sst command, tested on spark7162
	time_t systemTime = time(NULL);
	struct tm *gmt;
	gmt = localtime(&systemTime);
	if (gmt->tm_year == 100)
	{
		fprintf(stderr, "Problem: RTC time not set.\n");
	}
	else
	{
		fprintf(stderr, "Setting front panel clock to\ncurrent system time: %02d:%02d:%02d (date ignored)\n",
				gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
		systemTime += gmt->tm_gmtoff;
		if (ioctl(context->fd, VFDSETTIME2, &systemTime) < 0)
		{
			perror("Set FP time to system time");
			return -1;
		}
	}
	return 0;
}

static int Spark_setText(Context_t *context, char *theText)
{
	//-t command, tested on spark7162
	char text[cMAXCharsSpark + 1];
	strncpy(text, theText, cMAXCharsSpark);
	text[cMAXCharsSpark] = ' ';
	write(context->fd, text, strlen(text));
	return 0;
}

static int Spark_setLed(Context_t *context, int which, int on)
{
	//-l command, tested on spark7162
	struct aotom_ioctl_data vData;
	if (on < 0 || on > 2)
	{
		printf("Illegal LED action %d (valid is 0..2)\n", on);
		return 0;
	}
	vData.u.led.led_nr = which;
	vData.u.led.on = on;
	res = (ioctl(context->fd, VFDSETLED, &vData));
	if (res < 0)
	{
		perror("Setled");
		return -1;
	}
	return 0;
}

static int Spark_setIcon(Context_t *context, int which, int on)
{
	//-i command, tested on spark7162
	struct aotom_ioctl_data vData;
	if (which < 1 || which > 47)
	{
		printf("Illegal icon number %d (valid is 1..47)\n", which);
		return 0;
	}
	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Set icon");
		return -1;
	}
	return 0;
}

static int Spark_setBrightness(Context_t *context, int brightness)
{
	//-b command, tested on spark7162
	struct aotom_ioctl_data vData;
	if (brightness < 0 || brightness > 7)
	{
		printf("Illegal brightness level %d (valid is 0..7)\n", brightness);
		return 0;
	}
	vData.u.brightness.level = brightness;
	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("Set brightness");
		return -1;
	}
	return 0;
}

static int Spark_setLight(Context_t *context, int on)  //! actually does not switch display off, but sets brightness to zero!
{
	//-L command, tested on spark7162
	struct aotom_ioctl_data vData;
	if (on < 0 || on > 1)
	{
		printf("Illegal light value %d (valid is 0 | 1)\n", on);
		return 0;
	}
	vData.u.light.onoff = on;
	if (ioctl(context->fd, VFDDISPLAYWRITEONOFF, &vData) < 0)
	{
		perror("Set light");
		return -1;
	}
	return 0;
}

static int Spark_setDisplayTime(Context_t *context, int on)
{
	// -sf command
	time_t systemTime = time(NULL);
	struct tm *gmt;

	gmt = localtime(&systemTime); //get system time

	if ((gmt->tm_year + 1900) > 2016)
	{
		fprintf(stderr, "Set display time to %d is not supported on this receiver.\n", on);
		return -1;
	}

	fprintf(stderr, "Set display time to %d is deprecated in this receiver,\nplease use -sst option in future.\n", on);

	if (gmt->tm_year == 100)
	{
		fprintf(stderr, "Problem: RTC time not set.\n");
	}
	else
	{
		fprintf(stderr, "Setting front panel clock to\ncurrent system time: %02d:%02d:%02d %02d-%02d-%04d\n",
				gmt->tm_hour, gmt->tm_min, gmt->tm_sec, gmt->tm_mday, gmt->tm_mon + 1, gmt->tm_year + 1900);
		systemTime += gmt->tm_gmtoff;
		if (ioctl(context->fd, VFDSETTIME2, &systemTime) < 0)
		{
			perror("Set FP time to system time");
			return -1;
		}
	}
	return 0;
}

static int Spark_setFan(Context_t *context, int on)
{
	// -sf command
	fprintf(stderr, "Set fan: not implemented,\nSparks do not have a fan!\n");
	return -1;
}

static int Spark_setRF(Context_t *context, int on)
{
	// -sr command
	fprintf(stderr, "Set RF: not implemented,\nSparks do not have an RF modulator!\n");
	return -1;
}

static int Spark_setLEDb(Context_t *context, int brightness)
{
	// -lb command
	fprintf(stderr, "Set LED brightness: not implemented,\nSparks cannot control LED brightness!\n");
	return -1;
}

static int Spark_clear(Context_t *context)
{
	//-c command, tested on spark7162
	struct aotom_ioctl_data vData;
	if (ioctl(context->fd, VFDDISPLAYCLR, &vData) < 0)
	{
		perror("Clear");
		return -1;
	}
	vData.u.icon.icon_nr = 46;
	vData.u.icon.on = 0;
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Set icon");
		return -1;
	}
	vData.u.led.led_nr = 0;
	vData.u.led.on = 0;
	res = (ioctl(context->fd, VFDSETLED, &vData));
	if (res < 0)
	{
		perror("Setled (red)");
		return -1;
	}
	vData.u.led.led_nr = 1;
	vData.u.led.on = 0;
	res = (ioctl(context->fd, VFDSETLED, &vData));
	if (res < 0)
	{
		perror("Setled (green)");
		return -1;
	}
	return 0;
}

static int Spark_getWakeupReason(Context_t *context, int *reason)
{
	//-w command, tested on spark7162
	char mode[8];
	if (ioctl(context->fd, VFDGETSTARTUPSTATE, mode) < 0)
	{
		perror("Get wakeup reason");
		return -1;
	}
	if (mode[0] != '\0')  /* if we get the fp wake up reason */
	{
		*reason = mode[0] & 0xff; //get first byte
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  //echo unknown
	}
	return 0;
}

static int Spark_getVersion(Context_t *context, int *version)
{
	//-v command, tested on spark7162
	unsigned char strVersion[20];
	const char *CPU_type[3] = { "Unknown", "ATTING48", "ATTING88" };
	const char *Display_type[9] = { "Unknown", "VFD", "LCD", "DVFD", "LED", "?", "?", "?", "LBD" };
	if (ioctl(context->fd, VFDGETVERSION, &strVersion) < 0) // get version info (1x u32 4x u8)
	{
		perror("Get version info");
		return -1;
	}
	if (strVersion[0] != '\0')  /* if the version info is OK */
	{
		printf("\nFront processor version info:\n");
		printf("FP CPU type is   : %d (%s)\n", strVersion[0], CPU_type[strVersion[0]]);
		printf("Display type is  : %d (%s)\n", strVersion[4], Display_type[strVersion[4]]);
		printf("# of keys        : %d\n", strVersion[5]);
		printf("FP SW version is : %d.%d\n", strVersion[6], strVersion[7]);
		*version = strVersion[4];
	}
	else
	{
		fprintf(stderr, "Error reading version from fp\n");
		*version = -1;
	}
	return 0;
}

static int Spark_setTimeMode(Context_t *context, int timemode)
{
	// -tm command, to be built, aotom/FP permitting
	fprintf(stderr, "%s: not implemented (yet?)\n", __func__);
	return -1;
}

static int Spark_exit(Context_t *context)
{
	tSparkPrivate *private = (tSparkPrivate *)((Model_t *)context->m)->private;
	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

Model_t Spark_model =
{
	.Name             = "Edision/Spark frontpanel control utility",
	.Type             = Spark,
	.Init             = Spark_init,
	.Clear            = Spark_clear,
	.Usage            = Spark_usage,
	.SetTime          = Spark_setTime,
	.SetSTime         = Spark_setSTime,
	.SetTimer         = Spark_setTimer,
	.GetTime          = Spark_getTime,
	.GetWTime         = Spark_getWTime,
	.SetWTime         = Spark_setWTime,
	.Shutdown         = Spark_shutdown,
	.Reboot           = Spark_reboot,
	.Sleep            = NULL,
	.SetText          = Spark_setText,
	.SetLed           = Spark_setLed,
	.SetIcon          = Spark_setIcon,
	.SetBrightness    = Spark_setBrightness,
	.GetWakeupReason  = Spark_getWakeupReason,
	.SetLight         = Spark_setLight,
	.SetLedBrightness = Spark_setLEDb,
	.GetVersion       = Spark_getVersion,
	.SetRF            = Spark_setRF,
	.SetFan           = Spark_setFan,
	.GetWakeupTime    = NULL,
	.SetDisplayTime   = Spark_setDisplayTime,
	.SetTimeMode      = Spark_setTimeMode,
	.ModelSpecific    = NULL,
	.Exit             = Spark_exit
};
