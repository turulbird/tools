/****************************************************************************
 *
 * eeprom.c
 *
 * (c) 2009 Dagobert@teamducktales
 *
 * Version for Kathrein UFS910 (24C16 EEPROM on I2C bus 0)
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

/*
 * Description:
 *
 * Startup of little EEPROM read/write utility. the EEPROM slots
 * are based on the very first original fw, so maybe they have
 * changed ...
 *
 * Audioniek: in the mean time it now has been tested and expanded.
 *
 ****************************************************************************
 *
 * Changes
 *
 * Date     By              Description
 * --------------------------------------------------------------------------
 * 20210702 Audioniek       Revitalized hexdump.
 * 20210702 Audioniek       Hexdump ASCII display added.
 * 20210703 Audioniek       Display by key debugged.
 * 20210703 Audioniek       Display of IP address format added.
 * 20210703 Audioniek       Display of Yes/No values added.
 * 20210703 Audioniek       Option -m (show MAC) added.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
//#include <linux/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define cMaxDetails  16

#define cTypeUnused   0
#define cTypeString   1
#define cTypeValue    2
#define cTypeValueYN  3
#define cTypeByte     4
#define cTypeIPaddr   5

/* FIXME: Eigentlich müßte ich das anders machen, es
 * gibt max 16 byte speicher insgesamt und die details
 * definieren nur virtuell was drinne ist und zwar je
 * nach größe ergibt sich die anzahl slot details.
 * Derzeit ist meine definition von cTypeUndefined
 * irreführend, denn wenn der erste 16 Byte hat sind zwar
 * alle anderen als undefined definiert sind aber durch den
 * ersten belegt.
 */

typedef struct
{
	char          *name;
	int           first_byte;
	int           number_bytes;

	int           type;

	unsigned char *buffer;
} slot_detail_t;

typedef struct
{
	unsigned char slot;
	unsigned int  i2c_bus;
	unsigned char i2c_addr;

	slot_detail_t details[cMaxDetails];
} slot_t;

#define CFG_EEPROM_ADDR  0x50
#define cMaxSlots 42

slot_t ufs922_slots[cMaxSlots] =
{
	/* i2c addr = 0x50 */
	{
		0x00, 0, 0x50,  // slot#, bus, i2c addr
		{
			{ "Magic",                   0, 8, cTypeString,  NULL },
			{ "Product",                 8, 8, cTypeByte,    NULL },
		}
	},
	{
		0x10, 0, 0x50,
		{
			{ "Slot_01_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_01_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x20, 0, 0x50,
		{
			{ "BootType",                0, 1, cTypeValue,   NULL },
			{ "DeepStandby",             1, 1, cTypeValue,   NULL },
			{ "RemoteAddr",              2, 1, cTypeValue,   NULL },
			{ "FirstInstallation",       3, 1, cTypeValueYN, NULL },
			{ "WakeupMode",              4, 1, cTypeValue,   NULL },
			{ "LogOption",               5, 1, cTypeValue,   NULL },
			{ "Slot_02_Empty01",         6, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty02",         7, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty03",         8, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty04",         9, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty05",        10, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty06",        11, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty07",        12, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty08",        13, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty09",        14, 1, cTypeUnused,  NULL },
			{ "Slot_02_Empty10",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x30, 0, 0x50,
		{
			{ "VideoOutput",             0, 1, cTypeValue,   NULL },
			{ "HDMIFormat",              1, 1, cTypeValue,   NULL },
			{ "Show_4_3",                2, 1, cTypeValue,   NULL },
			{ "AudioHDMI",               3, 1, cTypeValue,   NULL },
			{ "TVAspect",                4, 1, cTypeValue,   NULL },
			{ "Picture4_3",              5, 1, cTypeValue,   NULL },
			{ "Picture16_9",             6, 1, cTypeValue,   NULL },
			{ "ScartTV",                 7, 1, cTypeValue,   NULL },
			{ "ScartVCR",                8, 1, cTypeValue,   NULL },
			{ "AudioDolby",              9, 1, cTypeValue,   NULL },
			{ "TVSystem",               10, 1, cTypeValue,   NULL },
			{ "AudioLanguage",          11, 1, cTypeValue,   NULL },
			{ "SubtitleLanguage",       12, 1, cTypeValue,   NULL },
			{ "Slot_03_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_03_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_03_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x40, 0, 0x50,
		{
			{ "Tuner2Type",              0, 1, cTypeValue,   NULL },
			{ "Tuner2SignalConfig",      1, 1, cTypeValue,   NULL },
			{ "Tuner1Connection",        2, 1, cTypeValue,   NULL },
			{ "Tuner2Connection",        3, 1, cTypeValue,   NULL },
			{ "OneCableType",            4, 1, cTypeValue,   NULL },
			{ "Slot_04_Empty01",         5, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty02",         6, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty03",         7, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty04",         8, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty05",         9, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty06",        10, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty07",        11, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty08",        12, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty09",        13, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty10",        14, 1, cTypeUnused,  NULL },
			{ "Slot_04_Empty11",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x50, 0, 0x50,
		{
			{ "LocalTimeOffset",         0, 4, cTypeValue,   NULL },
			{ "WakeupTimer",             4, 4, cTypeValue,   NULL },
			{ "TimeSetupChannelNumber",  8, 4, cTypeValue,   NULL },
			{ "SummerTime",             12, 1, cTypeValueYN, NULL },
			{ "Slot_05_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_05_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_05_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x60, 0, 0x50,
		{
			{ "Slot_06_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_06_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x70, 0, 0x50,
		{
			{ "ForceDecrypt",            0, 1, cTypeValueYN, NULL },
			{ "ForceDecryptMode",        1, 1, cTypeValue,   NULL },
			{ "AlphacryptMultiDecrypt",  2, 1, cTypeValueYN, NULL },
			{ "Slot_07_Empty01",         3, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty02",         4, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty03",         5, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty04",         6, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty05",         7, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty06",         8, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty07",         9, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty08",        10, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty09",        11, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty10",        12, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty11",        13, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty12",        14, 1, cTypeUnused,  NULL },
			{ "Slot_07_Empty13",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x80, 0, 0x50,
		{
			{ "ChannelCRC",              0, 4, cTypeByte,    NULL },
			{ "SatelliteCRC",            4, 4, cTypeByte,    NULL },
			{ "ChannelCRC_HDD",          8, 4, cTypeByte,    NULL },
			{ "SatelliteCRC_HDD",       12, 4, cTypeByte,    NULL },
		}
	},
	{
		0x90, 0, 0x50,
		{
			{ "LastAudioVolume",         0, 1, cTypeValue,   NULL },
			{ "LastServiceType",         1, 1, cTypeValue,   NULL },
			{ "LastTVNumber",            2, 4, cTypeValue,   NULL },
			{ "LastRadioNumber",         6, 4, cTypeValue,   NULL },
			{ "Audio_Mute",             10, 1, cTypeValueYN, NULL },
			{ "Slot_09_Empty01",        11, 1, cTypeUnused,  NULL },
			{ "Slot_09_Empty02",        12, 1, cTypeUnused,  NULL },
			{ "Slot_09_Empty03",        13, 1, cTypeUnused,  NULL },
			{ "Slot_09_Empty04",        14, 1, cTypeUnused,  NULL },
			{ "Slot_09_Empty05",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xa0, 0, 0x50,
		{
			{ "lockMainMenu",            0, 1, cTypeValue,   NULL },
			{ "lockReceiver",            1, 1, cTypeValue,   NULL },
			{ "SleepTimer",              2, 1, cTypeValue,   NULL },
			{ "ChannelBannerDuration",   3, 1, cTypeValue,   NULL },
			{ "PlaybackBannerDuration",  4, 1, cTypeValue,   NULL },
			{ "DisplayVolume",           5, 1, cTypeValueYN, NULL },
			{ "FrontDisplayBrightness",  6, 1, cTypeValue,   NULL },
			{ "EPGGrabbing",             7, 1, cTypeValue,   NULL },
			{ "EPGStartView",            8, 1, cTypeValue,   NULL },
			{ "AutomaticTimeshift",      9, 1, cTypeValue,   NULL },
			{ "FunctionalRange",        10, 1, cTypeValue,   NULL },
			{ "AudioDelay",             11, 2, cTypeValue,   NULL },
			{ "Slot_0A_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_0A_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_0A_Empty02",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xb0, 0, 0x50,
		{
			{ "PreRecordtimes",          0, 2, cTypeValue,   NULL },
			{ "PostRecordtime",          2, 2, cTypeValue,   NULL },
			{ "PinCode",                 4, 4, cTypeByte,    NULL },
			{ "EPGGrabbingTime",         8, 4, cTypeValue,   NULL },
			{ "FreeAccessFrom",         12, 4, cTypeValue,   NULL },
		}
	},
	{
		0xc0, 0, 0x50,
		{
			{ "FreeAccessUntil",         0, 4, cTypeValue,   NULL },
			{ "DefaultRecordDuration",   4, 4, cTypeValue,   NULL },
			{ "FanControl",              8, 1, cTypeByte,    NULL },
			{ "AutomaticSWDownload",     9, 1, cTypeValueYN, NULL },
			{ "TimeshiftBufferSize",    10, 4, cTypeValue,   NULL },
			{ "Slot_0C_Empty01",        14, 1, cTypeUnused,  NULL },
			{ "Slot_0C_Empty02",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xd0, 0, 0x50,
		{
			{ "MyLongitude",             0, 4, cTypeValue,   NULL },
			{ "Latitude",                4, 4, cTypeValue,   NULL },
			{ "Slot_0D_Empty01",         8, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty02",         9, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty03",        10, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty04",        11, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty05",        12, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty06",        13, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty07",        14, 1, cTypeUnused,  NULL },
			{ "Slot_0D_Empty08",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xe0, 0, 0x50,
		{
			{ "Tuner1_SCR",              0, 4, cTypeValue,   NULL },
			{ "Tuner2_SCR",              4, 4, cTypeValue,   NULL },
			{ "Tuner1_SCR_Frequency",    8, 4, cTypeValue,   NULL },
			{ "Tuner2_SCR_Frequency",   12, 4, cTypeValue,   NULL },
		}
	},
	{
		0xf0, 0, 0x50,
		{
			{ "Use_OneCable",            0, 4, cTypeValue,   NULL },
			{ "MDU",                     4, 4, cTypeValue,   NULL },
			{ "Tuner1_PinCode",          8, 4, cTypeByte,    NULL },
			{ "Tuner2_PinCode",         12, 4, cTypeByte,    NULL },
		}
	},

	/* i2c addr = 0x51 */
	{
		0x00, 0, 0x51,
		{
			{ "System_Language",         0, 4, cTypeValue,   NULL },
			{ "Slot_10_Empty01",         4, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty02",         5, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty03",         6, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty04",         7, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty05",         8, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty06",         9, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty07",        10, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty08",        11, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty09",        12, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty10",        13, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty11",        14, 1, cTypeUnused,  NULL },
			{ "Slot_10_Empty12",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x10, 0, 0x51,
		{
			{ "ChannelListYear",         0, 2, cTypeValue,   NULL },
			{ "ChannelListMonth",        2, 1, cTypeValue,   NULL },
			{ "ChannelListDay",          3, 1, cTypeValue,   NULL },
			{ "FirmwareYear",            4, 2, cTypeValue,   NULL },
			{ "FirmwareMonth",           6, 1, cTypeValue,   NULL },
			{ "FirmwareDay",             7, 1, cTypeValue,   NULL },
			{ "Slot_11_Empty01",         8, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty02",         9, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty03",        10, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty14",        11, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty15",        12, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty16",        13, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty17",        14, 1, cTypeUnused,  NULL },
			{ "Slot_11_Empty18",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x20, 0, 0x51,
		{
			{ "IpAddress",               0, 4, cTypeIPaddr,  NULL },
			{ "SubNet",                  4, 4, cTypeIPaddr,  NULL },
			{ "Gateway",                 8, 4, cTypeIPaddr,  NULL },
			{ "EEProm_DNS",             12, 4, cTypeIPaddr,  NULL },
		}
	},
	{
		0x30, 0, 0x51,
		{
			{ "DHCP",                    0, 1, cTypeValueYN, NULL },
			{ "TimeMode",                1, 1, cTypeByte,    NULL },
			{ "SerialKbd",               2, 1, cTypeByte,    NULL },
			{ "Slot_13_Empty01",         3, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty02",         4, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty03",         5, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty04",         6, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty05",         7, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty06",         8, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty07",         9, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty08",        10, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty09",        11, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty10",        12, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty11",        13, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty12",        14, 1, cTypeUnused,  NULL },
			{ "Slot_13_Empty13",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x40, 0, 0x51,
		{
			{ "Playback_Shuffle",        0, 1, cTypeValueYN, NULL },
			{ "Playback_Repeat",         1, 1, cTypeValueYN, NULL },
			{ "Playback_Duration",       2, 1, cTypeByte,    NULL },
			{ "Slot_14_Empty01",         3, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty02",         4, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty03",         5, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty04",         6, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty05",         7, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty06",         8, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty07",         9, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty08",        10, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty09",        11, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty10",        12, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty11",        13, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty12",        14, 1, cTypeUnused,  NULL },
			{ "Slot_14_Empty13",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x50, 0, 0x51,
		{
			{ "FTP",                     0, 1, cTypeValueYN, NULL },
			{ "4G_Limit",                1, 1, cTypeByte,    NULL },
			{ "UPnP",                    2, 1, cTypeValueYN, NULL },
			{ "Slot_15_Empty01",         3, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty02",         4, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty03",         5, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty04",         6, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty05",         7, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty06",         8, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty07",         9, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty08",        10, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty09",        11, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty10",        12, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty11",        13, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty12",        14, 1, cTypeUnused,  NULL },
			{ "Slot_15_Empty13",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x60, 0, 0x51,
		{
			{ "Slot_16_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_16_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x70, 0, 0x51,
		{
			{ "Slot_17_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_17_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x80, 0, 0x51,
		{
			{ "Slot_18_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_18_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x90, 0, 0x51,
		{
			{ "Slot_19_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_19_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xa0, 0, 0x51,
		{
			{ "Slot_1A_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_1A_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xb0, 0, 0x51,
		{
			{ "Slot_1B_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_1B_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xc0, 0, 0x51,
		{
			{ "Slot_1C_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_1C_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xd0, 0, 0x51,
		{
			{ "Slot_1D_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_1D_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0xe0, 0, 0x51,
		{
			{ "Slot_1E_Empty01",         0, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty02",         1, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty03",         2, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty04",         3, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty05",         4, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty06",         5, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty07",         6, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty08",         7, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty09",         8, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty10",         9, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty11",        10, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty12",        11, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty13",        12, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty14",        13, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty15",        14, 1, cTypeUnused,  NULL },
			{ "Slot_1E_Empty16",        15, 1, cTypeUnused,  NULL },
		}
	},
	/* i2c addr = 0x52 unused */

	/* i2c addr = 0x53 */
	{
		0xf0, 0, 0x53,
		{
			{ "TestFPGA",                0, 1, cTypeValueYN, NULL },
			{ "TestFPGAMode",            1, 1, cTypeByte,    NULL },
			{ "ChannelChangeMode",       2, 1, cTypeByte,    NULL },
			{ "ConaxParingMode",         3, 1, cTypeByte,    NULL },
			{ "Slot_3F_Empty01",         4, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty02",         5, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty03",         6, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty04",         7, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty05",         8, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty06",         9, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty07",        10, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty08",        11, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty09",        12, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty10",        13, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty11",        14, 1, cTypeUnused,  NULL },
			{ "Slot_3F_Empty12",        15, 1, cTypeUnused,  NULL },
		}
	},

	/* i2c addr = 0x54 - 56: boot loader enviroonment */

	/* i2c addr = 0x57 */
	{
		0x00, 0, 0x57,
		{
			{ "UpdateFlag",              0, 1, cTypeValueYN, NULL },
			{ "Slot_70_Empty01",         1, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty02",         2, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty03",         3, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty04",         4, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty05",         5, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty06",         6, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty07",         7, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty08",         8, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty19",         9, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty10",        10, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty11",        11, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty12",        12, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty13",        13, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty14",        14, 1, cTypeUnused,  NULL },
			{ "Slot_70_Empty15",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x10, 0, 0x57,
		{
			{ "UpdateUnit1UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit1FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit1DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit1CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_71_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_71_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_71_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x20, 0, 0x57,
		{
			{ "UpdateUnit2UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit2FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit2DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit2CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_72_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_72_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_72_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x30, 0, 0x57,
		{
			{ "UpdateUnit3UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit3FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit3DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit3CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_73_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_73_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_73_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x40, 0, 0x57,
		{
			{ "UpdateUnit4UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit4FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit4DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit4CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_74_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_74_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_74_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x50, 0, 0x57,
		{
			{ "UpdateUnit5UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit5FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit5DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit5CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_75_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_75_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_75_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x60, 0, 0x57,
		{
			{ "UpdateUnit6UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit6FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit6DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit6CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_76_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_76_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_76_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x70, 0, 0x57,
		{
			{ "UpdateUnit7UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit7FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit7DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit7CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_77_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_77_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_77_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x80, 0, 0x57,
		{
			{ "UpdateUnit8UpdateFlag",   0, 1, cTypeByte,    NULL },
			{ "UpdateUnit8FlashOffset",  4, 4, cTypeByte,    NULL },
			{ "UpdateUnit8DataLength",   8, 4, cTypeValue,   NULL },
			{ "UpdateUnit8CRC",         12, 4, cTypeValue,   NULL },
			{ "Slot_78_Empty01",        13, 1, cTypeUnused,  NULL },
			{ "Slot_78_Empty02",        14, 1, cTypeUnused,  NULL },
			{ "Slot_78_Empty03",        15, 1, cTypeUnused,  NULL },
		}
	},
	{
		0x90, 0, 0x57,
		{
			{ "SWPackVersion",           0, 4, cTypeValue,   NULL },
			{ "ChannelDeltaVersion",     4, 4, cTypeValue,   NULL },
			{ "Slot_79_Empty01",         8, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty02",         9, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty03",        10, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty04",        11, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty05",        12, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty06",        13, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty07",        14, 1, cTypeUnused,  NULL },
			{ "Slot_79_Empty08",        15, 1, cTypeUnused,  NULL },
		}
	},
};

/*
 * I2C Message - used for pure i2c transaction, also from /dev interface
 */
struct i2c_msg
{
	unsigned short addr;  /* slave address       */
	unsigned short flags;
	unsigned short len;   /* msg length          */
	unsigned char  *buf;  /* pointer to msg data */
};

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data
{
	struct i2c_msg *msgs;  /* pointers to i2c_msgs */
	unsigned int nmsgs;    /* number of i2c_msgs   */
};

#define I2C_SLAVE       0x0703  /* Change slave address */
		/* Attn.: Slave address is 7 or 10 bits */

#define I2C_SLAVE_FORCE	0x0706  /* Change slave address */
#define I2C_RDWR        0x0707  /* Combined R/W transfer (one stop only)*/

int i2c_slave_force(int fd, char addr, char c1, char c2)
{
	char buf[256];

	if (ioctl(fd, I2C_SLAVE_FORCE, addr) != 0)
	{
		printf("i2c failed 1\n");
		return -1;
	}

	buf[0] = c1;
	buf[1] = c2;
	if (write(fd, buf, 2) != 2)
	{
		return -2;
	}
	return 0;
}

int i2c_slave_force_var(int fd, char addr, unsigned char *buffer, int len)
{
//	printf("%s: > addr = 0x%02x, len = %d, %d bytes\n", __func__, addr, *buffer, len);

	if (ioctl(fd, I2C_SLAVE_FORCE, addr) != 0)
	{
		printf("%s: i2c failed\n", __func__);
		return -1;
	}

//	printf("%s: buffer 0x%x\n", __func__, buffer[0]);
	if (write(fd, buffer, len) != len)
	{
		return -2;
	}
	return 0;
}

int i2c_readreg(int fd_i2c, char addr, unsigned char *buffer, int len)
{
	struct i2c_rdwr_ioctl_data i2c_rdwr;
	int	err;

//	printf("%s: > addr = 0x%02x, len = %d bytes\n", __func__, addr, len);

	/* */
	i2c_rdwr.nmsgs = 1;
	i2c_rdwr.msgs = malloc(2 * 32);
	i2c_rdwr.msgs[0].addr = addr;
	i2c_rdwr.msgs[0].flags = 1;
	i2c_rdwr.msgs[0].len = len;
	i2c_rdwr.msgs[0].buf = malloc(len);

	memcpy(i2c_rdwr.msgs[0].buf, buffer, len);

	if ((err = ioctl(fd_i2c, I2C_RDWR, &i2c_rdwr)) < 0)
	{
		printf("%s: failed %d %d\n", __func__, err, errno);
		printf("addr = 0x%2x, len = %d bytes\n", addr, len);
		printf("%s\n", strerror(errno));
		free(i2c_rdwr.msgs[0].buf);
		free(i2c_rdwr.msgs);
		return -1;
	}
	memcpy(buffer, i2c_rdwr.msgs[0].buf, len);
	free(i2c_rdwr.msgs[0].buf);
	free(i2c_rdwr.msgs);
	return 0;
}

char *set_text_length(char *text, int length)
{
	int orig_len;
	char *spc_text = "                                        ";
	char *tmp_text = malloc(256);
	char *new_text = malloc(256);

	orig_len = strlen(text);
	if (length < orig_len
	||  length > 40)
	{
		return text;
	}
	strncpy(new_text, text, orig_len);
	strncpy(tmp_text, spc_text, length - orig_len);
	strcat(new_text, tmp_text);
	return new_text;
}

void handle_bootargs(int mac)
{
	int fd_i2c;
	int vLoopAddr;
	unsigned char reg = 0;
	int startaddr = 4;  // skip CRC
	unsigned char buffer[3 * 256] = { 0 };
	char name[48] = { 0 };
	int namelength;
	char arg[256] = { 0 };
	int arglength;
	int argcount = 0;
//	int crcargs = 0;

	fd_i2c = open("/dev/i2c-0", O_RDWR);

	for (vLoopAddr = 4; vLoopAddr <= 6; vLoopAddr++)
	{
		i2c_slave_force_var(fd_i2c, CFG_EEPROM_ADDR + vLoopAddr, &reg, 1);
		i2c_readreg(fd_i2c, CFG_EEPROM_ADDR + vLoopAddr, buffer + ((vLoopAddr - 4) * 256), 256);
	}

	argcount = 0;
	do
	{
		namelength = 0;
		do
		{
			name[namelength] = buffer[startaddr];
			startaddr++;
			namelength++;
		}
		while (buffer[startaddr] != 0x3d);
		name[namelength] = 0;  // terminate name
		startaddr++;  // skip =

		arglength = 0;
		do
		{
			arg[arglength] = buffer[startaddr];
			startaddr++;
			arglength++;
		}
		while (buffer[startaddr] != 0);
		arg[arglength] = 0; // terminate arg

		if (!mac)
		{
			printf("%s '%s'\n", name, arg);
		}
		else
		{
			if (strcmp((const char *)name, "ethaddr") == 0)
			{
				if (arg[strlen(arg) - 1] == 0x0a)
				{
					arg[strlen(arg) - 1] = 0;
				}
				printf("%s\n", arg);
			}
		}
		argcount++;
		startaddr++;  // skip null byte
	}
	while (buffer[startaddr] != 0);

	close(fd_i2c);

	if (!mac)
	{
//		crcargs = crc32(0, *(buffer + 4), startaddr - 4);
		printf("\n bootargs CRC = 0x%02x%02x%02x%02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		printf("\n %d of 768 bytes used\n", startaddr);
		printf("\n %d bootargs\n", argcount);
	}
}

int main(int argc, char *argv[])
{
	int fd_i2c;
	char device[256];
	int vLoopAddr, vLoopSlot = 0, vLoop;

	if (argc == 2 ) // 1 argument given
	{
		if ((strcmp(argv[1], "-d") == 0) || (strcmp(argv[1], "--dump") == 0))
		{
			sprintf(device, "/dev/i2c-%d", ufs922_slots[vLoopSlot].i2c_bus);  // add i2c bus number to device name
			fd_i2c = open(device, O_RDWR);

			printf("EEPROM dump\n");
			printf("Addr  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F   ASCII\n");
			printf("-----------------------------------------------------------------------\n");

			for (vLoopAddr = CFG_EEPROM_ADDR; vLoopAddr <= CFG_EEPROM_ADDR + 7; vLoopAddr++)
			{
				for (vLoopSlot = 0x00; vLoopSlot <= 0x0f; vLoopSlot++)
				{
					unsigned char reg = vLoopSlot * 0x10;
					unsigned char buffer[16] =
					{ 0x18, 0x74, 0x88, 0x7b, 0x94, 0x77, 0xb4, 0x0, 0x2c, 0x3a, 0xb4, 0x0, 0x1, 0x0, 0x0, 0x0 };

					i2c_slave_force_var(fd_i2c, vLoopAddr, &reg, 1);

					i2c_readreg(fd_i2c, vLoopAddr, buffer, 16);

					printf("%1x%02x  ", vLoopAddr & 0x07, reg);
					for (vLoop = 0; vLoop < 16; vLoop++)
					{
						printf("%02x ", buffer[vLoop]);
					}
					// ASCII display
					printf("  ");
					for (vLoop = 0; vLoop < 16; vLoop++)
					{
						if (buffer[vLoop] >= 0x20 && buffer[vLoop] < 0x7f)
						{
							printf("%c", buffer[vLoop]);
						}
						else
						{
							printf(".");
						}
					}
					printf("\n");
				}
			}
			close(fd_i2c);
		}
		else if ((strcmp(argv[1], "-k") == 0) || (strcmp(argv[1], "--key") == 0))
		{
			int vLoopSlot, vLoop;

			for (vLoopSlot = 0; vLoopSlot < cMaxSlots; vLoopSlot++)  // counts items in ufs922_slots
			{
				char device[256];
				unsigned char buffer[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
				int bytes_processed = 0;
				int i;
				char tmpbuf[16] = { 0 };
				char *pname;

				sprintf(device, "/dev/i2c-%d", ufs922_slots[vLoopSlot].i2c_bus);  // add i2c bus number to device name

				fd_i2c = open(device, O_RDWR);  // open i2c device

				if (fd_i2c < 0)
				{
					printf("[eeprom] Error opening %s\n", device);
					return 1;
				}
				i2c_slave_force_var(fd_i2c, ufs922_slots[vLoopSlot].i2c_addr, &ufs922_slots[vLoopSlot].slot, 1);

				i2c_readreg(fd_i2c, ufs922_slots[vLoopSlot].i2c_addr, buffer, 16);  // read one slot

				vLoop = 0;
				do
				{
					// normalize name length
					pname = set_text_length(ufs922_slots[vLoopSlot].details[vLoop].name, 24);

					if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeString)
					{
						for (i = 0; i < ufs922_slots[vLoopSlot].details[vLoop].number_bytes; i++)
						{
							tmpbuf[i] = buffer[i + ufs922_slots[vLoopSlot].details[vLoop].first_byte];
						}
						tmpbuf[ufs922_slots[vLoopSlot].details[vLoop].number_bytes] = 0;  // terminate string


						printf("Name = %s [%2d:%2d] value = %s\n",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes,
						       tmpbuf);
					}
					else if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeValue)
					{
						unsigned long value;
						int valueLoop;

						ufs922_slots[vLoopSlot].details[vLoop].buffer = malloc(ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						memcpy(ufs922_slots[vLoopSlot].details[vLoop].buffer,
						       buffer + ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						value = 0;

						for (valueLoop = 0; valueLoop < ufs922_slots[vLoopSlot].details[vLoop].number_bytes; valueLoop++)
						{
							value |= ufs922_slots[vLoopSlot].details[vLoop].buffer[valueLoop] <<
								 ((ufs922_slots[vLoopSlot].details[vLoop].number_bytes - valueLoop - 1) * 8);
							/*
							printf("value = %d\n", value);
							printf("buffer = 0x%x shift = %d\n", ufs922_slots[vLoopSlot].details[vLoop].buffer[valueLoop],((ufs922_slots[vLoopSlot].details[vLoop].number_bytes - valueLoop - 1) * 8));
							*/
						}

						printf("Name = %s [%2d:%2d] value = %d (0x%x)\n",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes,
						       (int)value,
						       (int)value);
					}
					else if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeValueYN)
					{
						unsigned long value;
						int valueLoop;

						ufs922_slots[vLoopSlot].details[vLoop].buffer = malloc(ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						memcpy(ufs922_slots[vLoopSlot].details[vLoop].buffer,
						       buffer + ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						value = 0;

						for (valueLoop = 0; valueLoop < ufs922_slots[vLoopSlot].details[vLoop].number_bytes; valueLoop++)
						{
							value |= ufs922_slots[vLoopSlot].details[vLoop].buffer[valueLoop] <<
								 ((ufs922_slots[vLoopSlot].details[vLoop].number_bytes - valueLoop - 1) * 8);
							/*
							printf("value = %d\n", value);
							printf("buffer = 0x%x shift = %d\n", ufs922_slots[vLoopSlot].details[vLoop].buffer[valueLoop],((ufs922_slots[vLoopSlot].details[vLoop].number_bytes - valueLoop - 1) * 8));
							*/
						}

						printf("Name = %s [%2d:%2d] value = %d (%s)\n",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes,
						       (int)value,
						       (value == 0 ? "No" : "Yes"));
					}
					else if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeByte)
					{
						int valueLoop;

						printf("Name = %s [%2d:%2d] value = ",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						for (valueLoop = 0; valueLoop < ufs922_slots[vLoopSlot].details[vLoop].number_bytes; valueLoop++)
						{
							printf("0x%02x ", buffer[valueLoop + bytes_processed]);
						}
						printf("\n");
					}
					else if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeIPaddr)
					{
						int valueLoop;

						printf("Name = %s [%2d:%2d] value = ",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes);

						for (valueLoop = 0; valueLoop < ufs922_slots[vLoopSlot].details[vLoop].number_bytes; valueLoop++)
						{
							printf("0x%02x ", buffer[valueLoop + bytes_processed]);
						}
						printf("(");
						for (valueLoop = ufs922_slots[vLoopSlot].details[vLoop].number_bytes - 1; valueLoop >= 0; valueLoop--)
						{
							printf("%03d", buffer[valueLoop + bytes_processed]);
							if (valueLoop != 0)
							{
								printf(":");
							}
						}
						printf(")\n");
					}
					else if (ufs922_slots[vLoopSlot].details[vLoop].type == cTypeUnused)
					{
#if 0
						printf("Name = %s [%2d:%2d] Unused\n",
						       pname,
						       ufs922_slots[vLoopSlot].details[vLoop].first_byte,
						       ufs922_slots[vLoopSlot].details[vLoop].number_bytes);
#endif
					}
					else
					{
						printf("Illegal value for type detected!\n");
						ufs922_slots[vLoopSlot].details[vLoop].number_bytes = 16;
					}
					bytes_processed += ufs922_slots[vLoopSlot].details[vLoop].number_bytes;
					vLoop++;
				}
				while (bytes_processed < 16);
			}
			close(fd_i2c);
		}
		else if ((strcmp(argv[1], "-b") == 0) || (strcmp(argv[1], "--bootargs") == 0))
		{
			handle_bootargs(0);
		}
		else if ((strcmp(argv[1], "-m") == 0) || (strcmp(argv[1], "--mac") == 0))
		{
			handle_bootargs(1);
		}
	}
#if 0
	else if (argc == 3 ) // 2 arguments given
	{
		if ((strcmp(argv[1], "-k") == 0) || (strcmp(argv[1], "--key") == 0))
		{
			printf("Display value of key %s\n", argv[2]);
		}
	}
#endif
	else
	{ // no arguments; show usage
		printf("%s version 1.0\n\n", argv[0]);
		printf(" Usage:\n");
		printf("%s [ -d | --dump | -k | --key | -b | --bootargs | -m | --mac ]\n\n", argv[0]);
		printf(" no args         : show this usage\n");
		printf(" -d | --dump     : hex dump EEPROM contents\n");
		printf(" -k | --key      : show EEPROM contents by key\n");
//		printf(" -k keyname  | --key keyname : show value of key keyname\n");
		printf(" -b | --bootargs : show bootloader environment\n");
		printf(" -m | --mac      : show MAC address\n");
		printf("\n CAUTION: keynames and key layout may not be valid\n");
		printf("          valid for factory firmware other than V1.04.\n");
	}
	return 0;
}
// vim:ts=4
