#ifndef _fortis_4g_h_
#define _fortis_4g_h_

#define VFDICONDISPLAYONOFF 0xc0425a0a
#define VFDSETLED           0xc0425afe
#define VFDSETMODE          0xc0425aff
#define ICON_DOT            1
#define LED_GREEN           1

struct set_icon_s
{
	int icon_nr;
	int on;
};

struct set_led_s
{
	int led_nr;
	int level;  // level was chosen here because some Fortis models can dim their LEDs
};

struct set_brightness_s
{
	int level;
};

struct set_light_s
{
	int onoff;
};

struct get_version_s
{
	unsigned int data[2]; // data[0] = boot loader version, data[1] = resellerID
};

/* this sets up the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = nuvoton mode */
};

/* time must be given as follows:
 * time[0] & time[1] = mjd ???
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

struct set_timeformat_s
{
	char format;
};

struct set_timedisplay_s
{
	char onoff;
};

struct fortis_4g_ioctl_data
{
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_light_s light;
		struct set_mode_s mode;
		struct set_standby_s standby;
		struct set_time_s time;
		struct set_timeformat_s timeformat;
		struct set_timedisplay_s timedisplay;
		struct get_version_s version;
	} u;
};

struct vfd_ioctl_data
{
	unsigned char start_address;
	unsigned char data[64];
	unsigned char length;
};
#endif  // _fortis_4g_h_
