#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifndef bool
#define bool unsigned char
#define true 1
#define false 0
#endif

#include "map.h"

#define INPUT_PRESS 1
#define INPUT_RELEASE 0

typedef enum
{
	Unknown,        //  0
	Ufs910_1W,      //  1
	Ufs910_14W,     //  2
	Ufs922,         //  3
	Ufc960,         //  4
	Tf7700,         //  5
	Hl101,          //  6
	Vip2,           //  7
	Fortis,         //  8
	Fortis_4G,      //  9
	Hs5101,         // 10
	Ufs912,         // 11
	Spark,          // 12
	Cuberevo,       // 13
	Adb_Box,        // 14
	Ipbox,          // 15
	CNBox,          // 16
	VitaminHD5000,  // 17
	LircdName       // 18
} eBoxType;

typedef enum
{
	RemoteControl,
	FrontPanel
} eKeyType;

typedef struct
{
	unsigned int delay;
	unsigned int period;
	unsigned int rc_code;
} tLongKeyPressSupport;

typedef struct RemoteControl_s
{
	char *Name;
	eBoxType Type;

	tButton *RemoteControl;
	tButton *Frontpanel;

	void *private;
	unsigned char supportsLongKeyPress;
	tLongKeyPressSupport *LongKeyPressSupport;
} RemoteControl_t;

typedef struct Context_s
{
//	void* *r;  // instance data
	RemoteControl_t *r;  // instance data
	int fd;  // filedescriptor of fd
} Context_t;

typedef struct BoxRoutines_s
{
	int (*Init)(Context_t *context, int argc, char *argv[]);
	int (*Shutdown)(Context_t *context);
	int (*Read)(Context_t *context);
	int (*Notification)(Context_t *context, const int on);
} BoxRoutines_t;

typedef struct Context_r_s
{
	BoxRoutines_t *br;  // box specific routines
} Context_r_t;

int getInternalCode(tButton *cButtons, const char cCode[3]);
int getInternalCodeHex(tButton *cButtons, const unsigned char cCode);
int getInternalCodeLircKeyName(tButton *cButtons, const char cCode[30]);
int printKeyMap(tButton *cButtons);
int checkTuxTxt(const int cCode);
void sendInputEvent(const int cCode);
void sendInputEventT(const unsigned int type, const int cCode);
int getEventDevice();
int selectRemote(Context_r_t *context_r, Context_t *context, eBoxType type);
void setInputEventRepeatRate(unsigned int delay, unsigned int period);
#endif
// vim:ts=4

