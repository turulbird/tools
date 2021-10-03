#ifndef __Opt9600_H
#define __Opt9600_H

/* time must be given as follows:
 * time[0] & time[1] = MJD
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

struct set_light_s
{
	int onoff;
};

struct set_test_s
{
	unsigned char data[12];
};

struct opt9600_fp_ioctl_data
{
	union
	{
		struct set_standby_s standby;
		struct set_time_s time;
		struct set_light_s light;
		struct set_test_s test;
	} u;
};
#endif  // __Opt9600_H
// vim:ts=4
