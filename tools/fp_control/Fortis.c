/*
 * Fortis.c
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
 *****************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------
 * 20130930 Audioniek       Fortis specific usage added.
 * 20170103 Audioniek       VFDTEST (-ms command) fixed.
 * 20170127 Audioniek       Get version fixed.
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
#include <linux/input.h>
#include <math.h>

#include "global.h"
#include "Fortis.h"

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);
static int setIcon(Context_t *context, int which, int on);

/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cRTC_OFFSET_FILE "/proc/stb/fp/rtc_offset"
#define cEVENT_DEVICE "/dev/input/event0"

#define cMAXCharsFortis 12
#define VFDGETWAKEUPTIME        0xc0425b03 // added by audioniek

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} tArgs;

tArgs vSArgs[] =
{
	{ "-e", "  --setTimer           ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Set the most recent timer from e2 or neutrino" },
	{ "", "                         ", "      to the frontcontroller and shutdown" },
	{ "", "                         ", "      Arg time date: Set frontcontroller wake up time to" },
	{ "", "                         ", "      time, shutdown, and wake up at given time" },
	{ "-d", "  --shutdown           ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Shut down immediately" },
	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
	{ "-r", "  --reboot             ", "Args: None" },
	{ "", "                         ", "      No arg: Reboot immediately" },
	{ "", "                         ", "      Arg time date: Reboot at given time/date" },
	{ "-g", "  --getTime            ", "Args: None        Display currently set frontprocessor time" },
	{ "-gs", " --getTimeAndSet      ", "Args: None" },
	{ "", "                         ", "      Set system time to current frontprocessor time" },
	{ "", "                         ", "      WARNING: system date will be 01-01-1970!" },
	{ "-gt", " --getWTime           ", "Args: None        Get the current frontcontroller wake up time" },
	{ "-st", " --setWakeTime        ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontcontroller wake up time" },
	{ "-s", "  --setTime            ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor time" },
	{ "-sst", "--setSystemTime      ", "Args: None        Set front processor time to system time" },
	{ "-p", "  --sleep              ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Reboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Args: text        Set text to frontpanel" },
	{ "-l", "  --setLed             ", "Args: LED# int    LED#: int=brightness (0..31)" },
	{ "-i", "  --setIcon            ", "Args: icon# 1|0   Set an icon on or off" },
	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
	{ "-tm", " --time_mode          ", "Args: 0/1         Set time mode" },
#if defined MODEL_SPECIFIC
	{ "-ms", " --model_specific     ", "Args: int1 [int2] [int3] ... [int16]   (note: input in hex)" },
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
} tFortisPrivate;

/* ******************* helper/misc functions ****************** */

static void setMode(int fd)
{
	struct nuvoton_ioctl_data nuvoton;

	nuvoton.u.mode.compat = 1;
	if (ioctl(fd, VFDSETMODE, &nuvoton) < 0)
	{
		perror("Set compatibility mode");
	}
}

unsigned long calcGetNuvotonTime(char *nuvotonTimeString)
{
	unsigned int    mjd     = ((nuvotonTimeString[0] & 0xFF) * 256) + (nuvotonTimeString[1] & 0xFF);
	unsigned long   epoch   = ((mjd - 40587) * 86400);
	unsigned int    hour    = nuvotonTimeString[2] & 0xFF;
	unsigned int    min     = nuvotonTimeString[3] & 0xFF;
	unsigned int    sec     = nuvotonTimeString[4] & 0xFF;

	epoch += (hour * 3600 + min * 60 + sec);
//	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd, epoch, hour, min, sec);
	return epoch;
}

/* Calculate the time value which we can pass to
 * the nuvoton fp. it is a mjd time (mjd=modified
 * julian date). mjd is relative to gmt so theTime
 * must be in GMT/UTC.
 */
void calcSetNuvotonTime(time_t theTime, char *destString)
{
	struct tm *now_tm;

	now_tm = gmtime(&theTime);
//	printf("Input time to set: %02d:%02d:%02d %02d-%02d-%04d\n", now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec,
//		 now_tm->tm_mday, now_tm->tm_mon+1, now_tm->tm_year+1900);
	double mjd = modJulianDate(now_tm);
	int mjd_int = mjd;
	destString[0] = (mjd_int >> 8);
	destString[1] = (mjd_int & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
//	printf("Converted time to set: %02d:%02d:%02d MJD=%d\n", destString[2], destString[3], destString[4], mjd_int);
}

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tFortisPrivate *private = malloc(sizeof(tFortisPrivate));
	int vFd;
	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tFortisPrivate));
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
		if (vSArgs[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vSArgs[i].arg) == 0) || (strstr(vSArgs[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vSArgs[i].arg, vSArgs[i].arg_long, vSArgs[i].arg_description);
		}
	}
	return 0;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{
	// -s command, OK
	struct nuvoton_ioctl_data vData;

	calcSetNuvotonTime(*theGMTTime, vData.u.time.time);
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("Set time");
		return -1;
	}
	return 0;
}

static int setSTime(Context_t *context, time_t *theGMTTime)
{
	// -sst command, OK
	time_t curTime;
	char fp_time[8];
	time_t curTimeFP;
	struct tm *ts;
	int proc_fs;
	FILE *proc_fs_file;

	time(&curTime);  //get system time (UTC)
	ts = localtime(&curTime);  // get local time
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts->tm_hour, ts->tm_min, ts->tm_sec,
		ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	setTime(context, &curTime); //set fp clock to local time

	/* Read fp time back */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Gettime");
		return -1;
	}
	curTimeFP = (time_t)calcGetNuvotonTime(fp_time);
	ts = localtime(&curTimeFP);
	printf("Front panel time set to: %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts->tm_hour, ts->tm_min, ts->tm_sec,
		ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	ts = gmtime(&curTimeFP); //get UTC offset

	// write UTC offset to /proc/stb/fp/rtc_offset
	proc_fs_file = fopen(cRTC_OFFSET_FILE, "w");
	if (proc_fs_file == NULL)
	{
		perror("Open rtc_offset");
		return -1;
	}
	proc_fs = fprintf(proc_fs_file, "%d", (int)ts->tm_gmtoff);
	if (proc_fs < 0)
	{
		perror("Write rtc_offset");
		return -1;
	}
	fclose(proc_fs_file);
	fprintf(stderr, "/proc/stb/fp/rtc_offset set to: %+d seconds\n", (int)ts->tm_gmtoff);
	return 0;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{
	// -g command, OK
	char fp_time[8];

	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Get time");
		return -1;
	}
	/* if we get the fp time */
	if (fp_time[0] != '\0')
	{
		*theGMTTime = (time_t)calcGetNuvotonTime(fp_time);
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
	// -e command, OK
	struct nuvoton_ioctl_data vData;
	time_t curTime;
	time_t curTimeFP = 0;
	time_t wakeupTime;
	struct tm *ts;
	struct tm *tsw;

	time(&curTime);
	ts = localtime(&curTime);
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (local)\n", ts->tm_hour, ts->tm_min, ts->tm_sec,
		ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
	if (theGMTTime == NULL) // -e no argument = shutdown until next e2/neutrino timer
	{
		wakeupTime = read_timers_utc(curTime); //get current 1st timer
	}
	else
	{
		wakeupTime = *theGMTTime; //get specified time
	}
	//check --> wakeupTime is set and larger than curTime and no larger than 300 days in the future (gost)
	//check --> there is no timer set
	if ((wakeupTime <= 0) || ((wakeupTime == LONG_MAX)) || (curTime > wakeupTime) || (curTime < (wakeupTime - 25920000)))
	{
		/* shut down immedately */
		fprintf(stderr, "No timers set or timer more than 300 days ahead, or wake up time in the past.\n");
//		vData.u.standby.time[0] = '\0'; //Set wake up time in the past
		wakeupTime = LONG_MAX; //Set wake up time to max. in the future
	}
	else //wake up time valid and in the coming 300 days
	{
		unsigned long diff;
		char fp_time[8];

//		tsw = localtime(&wakeupTime);
//		printf("Wake up Time: %02d:%02d:%02d %02d-%02d-%04d (local)\n", tsw->tm_hour, tsw->tm_min, tsw->tm_sec,
//			tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);

		/* Determine difference between system time and wake up time */
		diff = (unsigned long int) wakeupTime - curTime;

		/* Check if front panel clock is set prpoperly */
		if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
		{
			perror("Gettime");
			return -1;
		}
		if (fp_time[0] != '\0')
		{
			curTimeFP = (time_t) calcGetNuvotonTime(fp_time);
			/* set FP-Time if system time of more than 1 hour off */
			if (((curTimeFP - curTime) > 3600) || ((curTime - curTimeFP) > 3600))
			{
				printf("Time difference between fp and system: %d seconds.\n", (int)(curTimeFP - curTime));
				setTime(context, &curTime); //sync fp clock
				curTimeFP = curTime;
				tsw = localtime(&curTimeFP);
				printf("Front panel time corrected, set to: %02d:%02d:%02d %02d-%02d-%04d (local)\n",
					tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon+1, tsw->tm_year + 1900);
			}
		}
		else
		{
			fprintf(stderr, "Error reading front panel time; ...assuming system time.\n");
			curTimeFP = curTime;
		}
		wakeupTime = curTimeFP + diff;
	}
	tsw = localtime(&wakeupTime);
	printf("Wake up Time: %02d:%02d:%02d %02d-%02d-%04d (local)\n", tsw->tm_hour, tsw->tm_min,
		tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
	calcSetNuvotonTime(wakeupTime, vData.u.standby.time);
	if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
	{
		perror("Shut down");
		return -1;
	}
	return 0;
}

static int getWTime(Context_t *context, time_t *theGMTTime)
{
	//-gt command: VFDGETWAKEUPTIME not supported by older nuvotons
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
		iTime = (time_t)calcGetNuvotonTime(fp_time);
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
	//-st command
	struct nuvoton_ioctl_data vData;
	struct tm *swtm;
	time_t wakeupTime;
	int proc_fs;
	FILE *proc_fs_file;

	wakeupTime = *theGMTTime;
	swtm = localtime(&wakeupTime);
	fprintf(stderr, "Setting wake up time to %02d:%02d:%02d %02d-%02d-%04d (local, seconds ignored) %+d\n", swtm->tm_hour,
		swtm->tm_min, swtm->tm_sec, swtm->tm_mday, swtm->tm_mon + 1, swtm->tm_year + 1900, (int)swtm->tm_gmtoff);

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

	wakeupTime -= swtm->tm_gmtoff; //get wake up time in UTC

	calcSetNuvotonTime(wakeupTime, vData.u.standby.time);
	if (ioctl(context->fd, VFDSETPOWERONTIME, &vData) < 0)
	{
		perror("Set wake up time");
		return -1;
	}
	return 0;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{
	// -d command to check
	time_t curTime;

	/* shutdown immediate */
	if (*shutdownTimeGMT == -1)
	{
		return (setTimer(context, NULL));
	}
	while (1)
	{
		time(&curTime);
		if (curTime >= *shutdownTimeGMT)
		{
			/* set most recent e2 timer and shut down */
			return (setTimer(context, NULL));
		}
		usleep(100000);
	}
	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	//-r command to check
	time_t curTime;
	struct nuvoton_ioctl_data vData;

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

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	// -p command, to be checked
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, i, rd;
	struct tm *ts;
	char output[cMAXCharsFortis + 1];
	struct input_event ev[64];

	tFortisPrivate *private = (tFortisPrivate *)((Model_t *)context->m)->private;
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
			strftime(output, cMAXCharsFortis + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{
	// -t command to check
	char vHelp[128];

	strncpy(vHelp, theText, cMAXCharsFortis);
	vHelp[cMAXCharsFortis] = '\0';
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

static int setLed(Context_t *context, int which, int level)
{
	// -l command, OK
	struct nuvoton_ioctl_data vData;

	if (level < 0 || level > 31)
	{
		printf("Illegal brightness level %d (valid is 0..31)\n", level);
		return 0;
	}

	if (which < 8)
	{
		if (which < 0)
		{
			printf("Illegal LED number %d (valid is 0..7)\n", which);
			return 0;
		}
		else
		{
			vData.u.led.led_nr = 1 << which; //LED# is bitwise in Fortis
		}
	}
	else // allow more than one led to be set at once
	{
		vData.u.led.led_nr = which;
	}

	vData.u.led.on = level;
	setMode(context->fd);
	if (ioctl(context->fd, VFDSETLED, &vData) < 0)
	{
		perror("SetLED");
		return -1;
	}
	usleep(100000); //allow frontprocessor to keep up
	return 0;
}

static int setIcon(Context_t *context, int which, int on)
{
	// -i command, OK
	struct nuvoton_ioctl_data vData;

	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	setMode(context->fd);
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Set icon");
		return -1;
	}
	return 0;
}

static int setBrightness(Context_t *context, int brightness)
{
	//-b command, OK
	struct nuvoton_ioctl_data vData;

	if (brightness < 0 || brightness > 7)
	{
		printf("Illegal brightness level %d (valid is 0..7)\n", brightness);
		return -1;
	}
	vData.u.brightness.level = brightness;
	setMode(context->fd);
	if (ioctl(context->fd, VFDBRIGHTNESS, &vData) < 0)
	{
		perror("Set VFD brightness");
		return -1;
	}
	return 0;
}

static int Clear(Context_t *context)
{
	// -c command, OK
	int i;
	int led_nr;

	setText(context, "            ");  //show no text
	led_nr = 1;
	for (i = 0; i <= 7; i++)
	{
		setLed(context, led_nr, 0); //all leds off
	}
	for (i = 1; i < 40; i++) //all icons off
	{
		setIcon(context, i, 0);
	}
	return 0;
}

static int setLight(Context_t *context, int on)
{
	// -L command, OK
	struct nuvoton_ioctl_data vData;

	vData.u.light.onoff = on;
//	setMode(context->fd);
	if (ioctl(context->fd, VFDDISPLAYWRITEONOFF, &vData) < 0)
	{
		perror("Set light");
		return -1;
	}
	return 0;
}

static int getWakeupReason(Context_t *context, int *reason)
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
		*reason = mode & 0xff; //get LS byte
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  //echo unknown
	}
	return 0;
}

static int getVersion(Context_t *context, int *version)
{
	//-v command
	int fp_version;
	int resellerID;

	if (ioctl(context->fd, VFDGETVERSION, &fp_version) < 0) // get version info (1x u32)
	{
		perror("Get version info");
		return -1;
	}
	if (fp_version != '\0')  /* if the version info is OK */
	{
		*version = fp_version;
	}
	else
	{
		*version = -1;
	}
	return 0;
}

static int setTimeMode(Context_t *context, int timemode)
{
	// -tm command, OK
	struct nuvoton_ioctl_data vData;

	vData.u.timemode.timemode = timemode;
//	setMode(context->fd);
	if (ioctl(context->fd, VFDSETTIMEFORMAT, &vData) < 0)
	{
		perror("Set time mode");
		return -1;
	}
	return 0;
}

#if defined MODEL_SPECIFIC
static int modelSpecific(Context_t *context, char len, char *data)
{
	//-ms command
	int i, res;
	char testdata[18];

	testdata[0] = len; // set length
	
	printf("nuvoton ioctl: VFDTEST (0x%08x) SOP CMD=", VFDTEST);
	for (i = 0; i < len; i++)
	{
		testdata[i + 1] = data[i];
		printf("0x%02x ", data[i] & 0xff);
	}
	printf("EOP\n");

	memset(data, 0, 9);

//	setMode(context->fd); //set mode 1

	res = (ioctl(context->fd, VFDTEST, &testdata) < 0);

	if (res < 0)
	{
		perror("Model specific");
		return -1;
	}
	else
	{
		for (i = 0; i < ((testdata[1] == 1) ? 11 : 2); i++)
		{
				data[i] = testdata[i]; //return values
		}
	}
	return testdata[0];
}
#endif

static int Exit(Context_t *context)
{
	tFortisPrivate *private = (tFortisPrivate *)((Model_t *)context->m)->private;
	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

Model_t Fortis_model =
{
	.Name             = "Fortis frontpanel control utility",
	.Type             = Fortis,
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
	.SetLedBrightness = NULL,
	.GetVersion       = getVersion,
	.SetRF            = NULL,
	.SetFan           = NULL,
	.GetWakeupTime    = getWTime,
	.SetDisplayTime   = NULL,
	.SetTimeMode      = setTimeMode,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = modelSpecific,
#endif
	.Exit             = Exit
};
