#ifndef __AM520_H
#define __AM520_H

struct set_light_s
{
	int onoff;
};

/* time must be given as follows:
 * time[0] = mjd high
 * time[1] = mjd low
 * time[2] = hour
 * time[3] = min
 * time[4] = sec
 */
struct set_standby_s
{
	char time[5];
};

struct set_time_s
{
	time_t localTime;
};

#if defined MODEL_SPECIFIC
struct modelspecific_s
{
	char data[19];  // the bytes to send, and returned
};
#endif

struct cnbox_ioctl_data
{
	union
	{
		struct set_standby_s standby;
		struct set_light_s light;
		struct set_time_s time;
#if defined MODEL_SPECIFIC
		struct modelspecific_s modelspecific;
#endif
	} u;
};

#endif  // __AM520_H
// vim:ts=4
