#ifndef __AM520_H
#define __AM520_H

struct set_standby_s
{
	time_t localTime;
};

struct set_time_s
{
	time_t localTime;
};

struct cnbox_ioctl_data
{
	union
	{
		struct set_standby_s standby;
		struct set_time_s time;
	} u;
};

#endif  // __AM520_H
// vim:ts=4
