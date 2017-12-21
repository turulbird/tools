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
	LircdName
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
} tLongKeyPressSupport;

typedef struct RemoteControl_s
{
	char *Name;
	eBoxType Type;

	tButton *RemoteControl;
	tButton *Frontpanel;
	void *private;
	unsigned char supportsLongKeyPress;
//	tLongKeyPressSupport *LongKeyPressSupport;
} RemoteControl_t;

typedef struct Context_s
{
	RemoteControl_t *r;  // instance data
	int fd;  // filedescriptor of fd
} Context_t;

typedef struct BoxRoutines_s
{
	int (*Init)(Context_t *context, int argc, char *argv[]);
	int (*Shutdown)(Context_t *context);
	int (*Read)(Context_t *context);
} BoxRoutines_t;

typedef struct Context_r_s
{
	BoxRoutines_t *br;  // box specific routines
} Context_r_t;

int getInternalCodeLircKeyName(tButton *cButtons, const char cCode[30]);
int printKeyMap(tButton *cButtons);
int checkTuxTxt(const int cCode);
int getEventDevice();
int selectRemote(Context_t *context, eBoxType type);

#endif
// vim:ts=4

