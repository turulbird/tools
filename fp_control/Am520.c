/*
 * Am520.c
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
#include <linux/input.h>

#include "global.h"
#include "Am520.h"

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);

/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cEVENT_DEVICE "/dev/input/event0"

#define cMAXCharsAm520 4

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} oArgs;

oArgs vAMArgs[] =
{
	{ "-e", "  --setTimer         * ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Set the most recent timer from e2 or neutrino" },
	{ "", "                         ", "      to the frontcontroller and shutdown" },
	{ "", "                         ", "      Arg time date: Set frontcontroller wake up time to" },
	{ "", "                         ", "      time, shutdown, and wake up at given time" },
//	{ "-d", "  --shutdown         * ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
//	{ "", "                         ", "      No arg: Shut down immediately" },
//	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
//	{ "-r", "  --reboot           * ", "Args: None" },
//	{ "", "                         ", "      No arg: Reboot immediately" },
//	{ "", "                         ", "      Arg time date: Reboot at given time/date" },
	{ "-g", "  --getTime          * ", "Args: None        Display currently set frontprocessor time" },
//	{ "-gs", " --getTimeAndSet    * ", "Args: None" },
//	{ "", "                         ", "      Set system time to current frontprocessor time" },
	{ "-gw", " --getWTime         * ", "Args: None        Get the current frontcontroller wake up time" },
	{ "-st", " --setWakeTime      * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontcontroller wake up time" },
	{ "-s", "  --setTime          * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor time" },
	{ "-sst", "--setSystemTime    * ", "Args: None        Set front processor time to system time" },
//	{ "-p", "  --sleep            * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
//	{ "", "                         ", "      Reboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Args: text        Set text to frontpanel" },
//	{ "-l", "  --setLed             ", "Args: LED# int    LED#: int=brightness (0..31)" },
//	{ "-i", "  --setIcon            ", "Args: icon# 1|0   Set an icon on or off" },
//	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
//	{ "-tm", " --time_mode          ", "Arg : 0/1         Set time mode, 1 = 24h" },
	{ "-V", "  --verbose            ", "Args: None        Verbose operation" },
#if defined MODEL_SPECIFIC
	{ "-ms", " --model_specific     ", "Args: int1 [int2] [int3] ... [int12]   (note: input in hex)" },
	{ "", "                         ", "                  Model specific test function" },
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
} tAM5xxPrivate;

/* ******************* helper/misc functions ****************** */

/* Convert the time received from the FP
 * to the standard time value (seconds
 * since epoch).
 */
unsigned long getAM5xxTime(char *TimeString)
{
	unsigned int    mjd     = ((TimeString[0] & 0xFF) * 256) + (TimeString[1] & 0xFF);
	unsigned long   epoch   = ((mjd - 40587) * 86400);
	unsigned int    hour    = TimeString[2] & 0xFF;
	unsigned int    min     = TimeString[3] & 0xFF;
	unsigned int    sec     = TimeString[4] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	if (disp)
	{
		printf("epoch = %ld, MJD = %d, time = %02d:%02d:%02d\n", epoch, mjd, hour, min, sec);
	}
	return epoch;
}

/* Calculate the time value which we can pass to
 * the Crenova fp. It is an MJD time (MJD=Modified
 * Julian Date). MJD is relative to GMT so theTime
 * must be in GMT/UTC.
 */
void setAM5xxTime(time_t theTime, char *destString)
{
	struct tm *now_tm;

	now_tm = gmtime(&theTime);
#if 0
	printf("Time to set: %02d:%02d:%02d %02d-%02d-%04d\n",
		   now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, now_tm->tm_mday, now_tm->tm_mon + 1, now_tm->tm_year + 1900);
#endif
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;
	destString[0] = (mjd_int >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tAM5xxPrivate *private = malloc(sizeof(tAM5xxPrivate));
	int vFd;
	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tAM5xxPrivate));
	checkConfig(&private->display, &private->display_custom, &private->timeFormat, &private->wakeupDecrement);
	return vFd;
}

static int usage(Context_t *context, char *prg_name, char *cmd_name)
{
	int i;

	fprintf(stderr, "Usage:\n\n");
	fprintf(stderr, "%s argument [optarg1] [optarg2]\n", prg_name);
	for (i = 0; ; i++)
	{
		if (vAMArgs[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vAMArgs[i].arg) == 0) || (strstr(vAMArgs[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vAMArgs[i].arg, vAMArgs[i].arg_long, vAMArgs[i].arg_description);
		}
	}
	fprintf(stderr, "Options marked * should be the only calling argument.\n");
	return 0;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{  // -s command, OK
	struct cnbox_ioctl_data vData;

//	printf("MJDh: %x, MJDl: %x, h: %d, m: %d, s: %d\n",  
	vData.u.time.localTime = *theGMTTime;
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("setTime");
		return -1;
	}
	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{ // -g command, OK
	char fp_time[8];

	/* front controller time */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("getTime");
		return -1;
	}
	/* if we get the fp time */
	if (fp_time[0] != '\0')
	{
		/* current front controller time */
		*theGMTTime = (time_t)getAM5xxTime(fp_time);
	}
	else
	{
		fprintf(stderr, "Error reading time from FP\n");
		*theGMTTime = 0;
	}
	return 0;
}

static int setSTime(Context_t *context, time_t *theGMTTime)
{  // -sst command, OK
	time_t curTime;
	char fp_time[8];
	time_t curTimeFP;
	struct tm *ts_gmt;
	int gmt_offset;
	int proc_fs;
	FILE *proc_fs_file;

	time(&curTime); // get system time in UTC
	ts_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*ts_gmt);
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts_gmt->tm_hour + (gmt_offset / 3600), ts_gmt->tm_min, ts_gmt->tm_sec,
		ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);
	curTime += gmt_offset;
	setTime(context, &curTime); // set fp clock to local time

	/* Read fp time back */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Gettime");
		return -1;
	}
	curTimeFP = (time_t)getAM5xxTime(fp_time);
	ts_gmt = gmtime(&curTimeFP);
	printf("Front panel time set to: %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
		ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);

	// write UTC offset to /proc/stb/fp/rtc_offset
	proc_fs_file = fopen(cRTC_OFFSET_FILE, "w");
	if (proc_fs_file == NULL)
	{
		perror("Open rtc_offset");
		return -1;
	}
	proc_fs = fprintf(proc_fs_file, "%d", gmt_offset);
	if (proc_fs < 0)
	{
		perror("Write rtc_offset");
		return -1;
	}
	fclose(proc_fs_file);
	printf("Note: /proc/stb/fp/rtc_offset set to: %+d seconds.\n", gmt_offset);

	return 0;
}

static int setTimer(Context_t *context, time_t *theWakeTime)
{ // -e command, OK
	struct cnbox_ioctl_data vData;
	time_t curTime;
	time_t curTimeFP;
	time_t wakeupTime;
	struct tm *ts;
	struct tm *tsw;
	int    gmt_offset;

	gmt_offset = getUTCoffset();

	time(&curTime);  // get system time (UTC)
	curTime += gmt_offset;  // curTime must be in local time
	ts = gmtime(&curTime);
	if (disp)
	{
		printf("Current Time (local): %02d:%02d:%02d %02d-%02d-%04d\n",
				ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	}
	if (theWakeTime == NULL) // if no time specified
	{
		wakeupTime = read_timers_utc(curTime);
	}
	else
	{
		wakeupTime = *theWakeTime;
	}
	//check --> wakeupTime is set and larger curTime and no larger than a year in the future (gost)
//	if ((wakeupTime <= 0) || (curTime > wakeupTime) || (curTime < (wakeupTime - 25920000)))
	if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
	{
		/* no timer set */
		wakeupTime = curTime - 86400;  // set wake up time yesterday
		tsw = gmtime(&wakeupTime);
		fprintf(stderr, "No timer set, set FP wake up time to yesterday (%02d:%02d:%02d %02d-%02d-%04d)\n",
			   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
		setAM5xxTime(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standBy");
			return -1;
		}
	}
	else
	{
		unsigned long diff;
		char    fp_time[8];

		if (disp)
		{
			tsw = localtime(&wakeupTime);
			printf("Wake up time: %02d:%02d:%02d %02d-%02d-%04d\n",
				   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
		}
//		fprintf(stderr, "Waiting for current time from FP...\n");
		/* front controller time */
		if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
		{
			perror("getTime");
			return -1;
		}
		/* difference from now to wake up */
		diff = (unsigned long int) wakeupTime - curTime;
		/* if we get the fp time */
		if (fp_time[0] != '\0')
		{
			fprintf(stderr, "Successfully read time from FP\n");
			/* current front controller time */
			curTimeFP = (time_t) getAM5xxTime(fp_time);
			/* set FP-Time if curTime > or < 12h (gost)*/
			if (((curTimeFP - curTime) > 43200) || ((curTime - curTimeFP) > 43200))
			{
				setTime(context, &curTime);
				curTimeFP = curTime;
			}
			tsw = gmtime(&curTimeFP);
			printf("Front panel time (local): %02d:%02d:%02d %02d-%02d-%04d\n",
				   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
		}
		else
		{
			fprintf(stderr, "Error reading time, assuming local time\n");
			/* noop current time already set */
		}
		wakeupTime = curTimeFP + diff;
		setAM5xxTime(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standBy");
			return -1;
		}
	}
	return 0;
}

static int getWTime(Context_t *context, time_t *theGMTTime)
{  //-gw command: OK
	char fp_time[5];
	time_t iTime;

	/* front controller wake up time */
	if (ioctl(context->fd, VFDGETWAKEUPTIME, &fp_time) < 0)
	{
		perror("Get wakeup time");
		return -1;
	}
	/* if we get the fp wakeup time */
	if (fp_time[0] != '\0')
	{
		iTime = (time_t)getAM5xxTime(fp_time);
		*theGMTTime = iTime;
	}
	else
	{
		fprintf(stderr, "Error reading wake up time from frontprocessor\n");
		*theGMTTime = 0;
		return -1;
	}
	return 0;
}

static int setWTime(Context_t *context, time_t *theGMTTime)
{
	//-st command, OK (assumes specified time to be local)
	struct cnbox_ioctl_data vData;
	time_t curTime;
	struct tm *ts_gmt;
	char   FP_timeMJD[8];
	time_t FP_time;
	struct tm *swtm;
	int    gmt_offset;
	time_t wakeupTime;
	int    proc_fs;
	FILE   *proc_fs_file;

	// determine GMT offset
	time(&curTime);  // get system time in UTC
	ts_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*ts_gmt);

	// Check if specified time is valid; its maximum is current FP clock time plus 55804708 minutes
	if (ioctl(context->fd, VFDGETTIME, &FP_timeMJD) < 0)
	{
		perror("getTime");
		return -1;
	}
	FP_time = getAM5xxTime(FP_timeMJD); 
	ts_gmt = gmtime(&FP_time);
	if (disp)
	{
	printf("Front processor time is %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts_gmt->tm_hour,
			ts_gmt->tm_min, ts_gmt->tm_sec, ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);
	}

	wakeupTime = *theGMTTime;  // get specified wake up time
	if (((wakeupTime - FP_time)) / 60 > 16777215)
	{
		printf("CAUTION: wake up time too far in the future, setting maximum possible.\n");
		wakeupTime = FP_time + 16777215 * 60;
		if (wakeupTime < curTime)  // if overflow
		{
			wakeupTime = LONG_MAX -1;
		}
	}
	swtm = gmtime(&wakeupTime);

	if (disp)
	{
		printf("Setting wake up time to %02d:%02d:%02d %02d-%02d-%04d (local, century and seconds ignored)\n", swtm->tm_hour,
			swtm->tm_min, swtm->tm_sec, swtm->tm_mday, swtm->tm_mon + 1, swtm->tm_year + 1900);
	}
	setAM5xxTime(wakeupTime, vData.u.standby.time);
	if (ioctl(context->fd, VFDSETPOWERONTIME, &vData) < 0)
	{
		perror("Set wake up time");
		return -1;
	}

	// write UTC offset to /proc/stb/fp/rtc_offset
	proc_fs_file = fopen(cRTC_OFFSET_FILE, "w");
	if (proc_fs_file == NULL)
	{
		perror("Open rtc_offset");
		return -1;
	}
	proc_fs = fprintf(proc_fs_file, "%d", gmt_offset);
	if (proc_fs < 0)
	{
		perror("Write rtc_offset");
		return -1;
	}
	fclose(proc_fs_file);
	printf("Note: /proc/stb/fp/rtc_offset set to: %+d seconds.\n", gmt_offset);
	return 0;
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
		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent E2 timer and bye bye */
			return (setTimer(context, NULL));
		}
		usleep(100000);
	}
	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;
	struct cnbox_ioctl_data vData;
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
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, i, rd;
	struct tm *ts;
	char output[cMAXCharsAm520 + 1];
	struct input_event ev[64];
	tAM5xxPrivate *private = (tAM5xxPrivate *)((Model_t *)context->m)->private;
	vFd = open(cEVENT_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cEVENT_DEVICE);
		perror("");
		return -1;
	}
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
			strftime(output, cMAXCharsAm520 + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{
	char vHelp[cMAXCharsAm520 + 1];

	strncpy(vHelp, theText, cMAXCharsAm520);
	vHelp[cMAXCharsAm520] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int setLed(Context_t *context, int which, int on)
{
	return 0;
}

static int setLight(Context_t *context, int on)
{
	// -L command, OK
	struct cnbox_ioctl_data vData;

	vData.u.light.onoff = on;
//	setMode(context->fd);
	if (ioctl(context->fd, VFDDISPLAYWRITEONOFF, &vData) < 0)
	{
		perror("Set light");
		return -1;
	}
	return 0;
}

static int getWakeupReason(Context_t *context, eWakeupReason *reason)
{
	//-w command, OK
	int mode = -1;

	if (ioctl(context->fd, VFDGETWAKEUPMODE, &mode) < 0)
	{
		perror("Get wakeup reason");
		return -1;
	}
	if (mode != '\0')  /* if we get a fp wake up reason */
	{
		*reason = (mode & 0xff) + 1;  // get LS byte
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  //echo unknown
	}
	return 0;
}

static int Clear(Context_t *context)
{
	setText(context, "    ");
	return 0;
}

static int getVersion(Context_t *context, int *version)
{
	// -v command, OK
	int fp_version;

	if (ioctl(context->fd, VFDGETVERSION, &fp_version) < 0)  // get version info
	{
		perror("Get version info");
		return -1;
	}
	if (fp_version != '\0')  // if the version info is OK
	{
		*version = fp_version;
	}
	else
	{
		*version = -1;
	}
	return 0;
}

#if defined MODEL_SPECIFIC
static int modelSpecific(Context_t *context, char len, unsigned char *data)
{
	// -ms command
	int i, res;
	unsigned char testdata[12];

	testdata[0] = len;  //  set length
	
	printf("cn_micom ioctl: VFDTEST (0x%08x) CMD=", VFDTEST);
	for (i = 0; i < len; i++)
	{
		testdata[i + 1] = data[i];
		printf("0x%02x ", data[i] & 0xff);
	}
	printf("\n");

	memset(data, 0, 12);

//	setMode(context->fd);  // set mode 1

	res = (ioctl(context->fd, VFDTEST, &testdata) < 0);

	if (res < 0)
	{
		perror("Model specific");
		return -1;
	}
	else
	{
		if (testdata[1] == 1)
		{
			for (i = 0; i < 8; i++)
			{
				data[i] = testdata[i];  // return values
			}
		}
	}
	return testdata[0];
}
#endif

static int Exit(Context_t *context)
{
	tAM5xxPrivate *private = (tAM5xxPrivate *)((Model_t *)context->m)->private;
	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

Model_t AM5XX_model =
{
	.Name             = "Atemio AM520HD frontpanel control utility",
	.Type             = AM5xx,
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
	.SetIcon          = NULL,
	.SetBrightness    = NULL,
	.GetWakeupReason  = getWakeupReason,
	.SetLight         = setLight,
	.SetLedBrightness = NULL,
	.GetVersion       = getVersion,
	.SetRF            = NULL,
	.SetFan           = NULL,
	.SetDisplayTime   = NULL,
	.SetTimeMode      = NULL,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = modelSpecific,
#endif
	.Exit             = Exit
};
// vim:ts=4
