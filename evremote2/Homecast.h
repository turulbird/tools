#ifndef __homecast__
#define __homecast__

#define VFDDISPLAYCHARS     0xc0425a00
#define VFDICONDISPLAYONOFF 0xc0425a0a
#define VFDSETMODE          0xc0425aff
#define VFDSETREMOTEKEY     0xc0425b05
#define	ICON_CIRCLE_0       1

/* This will set the mode temporarily (for one ioctl)
 * to the desired mode. Currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = homecast mode */
};

struct set_brightness_s
{
	int level;
};

struct set_icon_s
{
	int icon_nr;
	int on;
};

struct set_light_s
{
	int onoff;
};

struct set_fan_s
{
	int speed;
};

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
	char time[5];
};

struct set_remote_s
{
	char key;
};

struct homecast_fp_ioctl_data
{
	union
	{
		struct set_icon_s icon;
		struct set_brightness_s brightness;
		struct set_light_s light;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
		struct set_fan_s fan;
		struct set_remote_s remote_key;
	} u;
};

struct vfd_ioctl_data
{
	unsigned char address;
	unsigned char data[64];
	unsigned char length;
};

#endif  // __homecast__
// vim:ts=4

