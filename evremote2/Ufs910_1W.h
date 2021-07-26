#ifndef _ufs910_1w_h_
#define _ufs910_1w_h_

#define VFDSETRCCODE 0xc0425af6
#define VFDSETLED    0xc0425afe
#define VFDSETMODE   0xc0425aff

#define LED_GREEN 2

/* This sets up the mode temporarily (for one ioctl)
 * to the desired mode. currently the "normal" mode
 * is the compatible vfd mode
 */
struct set_mode_s
{
	int compat; /* 0 = compatibility mode to vfd driver; 1 = micom mode */
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

struct set_led_s
{
	int led_nr;
	int on;
};

struct ufs910_fp_ioctl_data
{
	union
	{
		struct set_icon_s icon;
		struct set_led_s led;
		struct set_brightness_s brightness;
		struct set_mode_s mode;
	} u;
};

#endif  // _ufs910_1w_h_
// vim:ts=4
