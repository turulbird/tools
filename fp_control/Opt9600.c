/*
 * Opt9600.c
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
#include "Opt9600.h"

static int setText(Context_t *context, char *theText);
static int Clear(Context_t *context);

/* ******************* constants ************************ */

#define cVFD_DEVICE "/dev/vfd"
#define cEVENT_DEVICE "/dev/input/event0"

#define cMAXCharsOpt9600 12

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} oArgs;

oArgs vOArgs[] =
{
//	{ "-e", "  --setTimer         * ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
//	{ "", "                         ", "      No arg: Set the most recent timer from e2 or neutrino" },
//	{ "", "                         ", "      to the frontcontroller and shutdown" },
//	{ "", "                         ", "      Arg time date: Set frontcontroller wake up time to" },
//	{ "", "                         ", "      time, shutdown, and wake up at given time" },
//	{ "-d", "  --shutdown         * ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
//	{ "", "                         ", "      No arg: Shut down immediately" },
//	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
//	{ "-r", "  --reboot           * ", "Args: None" },
//	{ "", "                         ", "      No arg: Reboot immediately" },
//	{ "", "                         ", "      Arg time date: Reboot at given time/date" },
	{ "-g", "  --getTime          * ", "Args: None        Display currently set frontprocessor time" },
	{ "-gs", " --getTimeAndSet    * ", "Args: None" },
	{ "", "                         ", "      Set system time to current frontprocessor time" },
	{ "", "                         ", "      WARNING: system date will be 01-01-1970!" },
//	{ "-gw", " --getWTime         * ", "Args: None        Get the current frontcontroller wake up time" },
//	{ "-st", " --setWakeTime      * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
//	{ "", "                         ", "      Set the frontcontroller wake up time" },
	{ "-s", "  --setTime          * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the frontprocessor time" },
	{ "-sst", "--setSystemTime    * ", "Args: None        Set front processor time to system time" },
	{ "-p", "  --sleep            * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Reboot receiver via fp at given time" },
	{ "-t", "  --settext            ", "Args: text        Set text to frontpanel" },
//	{ "-l", "  --setLed             ", "Args: LED# int    LED#: int=brightness (0..31)" },
//	{ "-i", "  --setIcon            ", "Args: icon# 1|0   Set an icon on or off" },
//	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons and LEDs off" },
	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
//	{ "-tm", " --time_mode          ", "Arg : 0/1         Set time mode" },
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
} tOpt9600Private;

/* ******************* helper/misc functions ****************** */

unsigned long getOpt9600Time(char *TimeString)
{
	unsigned int    mjd     = ((TimeString[0] & 0xFF) * 256) + (TimeString[1] & 0xFF);
	unsigned long   epoch   = ((mjd - 40587) * 86400);
	unsigned int    hour    = TimeString[2] & 0xFF;
	unsigned int    min     = TimeString[3] & 0xFF;
	unsigned int    sec     = TimeString[4] & 0xFF;
	epoch += (hour * 3600 + min * 60 + sec);
	printf("MJD = %d epoch = %ld, time = %02d:%02d:%02d\n", mjd, epoch, hour, min, sec);
	return epoch;
}

/* Calculate the time value which we can pass to
 * the fp. its a mjd time (mjd=modified julian
 * date). mjd is relative to gmt so theTime
 * must be in GMT/UTC.
 */
void setOpt9600Time(time_t theTime, char *destString)
{
	struct tm *now_tm;
	now_tm = gmtime(&theTime);
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

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
	tOpt9600Private *private = malloc(sizeof(tOpt9600Private));
	int vFd;
	vFd = open(cVFD_DEVICE, O_RDWR);
	if (vFd < 0)
	{
		fprintf(stderr, "Cannot open %s\n", cVFD_DEVICE);
		perror("");
	}
	((Model_t *)context->m)->private = private;
	memset(private, 0, sizeof(tOpt9600Private));
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
		if (vOArgs[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vOArgs[i].arg) == 0) || (strstr(vOArgs[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vOArgs[i].arg, vOArgs[i].arg_long, vOArgs[i].arg_description);
		}
	}
	fprintf(stderr, "Options marked * should be the only calling argument.\n");
	return 0;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{
	struct opt9600_fp_ioctl_data vData;
	vData.u.time.localTime = *theGMTTime;
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
		*theGMTTime = (time_t)getOpt9600Time(fp_time);
	}
	else
	{
		fprintf(stderr, "Error reading time from fp\n");
		*theGMTTime = 0;
	}
	return 0;
}

#if 0
static int setTimer(Context_t *context, time_t *theGMTTime)
{
	struct opt9600_fp_ioctl_data vData;
	time_t curTime;
	time_t curTimeFP;
	time_t wakeupTime;
	struct tm *ts;
	struct tm *tsw;
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
	tsw = localtime(&wakeupTime);
	printf("wakeup Time: %02d:%02d:%02d %02d-%02d-%04d\n",
		   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
	tsw = localtime(&curTime);
	printf("current Time: %02d:%02d:%02d %02d-%02d-%04d\n",
		   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
	//check --> WakupTime is set and larger curTime and no larger than a year in the future (gost)
	if ((wakeupTime <= 0) || (curTime > wakeupTime) || (curTime < (wakeupTime-25920000)))
	//if ((wakeupTime <= 0) || (wakeupTime == LONG_MAX))
	{
		/* nothing to do for e2 */
		fprintf(stderr, "no e2 timer found clearing fp wakeup time ... good bye ...\n");
		vData.u.standby.localTime = 0;
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standBy: ");
			return -1;
		}
	}
	else
	{
		unsigned long diff;
		char    fp_time[8];
		fprintf(stderr, "Waiting for current time from fp...\n");
		/* front controller time */
		if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
		{
			perror("getTime: ");
			return -1;
		}
		/* difference from now to wake up */
		diff = (unsigned long int) wakeupTime - curTime;
		/* if we get the fp time */
		if (fp_time[0] != '\0')
		{
			fprintf(stderr, "Success reading time from fp\n");
			/* current front controller time */
			curTimeFP = (time_t) getOpt9600Time(fp_time);
			/* set FP-Time if curTime > or < 12h (gost)*/
			if (((curTimeFP - curTime) > 43200) || ((curTime - curTimeFP) > 43200))
			{
				setTime(context, &curTime);
				curTimeFP = curTime;
			}
			tsw = gmtime(&curTimeFP);
			printf("fp_time (UTC): %02d:%02d:%02d %02d-%02d-%04d\n",
				   tsw->tm_hour, tsw->tm_min, tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
		}
		else
		{
			fprintf(stderr, "Error reading time, assuming localtime\n");
			/* noop current time already set */
		}
		wakeupTime = curTimeFP + diff;
//		setOpt9600Time(wakeupTime, vData.u.standby.time);
		if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
		{
			perror("standBy: ");
			return -1;
		}
	}
	return 0;
}
#endif

#if 0
static int getWTime(Context_t *context, time_t *theGMTTime)
{
	fprintf(stderr, "%s: not implemented\n", __func__);
	return -1;
}
#endif

#if 0
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
			/* set most recent e2 timer and bye bye */
			return (setTimer(context, NULL));
		}
		usleep(100000);
	}
	return -1;
}
#endif

#if 0
static int reboot(Context_t *context, time_t *rebootTimeGMT)
{
	time_t curTime;
	struct opt9600_fp_ioctl_data vData;

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
#endif

static int Sleep(Context_t *context, time_t *wakeUpGMT)
{
	time_t curTime;
	int sleep = 1;
	int vFd;
	fd_set rfds;
	struct timeval tv;
	int retval, i, rd;
	struct tm *ts;
	char output[cMAXCharsOpt9600 + 1];
	struct input_event ev[64];
	tOpt9600Private *private = (tOpt9600Private *)((Model_t *)context->m)->private;
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
			strftime(output, cMAXCharsOpt9600 + 1, private->timeFormat, ts);
			setText(context, output);
		}
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{
	char vHelp[cMAXCharsOpt9600 + 1];
	strncpy(vHelp, theText, cMAXCharsOpt9600);
	vHelp[cMAXCharsOpt9600] = '\0';
	/* printf("%s, %d\n", vHelp, strlen(vHelp));*/
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}

#if 0
static int setLed(Context_t *context, int which, int on)
{
	return 0;
}
#endif

static int setLight(Context_t *context, int on)
{
	// -L command, OK
	struct opt9600_fp_ioctl_data vData;

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
		*reason = mode & 0xff; //get LS byte
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  //echo unknown
	}
	return 0;
}

static int Exit(Context_t *context)
{
	tOpt9600Private *private = (tOpt9600Private *)((Model_t *)context->m)->private;
	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	exit(1);
}

static int Clear(Context_t *context)
{
	setText(context, "        ");
	return 0;
}

#if defined MODEL_SPECIFIC
static int modelSpecific(Context_t *context, char len, unsigned char *data)
{
	//-ms command
	int i, res = -1;
	unsigned char testdata[12];

	testdata[0] = len; // set length
	
	printf("mcom ioctl: VFDTEST (0x%08x) CMD=", VFDTEST);
	for (i = 0; i < len; i++)
	{
		testdata[i + 1] = data[i];
		printf("0x%02x ", data[i] & 0xff);
	}
	printf("\n");

	memset(data, 0, sizeof(testdata));

//	setMode(context->fd); // set mode 1

//	res = 0;
	res = (ioctl(context->fd, VFDTEST, &testdata) < 0);

	if (res < 0)
	{
		perror("Model specific");
		return -1;
	}
#if 1
	else
	{
		if (testdata[1] != 0)
		{
			printf("Received %d bytes back\n", testdata[1] + 2);
		}
		for (i = 0; i < ((testdata[1] != 0) ? testdata[1] + 2 : 2); i++)
		{
			data[i] = testdata[i];  // return values
		}
	}
	return testdata[0];
#else
	return res;
#endif
}
#endif


Model_t Opt9600_model =
{
	.Name             = "Opticum HD (TS) 9600 frontpanel control utility",
	.Type             = Opt9600,
	.Init             = init,
	.Clear            = Clear,
	.Usage            = usage,
	.SetTime          = setTime,
	.GetTime          = getTime,
	.SetTimer         = NULL,  // setTimer,
	.GetWTime         = NULL,  // getWTime,
	.SetWTime         = NULL,
	.Shutdown         = NULL,  // shutdown,
	.Reboot           = NULL,  // reboot,
	.Sleep            = Sleep,
	.SetText          = setText,
	.SetLed           = NULL,  // setLed,
	.SetIcon          = NULL,
	.SetBrightness    = NULL,
	.GetWakeupReason  = getWakeupReason,
	.SetLight         = setLight,
	.SetLedBrightness = NULL,
	.GetVersion       = NULL,
	.SetRF            = NULL,
	.SetFan           = NULL,
	.SetDisplayTime   = NULL,
	.SetTimeMode      = NULL,
#if defined MODEL_SPECIFIC
	.ModelSpecific    = modelSpecific,
#endif
	.Exit             = Exit
};

