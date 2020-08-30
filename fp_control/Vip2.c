/*
 * Vip2.c
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
 ****************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------
 * 20200702 Audioniek       Usage added.
 * 20200702 Audioniek       Get wake up reason made functional.
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
#include "Vip2.h"

static int Vip2_setText(Context_t *context, char *theText);
unsigned int fp_type;
unsigned int time_mode;

/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cRTC_OFFSET_FILE "/proc/stb/fp/rtc_offset"
#define cMAXCharsVIP2 8  // TODO: assumption is that all VIP2's have a VFD (DVFD models may exist!)

typedef struct
{
	char    *arg;
	char    *arg_long;
	char    *arg_description;
} tArgs;

tArgs vEArgs[] =
{
	{ "-e", "  --setTimer           ", "Args: [time date] (format: HH:MM:SS dd-mm-YYYY)" },
	{ "", "                         ", "      No arg:     Set the most recent timer from e2 or neutrino" },
	{ "", "                         ", "                  to the frontcontroller and shutdown" },
	{ "", "                         ", "      Arg time date: Set frontcontroller wake-up time to" },
	{ "", "                         ", "                  time, shutdown, and wake up at given time" },
	{ "-d", "  --shutdown           ", "Args: [time date] (format: HH:MM:SS dd-mm-YYYY)" },
	{ "", "                         ", "      No arg:     Shut down immediately" },
	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
	{ "-r", "  --reboot             ", "Args: [time date] (format: HH:MM:SS dd-mm-YYYY)" },
	{ "", "                         ", "      No arg:     Reboot immediately (= -e current time+5 date today)" },
	{ "", "                         ", "      Arg time date: Reboot at given time/date (= -e time date)" },
	{ "-g", "  --getTime            ", "Args: None        Display currently set frontprocessor time" },
	{ "-gs", " --getTimeAndSet      ", "Args: None        Set system time to current frontprocessor time" },
	{ "-gt", " --getWTime           ", "Args: None        Get the current frontcontroller wake up time" },
	{ "-s", "  --setTime            ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "                  Set the frontprocessor time (date ignored)" },
	{ "", "                         ", "      WARNING:    front panel date will be 01-01-1970!" },
	{ "-st", " --setWTime           ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "                  Set the frontprocessor wake up time" },
	{ "-sst", "--setSystemTime      ", "Args: None        Set frontprocessor time to current system time" },
//	{ "-p", "  --sleep              ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY\n\t\tReboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Arg : text        Show text in front panel display" },
	{ "-l", "  --setLed             ", "Args: LED# 0|1|2  LED#: off, on or blink" },
	{ "-i", "  --setIcon            ", "Args: icon# 0|1   Set an icon off or on" },
	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness (VFD/DVFD only)" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
//	{ "-tm", " --time_mode          ", "Arg : 0|1         Clock display off|on (DVFD only)" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display off|on" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
	{ "-V", "  --verbose            ", "Args: None        Verbose operation" },
#if defined MODEL_SPECIFIC
	{ "-ms ", "--set_model_specific ", "Args: long long   Model specific set function" },
#endif
	{ NULL, NULL, NULL }
};

typedef struct
{
	int display;
	int display_custom;
	char *timeFormat;

	time_t wakeupTime;
	int wakeupDecrement;
} tVIP2Private;

/* ******************* helper/misc functions ****************** */

/* Calculate the time value which we can pass to
 * the aotom fp. It is a mjd time (mjd=modified
 * julian date). mjd is relative to gmt so theGMTTime
 * must be in GMT/UTC.
 */
void Vip2_calcAotomTime(time_t theGMTTime, char *destString)
{
	/* from u-boot aotom */
	struct tm *now_tm;
	now_tm = localtime(&theGMTTime);
#if 0
	printf("Set Time (local): %02d:%02d:%02d %02d-%02d-%04d\n",
		now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_tm->tm_mday, now_tm->tm_mon+1, now_tm->tm_year+1900);
#endif
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;

	destString[0] = ((mjd_int & 0xff00) >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

unsigned long Vip2_getAotomTime(char *aotomTimeString)
{
	unsigned int  mjd   = ((aotomTimeString[1] & 0xff) * 256) + (aotomTimeString[2] & 0xff);
	unsigned long epoch = ((mjd - 40587) * 86400);  // 40587 = 01-01-1970

	unsigned int  hour  = aotomTimeString[3] & 0xff;
	unsigned int  min   = aotomTimeString[4] & 0xff;
	unsigned int  sec   = aotomTimeString[5] & 0xff;
	epoch += (hour * 3600 + min * 60 + sec);
//	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd, epoch, hour, min, sec);
	return epoch;
}

/* ******************* driver functions ****************** */

static int Vip2_init(Context_t *context)
{
	tVIP2Private *private = malloc(sizeof(tVIP2Private));
	int vFd;
	int t_mode;
	unsigned char strVersion[20];
	const char *dp_type[9] = { "Unknown", "VFD", "LCD", "DVFD", "LED", "?", "?", "?", "LBD" };
	const char *tm_type[2] = { "off", "on" };

	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}

	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tVIP2Private));
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	// Determine display type by asking aotom
	if (ioctl(vFd, VFDGETVERSION, &strVersion) < 0) // get version info (1x u32 4x u8)
	{
		perror("Get version info");
		return -1;
	}
	if (strVersion[0] != '\0')  /* if the version info is OK */
	{
		fp_type = strVersion[4];
	}
	else
	{
		fprintf(stderr, "Error reading version from fp\n");
	}

	if (ioctl(vFd, VFDGETDISPLAYTIME, &t_mode) < 0)
	{
		time_mode = 1;  // if no support from aotom, assume clock on
	}
	else
	{
		time_mode = t_mode;
	}
	if (disp)
	{
		printf("FP type is   : %s", dp_type[fp_type]);
		if (fp_type == 3)
		{
			printf(", time mode %s\n\n", tm_type[time_mode]);
		}
		else
		{
			printf("\n\n");
		}

	}
	return vFd;
}

static int Vip2_usage(Context_t *context, char *prg_name, char *cmd_name)
{
	int i;

	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "%s argument [optarg1] [optarg2]\n", prg_name);

	for (i = 0; ; i++)
	{
		if (vEArgs[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vEArgs[i].arg) == 0) || (strstr(vEArgs[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vEArgs[i].arg, vEArgs[i].arg_long, vEArgs[i].arg_description);
		}
	}
	return 0;
}

static int Vip2_setTimer(Context_t *context, time_t *theGMTTime)
{
	// -e command
	struct aotom_ioctl_data vData;
	time_t curTime;
	time_t wakeupTime;
	struct tm *ts;

	time(&curTime);
	ts = localtime(&curTime);
	fprintf(stderr, "Current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
		ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon+1, ts->tm_year+1900);

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
		fprintf(stderr, "no e2 timer found clearing fp wakeup time ... good bye ...\n");
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
//			fprintf(stderr, "success reading time from fp\n");

			/* current front controller time */
			curTime = (time_t) Vip2_getAotomTime(fp_time);
		}
		else
		{
			fprintf(stderr, "Error reading time, assuming localtime.\n");
			/* noop current time already set */
		}
		wakeupTime = curTime + diff;
		Vip2_calcAotomTime(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("Shut down until wake up time");
			return -1;
		}
	}
	return 0;
}

static int Vip2_shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	// -d command
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
	while (1)
	{
		time(&curTime);
		/*printf("curTime = %d, shutdown %d\n", curTime, *shutdownTimeGMT);*/
		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent e2 timer and bye bye */
//			return (setTimer(context, NULL));
			return 0;
		}
		usleep(100000);
	}
	return -1;
}

static int Vip2_reboot(Context_t *context, time_t *rebootTimeGMT)
{
	// -r command
	// Note: aotom does not have a particular reboot command
	time_t curTime;
	struct aotom_ioctl_data vData;

	while (1)
	{
		time(&curTime);
		if (curTime >= *rebootTimeGMT)
		{
			if (ioctl(context->fd, VFDREBOOT, &vData) < 0)
			{
				perror("Reboot");
				return -1;
			}
		}
		usleep(100000);
	}
	return 0;
}

static int Vip2_getTime(Context_t *context, time_t *theGMTTime)
{
	// -g and -gs commands, copied from Spark
	struct tm *g_tm;
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
		/* current frontcontroller time */
		*theGMTTime = iTime;
		g_tm = gmtime(&iTime);
		printf("Frontprocessor time: %02d:%02d:%02d %02d-%02d-%04d\n", g_tm->tm_hour,
			g_tm->tm_min, g_tm->tm_sec, g_tm->tm_mday, g_tm->tm_mon + 1, g_tm->tm_year + 1900);
	}
	else
	{
		fprintf(stderr, "Error reading time from frontprocessor\n");
		*theGMTTime = 0;
		return -1;
	}
	return 1;
}

static int Vip2_getWTime(Context_t *context, time_t *theGMTTime)
{
	//-gt command, note: VFDGETWAKEUPTIME not supported by older aotoms
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}

static int Vip2_setTime(Context_t *context, time_t *theGMTTime)
{
	// -s command
	time_t sTime;
	struct tm *s_tm;

	sTime = *theGMTTime;
	s_tm = localtime(&sTime); //get the time the user put in and the matching GMT offset

	sTime += s_tm->tm_gmtoff;
	if (ioctl(context->fd, VFDSETTIME2, &sTime) < 0)
	{
		perror("Set time");
		return -1;
	}
	fprintf(stderr, "Frontprocessor time set to: %02d:%02d:%02d %02d-%02d-%04d (local)\n", s_tm->tm_hour,
		s_tm->tm_min, s_tm->tm_sec, s_tm->tm_mday, s_tm->tm_mon + 1, s_tm->tm_year + 1900);
	return 1;
}

static int Vip2_setWTime(Context_t *context, time_t *theGMTTime)
{
	//-st command
	struct tm *swtm;
	time_t iTime;
	int proc_fs;
	FILE *proc_fs_file;

	iTime = *theGMTTime;
	swtm = localtime(&iTime);
	fprintf(stderr, "Setting wake up time to %02d:%02d:%02d %02d-%02d-%04d\n", swtm->tm_hour,
		swtm->tm_min, swtm->tm_sec, swtm->tm_mday, swtm->tm_mon + 1, swtm->tm_year + 1900);

	iTime += swtm->tm_gmtoff;
	if (ioctl(context->fd, VFDSETPOWERONTIME, &iTime) < 0)
	{
		perror("Set wake up time");
		return -1;

		// write UTC offset to /proc/stb/fp/rtc_offset
		proc_fs_file = fopen(cRTC_OFFSET_FILE, "w");
		if (proc_fs_file == NULL)
		{
			perror("Open rtc_offset");
			return -1;
		}
		proc_fs = fprintf(proc_fs_file, "%d", (int)swtm->tm_gmtoff);
		if (proc_fs < 0)
		{
			perror("Write rtc_offset");
			return -1;
		}
		fclose(proc_fs_file);
		fprintf(stderr, "/proc/stb/fp/rtc_offset set to: %+d seconds\n", (int)swtm->tm_gmtoff);
	}
	return 0;
}

static int Vip2_setSTime(Context_t *context, time_t *theGMTTime)
{
	// -sst command, OK
	time_t systemTime = time(NULL);
	struct tm *sst;
	int proc_fs;
	FILE *proc_fs_file;

	sst = localtime(&systemTime);
	if (sst->tm_year == 100)
	{
		fprintf(stderr, "Problem: system time not set.\n");
	}
	else
	{
		systemTime += sst->tm_gmtoff;
		// set front panel clock to local time
		if (ioctl(context->fd, VFDSETTIME2, &systemTime) < 0)
		{
			perror("Set FP time to system time");
			return -1;
		}
		fprintf(stderr, "Frontprocessor time set to: %02d:%02d:%02d %02d-%02d-%04d\n", sst->tm_hour,
			 sst->tm_min, sst->tm_sec, sst->tm_mday, sst->tm_mon + 1, sst->tm_year + 1900);

		// write UTC offset to /proc/stb/fp/rtc_offset
		proc_fs_file = fopen(cRTC_OFFSET_FILE, "w");
		if (proc_fs_file == NULL)
		{
			perror("Open rtc_offset");
			return -1;
		}
		proc_fs = fprintf(proc_fs_file, "%d", (int)sst->tm_gmtoff);
		if (proc_fs < 0)
		{
			perror("Write rtc_offset");
			return -1;
		}
		fclose(proc_fs_file);
		fprintf(stderr, "/proc/stb/fp/rtc_offset set to: %+d seconds\n", (int)sst->tm_gmtoff);
	}
	return 0;
}

static int Vip2_setText(Context_t *context, char *theText)
{
	// -t command, OK
	char text[cMAXCharsVIP2 + 1];

	strncpy(text, theText, cMAXCharsVIP2);
	text[cMAXCharsVIP2] = '\0';
	write(context->fd, text, strlen(text));
	return 0;
}

static int Vip2_setLed(Context_t *context, int which, int on)
{
	struct aotom_ioctl_data vData;

	if (on < 0 || on > 255)
	{
		printf("Illegal LED action %d (valid is 0..255)\n", on);
		return 0;
	}
	vData.u.led.led_nr = which;
	vData.u.led.on = on;
	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("Setled");
		return -1;
	}
	return 0;
}

static int Vip2_setIcon(Context_t *context, int which, int on)
{
	// -i command, OK  TODO/assumption: all VIP2's have a VFD
	struct aotom_ioctl_data vData;

	if ((which < 1 || which > 47))
	{
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

static int Vip2_setBrightness(Context_t *context, int brightness)
{
	// -b command
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

static int Vip2_setLight(Context_t *context, int on)
{
	// -L command
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

static int Vip2_setDisplayTime(Context_t *context, int on)
{
	// -dt command
	time_t systemTime = time(NULL);
	struct tm *gmt;

	gmt = localtime(&systemTime); //get system time

	if ((gmt->tm_year + 1900) > 2016)
	{
		fprintf(stderr, "Option -dt is not supported on this receiver.\n");
		return -1;
	}

	fprintf(stderr, "Option -dt is deprecated in this receiver,\nplease use -sst or -s option in future.\n");

	if (gmt->tm_year == 100)
	{
		fprintf(stderr, "Problem: system time not set.\n");
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

static int Vip2_setFan(Context_t *context, int on)
{
	// -sf command
	fprintf(stderr, "Set fan: not implemented,\nThis model does not have a fan!\n");
	return -1;
}

static int Vip2_setRF(Context_t *context, int on)
{
	// -sr command
	fprintf(stderr, "Set RF: not implemented,\nThis model does not have an RF modulator!\n");
	return -1;
}

static int Vip2_setLEDb(Context_t *context, int brightness)
{
	// -lb command
	fprintf(stderr, "Set LED brightness: not implemented,\nThis modles cannot control LED brightness!\n");
	return -1;
}

#if 0
static int Vip2_Clear(Context_t *context)
{
	struct aotom_ioctl_data vData;
	if (ioctl(context->fd, VFDDISPLAYCLR, &vData) < 0)
	{
		perror("clear: ");
		return -1;
	}
	return 0;
}
#endif

static int Vip2_Clear(Context_t *context)
{
	// -c command
	int res;
	struct aotom_ioctl_data vData;

	if (ioctl(context->fd, VFDDISPLAYCLR, &vData) < 0)
	{
		perror("Clear");
		return -1;
	}
	if (fp_type == 1 || fp_type == 3)
	{
		vData.u.icon.icon_nr = 46;
		vData.u.icon.on = 0;
		if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
		{
			perror("Set icon");
			return -1;
		}
	}
	vData.u.led.led_nr = 0;
	vData.u.led.on = 0;
	res = (ioctl(context->fd, VFDSETLED, &vData));
	if (res < 0)
	{
		perror("Setled (red)");
		return -1;
	}
#if 0
	if (fp_type != 1)
	{
		vData.u.led.led_nr = 1;
		vData.u.led.on = 0;
		res = (ioctl(context->fd, VFDSETLED, &vData));
		if (res < 0)
		{
			perror("Setled (green)");
			return -1;
		}
	}
#endif
	return 0;
}

static int Vip2_getWakeupReason(Context_t *context, eWakeupReason *reason)
{
	// -w command
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

static int Vip2_getVersion(Context_t *context, int *version)
{
	// -v command
	unsigned char strVersion[20];
	const char *CPU_type[3] = { "Unknown", "ATtiny48", "ATtiny88" };
	const char *Display_type[9] = { "Unknown", "VFD", "LCD", "DVFD", "LED", "?", "?", "?", "LBD" };

	if (ioctl(context->fd, VFDGETVERSION, &strVersion) < 0) // get version info (1x u32 4x u8)
	{
		perror("Get version info");
		return -1;
	}
	if (strVersion[0] != '\0')  /* if the version info is OK */
	{
		printf("Front processor version info:\n");
		printf("FP CPU type is   : %d (%s)\n", strVersion[0], CPU_type[strVersion[0]]);
		printf("Display type is  : %d (%s)\n", strVersion[4], Display_type[strVersion[4]]);
		printf("# of keys        : %d\n", strVersion[5]);
		printf("FP SW version is : %d.%d\n", strVersion[6], strVersion[7]);
		*version = strVersion[6] * 100 + strVersion[7];
	}
	else
	{
		fprintf(stderr, "Error reading version from fp\n");
		*version = -1;
	}
	return 0;
}
static int Vip2_Exit(Context_t *context)
{
	tVIP2Private *private = (tVIP2Private *)((Model_t *)context->m)->private;

	if (context->fd > 0)
	{
		close(context->fd);
	}

	free(private);
	exit(1);
}

Model_t VIP2_model =
{
	.Name             = "Edision VIP2 frontpanel control utility",
	.Type             = Vip2,
	.Init             = Vip2_init,
	.Clear            = Vip2_Clear,
	.Usage            = Vip2_usage,
	.SetTime          = Vip2_setTime,
	.SetSTime         = Vip2_setSTime,
	.SetTimer         = Vip2_setTimer,
	.GetTime          = Vip2_getTime,
	.GetWTime         = Vip2_getWTime,
	.SetWTime         = Vip2_setWTime,
	.Shutdown         = Vip2_shutdown,
	.Reboot           = Vip2_reboot,
	.Sleep            = NULL,
	.SetText          = Vip2_setText,
	.SetLed           = Vip2_setLed,
	.SetIcon          = Vip2_setIcon,
	.SetBrightness    = Vip2_setBrightness,
	.GetWakeupReason  = Vip2_getWakeupReason,
	.SetLight         = Vip2_setLight,
	.SetLedBrightness = Vip2_setLEDb,
	.GetVersion       = Vip2_getVersion,
	.SetRF            = Vip2_setRF,
	.SetFan           = Vip2_setFan,
	.SetDisplayTime   = Vip2_setDisplayTime,
	.SetTimeMode      = NULL,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = NULL,
#endif
	.Exit             = Vip2_Exit
};
