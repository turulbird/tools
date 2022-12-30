/*
 * Hchchs8100.c
 * 
 * (c) 2009 corev
 * (c) 2022 Audioniek
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
#include <termios.h>
#include <linux/fb.h>
#include <linux/stmfb.h>
#include <linux/input.h>

#include "global.h"
#include "Hchs8100.h"

static int setText(Context_t *context, char *theText);
static int setIcon (Context_t *context, int which, int on);
static int Sleep(Context_t *context, time_t *wakeUpGMT);
static int setLight(Context_t *context, int on);

/******************** constants ************************ */
#define cmdReboot "init 6"
#define cmdHalt "/sbin/halt"

#define cVFD_DEVICE "/dev/vfd"
#define cCMDLINE "/proc/cmdline"

#define cMAXCharsHS8100 12
#define cMAXIconsHS8100 34
#define VFDSETFAN 0xc0425af8

typedef struct
{
	char *arg;
	char *arg_long;
	char *arg_description;
} tArgs;

tArgs vHs8100Arg[] =
{
	{ "-e", "  --setTimer         * ", "Args: [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Set the most recent timer from e2 or neutrino" },
	{ "", "                         ", "      to the RTC and shutdown" },
	{ "", "                         ", "      Arg time date: Set RTC wake up time to time, shutdown," },
	{ "", "                         ", "      and wake up at given time" },
	{ "-d", "  --shutdown         * ", "Args: None or [time date]  Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      No arg: Shut down immediately" },
	{ "", "                         ", "      Arg time date: Shut down at given time/date" },
	{ "-r", "  --reboot           * ", "Args: None" },
	{ "", "                         ", "      No arg: Reboot immediately" },
	{ "", "                         ", "      Arg time date: Reboot at given time/date" },
	{ "-g", "  --getTime          * ", "Args: None        Display current RTC time" },
	{ "-gs", " --getTimeAndSet    * ", "Args: None" },
	{ "", "                         ", "      Set system time to current RTC time" },
	{ "-gw", " --getWTime         * ", "Args: None        Get the current RTC wake up time" },
	{ "-st", " --setWakeTime      * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the RTC wake up time" },
	{ "-s", "  --setTime          * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Set the RTC time" },
	{ "-sst", "--setSystemTime    * ", "Args: None        Set RTC to system time" },
	{ "-p", "  --sleep            * ", "Args: time date   Format: HH:MM:SS dd-mm-YYYY" },
	{ "", "                         ", "      Reboot receiver via RTC at given time" },
	{ "-t", "  --settext            ", "Args: text        Set text to frontpanel" },
	{ "-i", "  --setIcon            ", "Args: icon# 1|0   Set an icon on or off" },
	{ "-b", "  --setBrightness      ", "Arg : 0..7        Set display brightness" },
	{ "-w", "  --getWakeupReason    ", "Args: None        Get the wake up reason" },
	{ "-L", "  --setLight           ", "Arg : 0|1         Set display on/off" },
	{ "-c", "  --clear              ", "Args: None        Clear display, all icons off" },
	{ "-sf", " --setFan             ", "Arg : 0..255      Set fan speed" },
//	{ "-v", "  --version            ", "Args: None        Get version info from frontprocessor" },
//	{ "-tm", " --time_mode          ", "Arg : 0/1         Set time mode" },
	{ NULL, NULL, NULL }
};

typedef struct
{
    int    vfd;
    
//	int    nfs;
    
    int    display;
    int    display_custom;
    char   *timeFormat;
    
    time_t wakeupTime;
    int    wakeupDecrement;
} tHS8100Private;

/* ----------------------------------------------------- */
void hchs8100_avs_standby(int fd_avs, unsigned int mode)
{
	printf("%s %d\n", __func__, mode);
	if (!mode)
	{
		write(fd_avs, "on", 2);
	}
	else
	{
		write(fd_avs, "off", 3);
	}
}

/* ----------------------------------------------------- */
void hchs8100_hdmi_standby(int fd_hdmi, int mode)
{
	struct stmfbio_output_configuration outputConfig = {0};

	printf("%s %d\n", __func__, mode);

	outputConfig.outputid = 1;
	if (ioctl(fd_hdmi, STMFBIO_GET_OUTPUT_CONFIG, &outputConfig) < 0)
	{
		perror("Getting current output configuration failed");
	}  
	outputConfig.caps = 0;
	outputConfig.activate = STMFBIO_ACTIVATE_IMMEDIATE;
	outputConfig.analogue_config = 0;

	outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;

	if (!mode)
	{
		outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_DISABLED;
	}
	else if (mode)
	{
		outputConfig.hdmi_config &= ~STMFBIO_OUTPUT_HDMI_DISABLED;
	}

	if (outputConfig.caps != STMFBIO_OUTPUT_CAPS_NONE)
	{
		if (ioctl(fd_hdmi, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig) < 0)
		{
			perror("setting output configuration failed");
		}
	}
}

/* ----------------------------------------------------- */

void hchs8100_startPseudoStandby(Context_t* context, tHS8100Private* private) 
{
	int id;
	int fd_avs = open("/proc/stb/avs/0/standby", O_RDWR);
	int fd_hdmi  = open("/dev/fb0",   O_RDWR);

	hchs8100_avs_standby(fd_avs, 0);
	hchs8100_hdmi_standby(fd_hdmi, 0);

	setText(context, "                ");
	for (id = 1; id < 71; id++) 
	{
		setIcon(context, id, 0);     
	}
	if (private->display == 0)
	{
		setLight(context, 0);
	}
	close(fd_hdmi);
	close(fd_avs);
}

void hchs8100_stopPseudoStandby(Context_t *context, tHS8100Private *private)
{
	int fd_avs = open("/proc/stb/avs/0/standby", O_RDWR);
	int fd_hdmi  = open("/dev/fb0",   O_RDWR);
	int id;

	if (private->display == 0)
   	{
		setLight(context, 1);
	}
	setText(context, "                ");

	for (id = 1; id < 71; id++)
	{
		setIcon(context, id, 0);
	}
	hchs8100_hdmi_standby(fd_hdmi, 1);
	hchs8100_avs_standby(fd_avs, 1);

	close(fd_hdmi);
	close(fd_avs);
}

/* ******************* helper/misc functions ****************** */

static void setMode(int fd)
{
	struct hchs8100_ioctl_data vData;

	vData.u.mode.compat = 1;
	if (ioctl(fd, VFDSETMODE, &vData) < 0)
	{
		perror("Set compatibility mode");
	}
}

unsigned long calcGetHs8100_fpTime(char *hchs8100_fpTimeString)
{
	unsigned int    mjd     = ((hchs8100_fpTimeString[0] & 0xff) << 8) + (hchs8100_fpTimeString[1] & 0xff);
	unsigned long   epoch   = ((mjd - 40587) * 86400);
	unsigned int    hour    = hchs8100_fpTimeString[2] & 0xff;
	unsigned int    min     = hchs8100_fpTimeString[3] & 0xff;
	unsigned int    sec     = hchs8100_fpTimeString[4] & 0xff;

	epoch += (hour * 3600 + min * 60 + sec);
	return epoch;
}

/* Calculate the time value which we can pass to
 * the hchs8100_fp fp. It is an MJD time (MJD=Modified
 * Julian Date). mjd is relative to gmt so theTime
 * must be in GMT/UTC.
 */
void calcSetHs8100_fpTime(time_t theTime, char *destString)
{
	struct tm *now_tm;
	int mjd;

	now_tm = gmtime(&theTime);
	
	mjd = (int)modJulianDate(now_tm);
	destString[0] = (mjd >> 8);
	destString[1] = (mjd & 0xff);
	destString[2] = now_tm->tm_hour;
	destString[3] = now_tm->tm_min;
	destString[4] = now_tm->tm_sec;
}

static int write_gmtoffset(int gmt_offset)
{
	int proc_fs;
	FILE *proc_fs_file;

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

/* ******************* driver functions ****************** */

static int init(Context_t *context)
{
//    char cmdLine[512];
    int vFd;
    tHS8100Private *private = malloc(sizeof(tHS8100Private));

	((Model_t*)context->m)->private = private;
    memset(private, 0, sizeof(tHS8100Private));

    vFd = open(cCMDLINE, O_RDWR);

#if 0
    private->nfs = 0;
    if (read(vFd, cmdLine, 512) > 0)
    {
		if (strstr("nfsroot", cmdLine) != NULL)
		{
			private->nfs = 1;
		}
    }
#endif
	close(vFd);

//    if (private->nfs)
//	{
//		printf("Mode = nfs\n");
//	}
//	else
//	{
//		printf("Mode = none nfs\n");
//	}
    close(vFd);
    vFd = open(cVFD_DEVICE, O_RDWR);

    if (vFd < 0)
    {
		fprintf(stderr, "cannot open %s\n", cVFD_DEVICE);
		perror("");
    }
//	private->vfd = open(cVFD_DEVICE, O_RDWR);
//
//    if (private->vfd < 0)
//	{
//		fprintf(stderr, "cannot open %s\n", cVFD_DEVICE);
//		perror("");
//	}
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
		if (vHs8100Arg[i].arg == NULL)
		{
			break;
		}
		if ((cmd_name == NULL) || (strcmp(cmd_name, vHs8100Arg[i].arg) == 0) || (strstr(vHs8100Arg[i].arg_long, cmd_name) != NULL))
		{
			fprintf(stderr, "%s   %s   %s\n", vHs8100Arg[i].arg, vHs8100Arg[i].arg_long, vHs8100Arg[i].arg_description);
		}
	}
	printf("Options marked * should be the only calling argument.\n");
	return 0;
}

static int setTime(Context_t *context, time_t *theGMTTime)
{  // -s command, OK
	struct tm *ts_gmt;
	int gmt_offset;
	struct hchs8100_ioctl_data vData;
	int i;

	// Assume specified time is local, but RTC is in UTC
	ts_gmt = gmtime(theGMTTime);
	gmt_offset = get_GMT_offset(*ts_gmt);
	*theGMTTime -= gmt_offset;
	calcSetHs8100_fpTime(*theGMTTime, vData.u.time.time);
	if (ioctl(context->fd, VFDSETTIME, &vData) < 0)
	{
		perror("Set time");
		return -1;
	}
	i = write_gmtoffset(gmt_offset);
	return i;
}

static int getTime(Context_t *context, time_t *theGMTTime)
{  // -g command, OK
	struct tm *ts_gmt;
	time_t curTime;
	int gmt_offset;
	char fp_time[8];

	// Assume specified time is local, but RTC is in UTC
	time(&curTime);
	ts_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*ts_gmt);
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Get time");
		return -1;
	}
	/* if we get the fp time */
	if (fp_time[0] != '\0')
	{
		*theGMTTime = (time_t)calcGetHs8100_fpTime(fp_time);
		*theGMTTime += gmt_offset;  //  return local time
	}
	else
	{
		fprintf(stderr, "Error reading time from fp\n");
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
	int i;
//	int proc_fs;
//	FILE *proc_fs_file;

	time(&curTime);
	ts_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*ts_gmt);
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (local)\n",
		(ts_gmt->tm_hour + (gmt_offset / 3600)) % 24,
		(ts_gmt->tm_min + (gmt_offset % 60)) % 60,
		ts_gmt->tm_sec,
		ts_gmt->tm_mday,
		ts_gmt->tm_mon + 1,
		ts_gmt->tm_year + 1900);
	curTime += gmt_offset;  // setTime expects local time
	setTime(context, &curTime);

	/* Read fp time back */
	if (ioctl(context->fd, VFDGETTIME, &fp_time) < 0)
	{
		perror("Gettime");
		return -1;
	}
	curTimeFP = (time_t)calcGetHs8100_fpTime(fp_time);
	ts_gmt = gmtime(&curTimeFP);
	printf("RTC time set to: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
		ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);

	i = write_gmtoffset(gmt_offset);
	return i;
}

static int setTimer(Context_t *context, time_t *theGMTTime)
{ // -e command
	time_t curTime;
	time_t curTimeRTC = 0;
	time_t wakeupTime;	struct tm *ts_gmt;
	int gmt_offset;
	struct tm *tsw;
	struct hchs8100_ioctl_data vData;

	time(&curTime);
	ts_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*ts_gmt);
	printf("Current system time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n", ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec,
		ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);

	if (theGMTTime == NULL) // -e no argument = shutdown until next e2/neutrino timer
	{
		wakeupTime = read_timers_utc(curTime); // get current 1st timer (are in UTC)
	}
	else
	{
		wakeupTime = *theGMTTime - gmt_offset; // get specified time (assumed local, convert to UTC)
	}
	// check --> wakeupTime is set and larger than curTime
	// check --> there is no timer set
	if ((wakeupTime <= 0) || ((wakeupTime == LONG_MAX)) || (curTime > wakeupTime))
	{
		/* shut down immedately */
		printf("No timers set, or all timer(s) in the past.\n");
		wakeupTime = LONG_MAX;
	}
	else // wake up time valid
	{
		unsigned long diff;
		char rtc_time[5];

		/* Determine difference between system time and wake up time */
		diff = (unsigned long int) wakeupTime - curTime;

		/* Check if RTC clock is set properly */
		if (ioctl(context->fd, VFDGETTIME, &rtc_time) < 0)
		{
			perror("Gettime");
			return -1;
		}
		if (rtc_time[0] != '\0')
		{
			curTimeRTC = (time_t)calcGetHs8100_fpTime(rtc_time);
			/* set RTC if its time is more than 5 minutes off compared to system time */
			if (((curTimeRTC - curTime) > 300) || ((curTime - curTimeRTC) > 300))
			{
				printf("Time difference between RTC and system: %+d seconds.\n", (int)(curTimeRTC - curTime));
				curTimeRTC = curTime;
				setTime(context, &curTime); // sync RTC clock
				ts_gmt = gmtime(&curTimeRTC);
				printf("RTC time corrected, set to: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n",
					ts_gmt->tm_hour, ts_gmt->tm_min, ts_gmt->tm_sec, ts_gmt->tm_mday, ts_gmt->tm_mon + 1, ts_gmt->tm_year + 1900);
			}
		}
		else
		{
			fprintf(stderr, "Error reading RTC time... using system time.\n");
			curTimeRTC = curTime;
		}
		wakeupTime = curTimeRTC + diff;
	}

	tsw = gmtime(&wakeupTime);
	printf("Wake up Time: %02d:%02d:%02d %02d-%02d-%04d (UTC)\n", tsw->tm_hour, tsw->tm_min,
		tsw->tm_sec, tsw->tm_mday, tsw->tm_mon + 1, tsw->tm_year + 1900);
	calcSetHs8100_fpTime(wakeupTime, vData.u.standby.time);
	sleep(1);
	if (ioctl(context->fd, VFDSTANDBY, &vData) < 0)
	{
		perror("Shut down");
		return -1;
	}
	return 0;
}

static int getWTime(Context_t *context, time_t *theGMTTime)
{  // -gw command, OK
	char fp_time[5];
	time_t iTime;
	int gmt_offset;
	struct tm *swtm;

	/* front controller wake up time */
	if (ioctl(context->fd, VFDGETWAKEUPTIME, &fp_time) < 0)
	{
		perror("Get wakeup time");
		return -1;
	}
	/* if we get the fp wakeup time */
	if (fp_time[0] != '\0')
	{
		iTime = (time_t)calcGetHs8100_fpTime(fp_time);
		swtm = gmtime(&iTime);
		gmt_offset = get_GMT_offset(*swtm);
		*theGMTTime = iTime + gmt_offset;  // returned time must be local
	}
	else
	{
		fprintf(stderr, "Error reading wake up time from RTC\n");
		*theGMTTime = 0;
		return -1;
	}
	return 0;
}

static int setWTime(Context_t *context, time_t *theGMTTime)
{  // -st command, OK
	struct hchs8100_ioctl_data vData;
	struct tm *swtm;
	int gmt_offset;
	time_t wakeupTime;
	int i;

	wakeupTime = *theGMTTime;
	swtm = gmtime(&wakeupTime);
	gmt_offset = get_GMT_offset(*swtm);
	printf("Setting wake up time to %02d:%02d:%02d %02d-%02d-%04d (local, seconds ignored)\n", swtm->tm_hour,
		swtm->tm_min, swtm->tm_sec, swtm->tm_mday, swtm->tm_mon + 1, swtm->tm_year + 1900);
	wakeupTime -= gmt_offset;  // get UTC

	calcSetHs8100_fpTime(wakeupTime, vData.u.standby.time);
	if (ioctl(context->fd, VFDSETPOWERONTIME, &vData) < 0)
	{
		perror("Set wake up time");
		return -1;
	}
	i = write_gmtoffset(gmt_offset);
	return i;
}

static int shutdown(Context_t *context, time_t *shutdownTimeGMT)
{  // -d command
	struct tm *shdn_gmt;
	time_t curTime;
	int gmt_offset;

	// shutdown immediately?
	if (*shutdownTimeGMT == -1)
    {
		system(cmdHalt);
	}
	// determine gmt_offset
	time(&curTime);
	shdn_gmt = gmtime(&curTime);
	gmt_offset = get_GMT_offset(*shdn_gmt);

	shdn_gmt = gmtime(shutdownTimeGMT);

	printf("Waiting until %02d:%02d:%02d %02d-%02d-%04d (local) to shut down.\n", shdn_gmt->tm_hour, shdn_gmt->tm_min,
		shdn_gmt->tm_sec, shdn_gmt->tm_mday, shdn_gmt->tm_mon + 1, shdn_gmt->tm_year + 1900);
	printf("Press Ctrl-C to interrupt.\n");
	while (1)
	{
		time(&curTime);

		if (curTime + gmt_offset >= *shutdownTimeGMT)
		{
			system(cmdHalt);
		}
		shdn_gmt = gmtime(&curTime);
		printf("\rTime: %02d:%02d:%02d %02d-%02d-%04d (local)", (shdn_gmt->tm_hour + (gmt_offset / 3600)) % 24 , (shdn_gmt->tm_min + (gmt_offset % 60)) % 60,
		 shdn_gmt->tm_sec, shdn_gmt->tm_mday, shdn_gmt->tm_mon + 1, shdn_gmt->tm_year + 1900);
		usleep(100000);
	}
	return -1;
}

static int reboot(Context_t *context, time_t *rebootTimeGMT)
{  // -r command, OK
	time_t curTime;

	while (1)
	{
		time(&curTime);
		if (curTime >= *rebootTimeGMT)
		{
			system(cmdReboot);
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
	int retval, len, i;
	struct tm *ts;
	char output[cMAXCharsHS8100 + 1];
	struct input_event data[64];
	tHS8100Private *private = (tHS8100Private *)((Model_t *)context->m)->private;

	printf("%s\n", __func__);

	output[cMAXCharsHS8100] = '\0';

	vFd = open("/dev/input/event0", O_RDONLY);
	if (vFd < 0) 
	{
		perror("event0");
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
				len = read(vFd, data, sizeof(struct input_event) * 64);

				for (i = 0; i < len / sizeof(struct input_event); i++)
				{
					if (data[i].type == EV_SYN) 
					{
						/* noop */
					}
					else if (data[i].type == EV_MSC
					&&       (data[i].code == MSC_RAW || data[i].code == MSC_SCAN))
					{
						/* noop */
					}
					else if (data[i].code == 116)
					{
						sleep = 0;
					}
				}
			}
		}
		if (private->display)
		{
			strftime(output, cMAXCharsHS8100 + 1, private->timeFormat, ts);
			write(private->vfd, &output, sizeof(output));
		}
	}
	return 0;
}
	
static int setFan(Context_t *context, int speed)
{ // -sf command, OK
	struct hchs8100_ioctl_data vData;

	vData.u.fan.speed = speed;
	setMode(context->fd);
	if (ioctl(context->fd, VFDSETFAN, &vData) < 0)
	{
		perror("setFan");
		return -1;
	}
	return 0;
}

static int setText(Context_t *context, char *theText)
{ // -t command, OK
	char vHelp[128];

	strncpy(vHelp, theText, 64);
	vHelp[64] = '\0';
	write(context->fd, vHelp, strlen(vHelp));
	return 0;
}
	
static int setIcon(Context_t *context, int which, int on)
{ // -i command, OK
	struct hchs8100_ioctl_data vData;

	if (which < 1 || which > cMAXIconsHS8100 + 2)
	{
		printf("Illegal icon number, range is 1 .. %d\n", cMAXIconsHS8100 + 2);
		return -1;
	}
	vData.u.icon.icon_nr = which;
	vData.u.icon.on = on;
	setMode(context->fd);
	if (ioctl(context->fd, VFDICONDISPLAYONOFF, &vData) < 0)
	{
		perror("Set icon");
		return -1;
	}
	return 0;}

static int setBrightness(Context_t *context, int brightness)
{  //-b command, OK
	struct hchs8100_ioctl_data vData;

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
{ // -c command, OK
//	int i;

	setText(context, "                ");
	setBrightness(context, 7);

//	for (i = 1; i <= cMAXIconsHS8100; i++)
//	{
//		setIcon(context, i, 0);
//	}
	return 0;
}

static int setLight(Context_t *context, int on)
{ // -L command, OK
	struct hchs8100_ioctl_data vData;

	vData.u.light.onoff = on;
	setMode(context->fd);
	if (ioctl(context->fd, VFDDISPLAYWRITEONOFF, &vData) < 0)
	{
		perror("Set light");
		return -1;
	}
	return 0;
}

static int getWakeupReason(Context_t *context, eWakeupReason *reason)
{ // -w command, OK
	int mode = -1;

	if (ioctl(context->fd, VFDGETWAKEUPMODE, &mode) < 0)
	{
		perror("Get wakeup reason");
		return -1;
	}
	if (mode != '\0')  // if we get a fp wake up reason
	{
		*reason = mode & 0xff;  // get LS byte
	}
	else
	{
		fprintf(stderr, "Error reading wakeup mode from frontprocessor\n");
		*reason = 0;  // echo unknown
	}
	return 0;
}

static int Exit(Context_t *context)
{
	tHS8100Private *private = (tHS8100Private *)((Model_t *)context->m)->private;

	if (context->fd > 0)
	{
		close(context->fd);
	}
	free(private);
	return 1;
}


Model_t Hchs8100_model =
{
	.Name             = "Homecast HS8100/9000 series frontpanel control utility",
	.Type             = Hchs8100,
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
	.SetLed           = NULL,
	.SetIcon          = setIcon,
	.SetBrightness    = setBrightness,
	.GetWakeupReason  = getWakeupReason,
	.SetLight         = setLight,
	.SetLedBrightness = NULL,
	.GetVersion       = NULL,
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
