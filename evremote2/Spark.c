/*
 * Spark.c
 *
 * (c) 2010 duckbox project
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include "global.h"
#include "map.h"
#include "remotes.h"
#include "Spark.h"

#define SPARK_RC04_PREDATA      0xCC33
#define SPARK_RC05_PREDATA      0x11EE  // also Edision VIP RC2
#define SPARK_RC08_PREDATA      0x44BB
#define SPARK_RC09_PREDATA      0x9966  // also Edision VIP RC1
#define SPARK_RC12_PREDATA      0x08F7
#define SPARK_DEFAULT_PREDATA   0xA25D  // HOF-12, SAB Unix Triple HD, Sogno Triple
#define SPARK_EDV_RC1           0x1CE3  // Edision argus pingulux RC1
#define SPARK_EDV_RC2           0xC43B  // Edision argus pingulux RC2
#define UFS910_RC660_PREDATA    0x2290  // matches UFS910 lirc conf
#define UFS913_RC230_PREDATA    0x7FB9
#define SAMSUNG_AA59_PREDATA    0xE0E0

#define STB_ID_GOLDENMEDIA_GM990        "09:00:07"
#define STB_ID_EDISION_PINGULUX         "09:00:08"  // Edision argus pingulux, pingulux plus & pingulux mini
#define STB_ID_AMIKO_ALIEN_SDH8900      "09:00:0A"
#define STB_ID_GALAXYINNOVATIONS_S8120  "09:00:0B"
#define STB_ID_SOGNO_TRIPLE_HD          "0C:00:43"  // Sogno Spark Triple
#define STB_ID_SAB_UNIX_TRIPLE_HD       "0C:00:0E"  // SAB Unix Triple HD (S903)
#define STB_ID_GOLDENMEDIA_SPARK_TRIPLE "0C:00:07"  // Golden Media Spark Triple

char VendorStbId[] = "00:00:00\0";

static tLongKeyPressSupport cLongKeyPressSupport =
{
	10, 120
};

/* Edision argus-spark RCU (HOF13), used for RC1 and RC2 */
static tButton cButtonsEdisionSpark[] =
{
	{ "POWER",       "25", KEY_POWER },
	{ "RC12",        "a5", KEY_R },
	{ "V.FORMAT",    "ad", KEY_SWITCHVIDEOMODE },
	{ "TIMER",       "8d", KEY_PROGRAM },
	{ "MUTE",        "85", KEY_MUTE },
	{ "Tms",         "e5", KEY_TIME },
	{ "PIP",         "ed", KEY_SCREEN },
	{ "F1",          "cd", KEY_F1 },
	{ "TV/SAT",      "c5", KEY_AUX },
	{ "1",           "b5", KEY_1 },
	{ "2",           "95", KEY_2 },
	{ "3",           "bd", KEY_3 },
	{ "4",           "f5", KEY_4 },
	{ "5",           "d5", KEY_5 },
	{ "6",           "fd", KEY_6 },
	{ "7",           "35", KEY_7 },
	{ "8",           "15", KEY_8 },
	{ "9",           "3d", KEY_9 },
	{ "TV/RADIO",    "77", KEY_TV2 },
	{ "0",           "57", KEY_0 },
	{ "RECALL",      "7f", KEY_BACK },
	{ "FIND",        "9d", KEY_SUBTITLE },
	{ "RECORD",      "45", KEY_RECORD },
	{ "VOL+",        "c7", KEY_VOLUMEUP },
	{ "VOL-",        "dd", KEY_VOLUMEDOWN },
	{ "PAGE+",       "07", KEY_CHANNELUP },
	{ "PAGE-",       "5f", KEY_CHANNELDOWN },
	{ "SAT",         "1d", KEY_SAT },
	{ "FAV",         "87", KEY_FAVORITES },
	{ "MENU",        "65", KEY_MENU },
	{ "INFO",        "a7", KEY_INFO },
	{ "UP",          "27", KEY_UP },
	{ "LEFT",        "6d", KEY_LEFT },
	{ "OK/LIST",     "2f", KEY_OK },
	{ "RIGHT",       "af", KEY_RIGHT },
	{ "DOWN",        "0f", KEY_DOWN },
	{ "EXIT",        "4d", KEY_EXIT },
	{ "EDV",         "8f", KEY_EPG },
	{ "FOLDER",      "75", KEY_FILE },
	{ "STOP",        "f7", KEY_STOP },
	{ "PAUSE",       "37", KEY_PAUSE },
	{ "PLAY",        "b7", KEY_PLAY },
	{ "PREVIOUS",    "55", KEY_PREVIOUS },
	{ "NEXT",        "d7", KEY_NEXT },
	{ "REWIND",      "1f", KEY_REWIND },
	{ "FASTFORWARD", "97", KEY_FASTFORWARD },
	{ "STEP_BACK",   "5d", KEY_SLOW },
	{ "STEP_FWD",    "df", KEY_F },
	{ "PLAYMODE",    "1f", KEY_W },
	{ "USB",         "95", KEY_MEDIA },
	{ "RED",         "7d", KEY_RED },
	{ "GREEN",       "ff", KEY_GREEN },
	{ "YELLOW",      "3f", KEY_YELLOW },
	{ "BLUE",        "bf", KEY_BLUE },
	{ "AUDIO",       "35", KEY_AUDIO },
	{ "MARK",        "8f", KEY_AUX },
	{ "",            "",   KEY_NULL }
};

/* spark RC8 */
static tButton cButtonsSparkRc08[] =
{
	{ "POWER",       "4D", KEY_POWER },
	{ "MUTE",        "DD", KEY_MUTE },
	{ "TIME",        "ED", KEY_PROGRAM },
	{ "V.FORMAT",    "AD", KEY_SWITCHVIDEOMODE },
	{ "F1",          "0F", KEY_F1 },
	{ "TV/SAT",      "37", KEY_AUX },  // !
	{ "USB",         "0D", KEY_MEDIA },
	{ "FIND",        "35", KEY_SUBTITLE },
	{ "0",           "27", KEY_0 },
	{ "1",           "7D", KEY_1 },
	{ "2",           "3F", KEY_2 },
	{ "3",           "BD", KEY_3 },
	{ "4",           "5D", KEY_4 },
	{ "5",           "1F", KEY_5 },
	{ "6",           "9D", KEY_6 },
	{ "7",           "55", KEY_7 },
	{ "8",           "17", KEY_8 },
	{ "9",           "95", KEY_9 },
	{ "TV/RADIO",    "65", KEY_TV2 },
	{ "RECALL",      "A5", KEY_BACK },
	{ "MENU",        "5F", KEY_MENU },
	{ "INFO",        "1D", KEY_INFO },
	{ "UP",          "9F", KEY_UP },
	{ "DOWN",        "AF", KEY_DOWN },
	{ "LEFT",        "3D", KEY_LEFT },
	{ "RIGHT",       "7F", KEY_RIGHT },
	{ "OK",          "BF", KEY_OK },
	{ "EXIT",        "2D", KEY_EXIT },
	{ "EPG",         "25", KEY_EPG },
	{ "VOL+",        "8D", KEY_VOLUMEUP },
	{ "VOL-",        "B5", KEY_VOLUMEDOWN },
	{ "REC",         "EF", KEY_RECORD },
	{ "PAGE+",       "B7", KEY_CHANNELUP },
	{ "PAGE-",       "77", KEY_CHANNELDOWN },
	{ "FOLDER",      "E5", KEY_FILE },
	{ "STOP",        "A7", KEY_STOP },
	{ "PLAY",        "75", KEY_PLAY },
	{ "PAUSE",       "F5", KEY_PAUSE },
	{ "FASTFORWARD", "CD", KEY_FASTFORWARD },
	{ "REWIND",      "D5", KEY_REWIND },
	{ "PREV",        "8F", KEY_PREVIOUS },
	{ "NEXT",        "57", KEY_NEXT },
	{ "Tms",         "4F", KEY_TIME },
	{ "FAST",        "97", KEY_F },
	{ "SLOW",        "15", KEY_SLOW },
	{ "PLAY_MODE",   "6F", KEY_P },
	{ "WHITE",       "67", KEY_W },
	{ "RED",         "05", KEY_RED },
	{ "GREEN",       "87", KEY_GREEN },
	{ "YELLOW",      "C5", KEY_YELLOW },
	{ "BLUE",        "47", KEY_BLUE },
	{ "FAV",         "F7", KEY_FAVORITES },
	{ "SAT",         "2F", KEY_SAT },
	{ "TTX",         "DF", KEY_TEXT },
	{ "AUDIO",       "D7", KEY_SUBTITLE },
	{ "",            "",   KEY_NULL }
};

/* Amiko alien-spark RCU */
static tButton cButtonsSparkRc09[] =
{
	{ "POWER",       "25", KEY_POWER },
	{ "MUTE",        "85", KEY_MUTE },
	{ "TIME",        "8D", KEY_PROGRAM },
	{ "V.FORMAT",    "AD", KEY_SWITCHVIDEOMODE },
	{ "TV/SAT",      "A5", KEY_AUX },
	{ "PICASA",      "E5", KEY_SUBTITLE },
	{ "SHOUTCAST",   "ED", KEY_AUDIO },
	{ "YOUTUBE",     "CD", KEY_VIDEO },
	{ "SPARK",       "C5", KEY_S },
	{ "0",           "57", KEY_0 },
	{ "1",           "B5", KEY_1 },
	{ "2",           "95", KEY_2 },
	{ "3",           "BD", KEY_3 },
	{ "4",           "F5", KEY_4 },
	{ "5",           "D5", KEY_5 },
	{ "6",           "FD", KEY_6 },
	{ "7",           "35", KEY_7 },
	{ "8",           "15", KEY_8 },
	{ "9",           "3D", KEY_9 },
	{ "TV/RADIO",    "77", KEY_TV2 },
	{ "RECALL",      "7F", KEY_BACK },
	{ "SAT",         "9D", KEY_SAT },
	{ "FAV",         "45", KEY_FAVORITES },
	{ "VOL-",        "C7", KEY_VOLUMEDOWN },
	{ "VOL+",        "DD", KEY_VOLUMEUP },
	{ "PAGE-",       "07", KEY_CHANNELDOWN },
	{ "PAGE+",       "5F", KEY_CHANNELUP },
	{ "INFO",        "1D", KEY_INFO },
	{ "EPG",         "87", KEY_EPG },
	{ "MENU",        "65", KEY_MENU },
	{ "EXIT",        "A7", KEY_EXIT },
	{ "UP",          "27", KEY_UP },
	{ "DOWN",        "0F", KEY_DOWN },
	{ "LEFT",        "6D", KEY_LEFT },
	{ "RIGHT",       "AF", KEY_RIGHT },
	{ "OK",          "2F", KEY_OK },
	{ "FIND",        "4D", KEY_SUBTITLE },
	{ "REC",         "8F", KEY_RECORD },
	{ "RED",         "75", KEY_RED },
	{ "GREEN",       "F7", KEY_GREEN },
	{ "YELLOW",      "37", KEY_YELLOW },
	{ "BLUE",        "B7", KEY_BLUE },
	{ "REWIND",      "55", KEY_REWIND },
	{ "PLAY",        "D7", KEY_PLAY },
	{ "PAUSE",       "17", KEY_PAUSE },
	{ "FASTFORWARD", "97", KEY_FASTFORWARD },
	{ "FOLDER",      "5D", KEY_FILE },
	{ "PLAY_MODE",   "DF", KEY_P },
	{ "USB",         "1F", KEY_MEDIA },
	{ "STOP",        "9F", KEY_STOP },
	{ "F1",          "7D", KEY_F1 },
	{ "F2",          "FF", KEY_F2 },
	{ "F3",          "3F", KEY_F3 },
	{ "F4",          "BF", KEY_F4 },
	{ "",            "",   KEY_NULL }
};

static tButton cButtonsSparkRc12[] =
{
	{ "MUTE",        "87", KEY_MUTE },
	{ "POWER",       "45", KEY_POWER },
	{ "PLAY_MODE",   "A7", KEY_P },
	{ "V.FORMAT",    "E5", KEY_SWITCHVIDEOMODE },
	{ "TIME",        "C5", KEY_PROGRAM },
	{ "USB",         "47", KEY_MEDIA },
	{ "FOLDER",      "65", KEY_FILE },
	{ "STOP",        "25", KEY_STOP },
	{ "PLAY",        "3D", KEY_PLAY },
	{ "PAUSE",       "1D", KEY_PAUSE },
	{ "FASTFORWARD", "C7", KEY_FASTFORWARD },
	{ "REWIND",      "FD", KEY_REWIND },
	{ "PREV",        "BF", KEY_PREVIOUS },
	{ "NEXT",        "E7", KEY_NEXT },
	{ "FAST",        "67", KEY_F },
	{ "SLOW",        "9F", KEY_SLOW },
	{ "MENU",        "DD", KEY_MENU },
	{ "Tms",         "BD", KEY_TIME },
	{ "INFO",        "FF", KEY_INFO },
	{ "UP",          "5D", KEY_UP },
	{ "DOWN",        "55", KEY_DOWN },
	{ "LEFT",        "1F", KEY_LEFT },
	{ "RIGHT",       "7F", KEY_RIGHT },
	{ "OK",          "7D", KEY_OK },
	{ "EXIT",        "3F", KEY_EXIT },
	{ "REC",         "9D", KEY_RECORD },
	{ "EPG",         "5F", KEY_EPG },
	{ "TV/SAT",      "D5", KEY_AUX },
	{ "RECALL",      "DF", KEY_BACK },
	{ "FIND",        "95", KEY_SUBTITLE },
	{ "VOL+",        "17", KEY_VOLUMEUP },
	{ "VOL-",        "37", KEY_VOLUMEDOWN },
	{ "SAT",         "15", KEY_SAT },
	{ "FAV",         "35", KEY_FAVORITES },
	{ "PAGE+",       "57", KEY_CHANNELUP },
	{ "PAGE-",       "D7", KEY_CHANNELDOWN },
	{ "0",           "0D", KEY_0 },
	{ "1",           "F5", KEY_1 },
	{ "2",           "B5", KEY_2 },
	{ "3",           "F7", KEY_3 },
	{ "4",           "CD", KEY_4 },
	{ "5",           "AD", KEY_5 },
	{ "6",           "77", KEY_6 },
	{ "7",           "0F", KEY_7 },
	{ "8",           "8D", KEY_8 },
	{ "9",           "4F", KEY_9 },
	{ "TV/RADIO",    "ED", KEY_TV2 },
	{ "RED",         "2F", KEY_RED },
	{ "GREEN",       "6F", KEY_GREEN },
	{ "YELLOW",      "EF", KEY_YELLOW },
	{ "BLUE",        "05", KEY_BLUE },
	{ "WHITE",       "2D", KEY_W },
	{ "",            "",   KEY_NULL }
};

/* spark Default (HOF-12D with portal key) */
static tButton cButtonsSparkDefault[] =
{
	{ "POWER",       "87", KEY_POWER },
	{ "V.FORMAT",    "0F", KEY_SWITCHVIDEOMODE },
	{ "TV/SAT",      "2F", KEY_AUX },
	{ "TIME",        "65", KEY_PROGRAM },
	{ "MUTE",        "A5", KEY_MUTE },
	{ "0",           "37", KEY_0 },
	{ "1",           "A7", KEY_1 },
	{ "2",           "07", KEY_2 },
	{ "3",           "E5", KEY_3 },
	{ "4",           "97", KEY_4 },
	{ "5",           "27", KEY_5 },
	{ "6",           "D5", KEY_6 },
	{ "7",           "B7", KEY_7 },
	{ "8",           "17", KEY_8 },
	{ "9",           "F5", KEY_9 },
	{ "TV/RADIO",    "CD", KEY_TV2 },
	{ "RECALL",      "CF", KEY_BACK },
	{ "PAGE+",       "ED", KEY_CHANNELUP },
	{ "PAGE-",       "DD", KEY_CHANNELDOWN },
	{ "FIND",        "C5", KEY_SUBTITLE },
	{ "FOLDER",      "67", KEY_FILE },
	{ "VOL+",        "AF", KEY_VOLUMEUP },
	{ "VOL-",        "9F", KEY_VOLUMEDOWN },
	{ "MENU",        "C7", KEY_MENU },
	{ "INFO",        "1F", KEY_INFO },
	{ "UP",          "3F", KEY_UP },
	{ "DOWN",        "85", KEY_DOWN },
	{ "LEFT",        "BF", KEY_LEFT },
	{ "RIGHT",       "FD", KEY_RIGHT },
	{ "OK",          "05", KEY_OK },
	{ "EXIT",        "F9", KEY_EXIT },
	{ "EPG",         "45", KEY_EPG },
	{ "FAV",         "3D", KEY_FAVORITES },
	{ "PORTAL",      "25", KEY_WWW },
	{ "SAT",         "0D", KEY_SAT },
	{ "RED",         "6D", KEY_RED },
	{ "GREEN",       "8D", KEY_GREEN },
	{ "YELLOW",      "77", KEY_YELLOW },
	{ "BLUE",        "AD", KEY_BLUE },
	{ "REC",         "F7", KEY_RECORD },
	{ "STOP",        "BB", KEY_STOP },
	{ "PLAY",        "57", KEY_PLAY },
	{ "PAUSE",       "4D", KEY_PAUSE },
	{ "FASTFORWARD", "35", KEY_FASTFORWARD },
	{ "REWIND",      "7F", KEY_REWIND },
	{ "PREV",        "FB", KEY_PREVIOUS },
	{ "NEXT",        "3B", KEY_NEXT },
	{ "FAST",        "E7", KEY_F },
	{ "SLOW",        "7B", KEY_SLOW },
	{ "PLAY_MODE",   "B5", KEY_P },
	{ "USB",         "DF", KEY_MEDIA },
	{ "Tms",         "55", KEY_TIME },
	{ "F1",          "95", KEY_F1 },
	{ "F2",          "15", KEY_F2 },
	{ "F3",          "D7", KEY_F3 },
	{ "",            "",   KEY_NULL },
};

static tButton cButtonsSognoTriplex[] =
{
	{ "POWER",       "10", KEY_POWER },
	{ "V.Format",    "01", KEY_MODE },
	{ "TV/SAT",      "05", KEY_AUX },  // !
	{ "TIME",        "4C", KEY_PROGRAM },
	{ "KEY_MUTE",    "54", KEY_MUTE },
	{ "1",           "14", KEY_1 },
	{ "2",           "00", KEY_2 },
	{ "3",           "5C", KEY_3 },
	{ "4",           "12", KEY_4 },
	{ "5",           "04", KEY_5 },
	{ "6",           "5A", KEY_6 },
	{ "7",           "16", KEY_7 },
	{ "8",           "02", KEY_8 },
	{ "9",           "5E", KEY_9 },
	{ "TV/RADIO",    "59", KEY_TV2 },
	{ "0",           "06", KEY_0 },
	{ "RECALL",      "19", KEY_BACK },
	{ "PAGE+",       "5D", KEY_CHANNELUP },
	{ "PAGE-",       "5B", KEY_CHANNELDOWN },
	{ "FIND",        "58", KEY_SUBTITLE },
	{ "VOL+",        "15", KEY_VOLUMEUP },
	{ "VOL-",        "13", KEY_VOLUMEDOWN },
	{ "FOLDER",      "0C", KEY_FILE },
	{ "MENU",        "18", KEY_MENU },
	{ "INFO",        "03", KEY_INFO },
	{ "UP",          "07", KEY_UP },
	{ "LEFT",        "17", KEY_LEFT },
	{ "RIGHT",       "5F", KEY_RIGHT },
	{ "DOWN",        "50", KEY_DOWN },
	{ "OK",          "40", KEY_OK },
	{ "EXIT",        "DF", KEY_EXIT },
	{ "EPG",         "48", KEY_EPG },
	{ "FAVORITES",   "47", KEY_FAVORITES },
	{ "PORTAL",      "44", KEY_OPEN },
	{ "SAT",         "41", KEY_SAT },
	{ "RED",         "4D", KEY_RED },
	{ "GREEN",       "51", KEY_GREEN },
	{ "YELLOW",      "0E", KEY_YELLOW },
	{ "BLUE",        "55", KEY_BLUE },
	{ "RECORD",      "1E", KEY_RECORD },
	{ "STOP",        "97", KEY_STOP },
	{ "PAUSE",       "49", KEY_PAUSE },
	{ "PLAY",        "0A", KEY_PLAY },
	{ "PREVIOUS",    "9F", KEY_PREVIOUS },
	{ "NEXT",        "87", KEY_NEXT },
	{ "REWIND",      "0F", KEY_REWIND },
	{ "FASTFORWARD", "46", KEY_FASTFORWARD },
//	{ "REWIND",      "8F", KEY_REWIND },
//	{ "FASTFORWARD", "1C", KEY_FASTFORWARD },
	{ "FAST",        "8F", KEY_F },
	{ "SLOW",        "1C", KEY_SLOW },
	{ "PLAY_MODE",   "56", KEY_PLAYPAUSE },
	{ "USB",         "1B", KEY_MEDIA },
	{ "Tms",         "4A", KEY_TIME },
	{ "F1",          "52", KEY_F1 },
	{ "F2",          "42", KEY_F2 },
	{ "F3",          "1A", KEY_F3 },
	{ "",            "",   KEY_NULL }
};

static tButton cButtonsSamsungAA59[] =
{
	{ "POWER",       "BF", KEY_POWER },
	{ "MODE",        "7F", KEY_MODE },       //# SOURCE
	{ "SLEEP",       "2E", KEY_PROGRAM },      //# HDMI
	{ "1",           "DF", KEY_1 },
	{ "2",           "5F", KEY_2 },
	{ "3",           "9F", KEY_3 },
	{ "4",           "EF", KEY_4 },
	{ "5",           "6F", KEY_5 },
	{ "6",           "AF", KEY_6 },
	{ "7",           "CF", KEY_7 },
	{ "8",           "4F", KEY_8 },
	{ "9",           "8F", KEY_9 },
	{ "0",           "77", KEY_0 },
	{ "VOL+",        "1F", KEY_VOLUMEUP },
	{ "VOL-",        "2F", KEY_VOLUMEDOWN },
	{ "PAGE+",       "B7", KEY_CHANNELUP },
	{ "PAGE-",       "F7", KEY_CHANNELDOWN },
	{ "MUTE",        "0F", KEY_MUTE },
	{ "TEXT",        "29", KEY_TEXT },       //# CKLIST
	{ "KEY_MENU",    "A7", KEY_MENU },
	{ "Smart",       "61", KEY_FILE },       //# SMART
	{ "EPG",         "0D", KEY_EPG },        //# GUIDE
	{ "FAV",         "2D", KEY_FAVORITES },  //# TOOLS
	{ "INFO",        "07", KEY_INFO },
	{ "UP",          "F9", KEY_UP },
	{ "DOWN",        "79", KEY_DOWN },
	{ "LEFT",        "59", KEY_LEFT },
	{ "RIGHT",       "B9", KEY_RIGHT },
	{ "OK",          "E9", KEY_OK },
	{ "RECALL",      "E5", KEY_BACK },       //# RETURN
	{ "EXIT",        "4B", KEY_EXIT },       //# EXIT
	{ "RED",         "C9", KEY_RED },
	{ "GREEN",       "D7", KEY_GREEN },
	{ "YELLOW",      "57", KEY_YELLOW },
	{ "BLUE",        "97", KEY_BLUE },
	{ "AUDIO",       "D9", KEY_AUDIO },      //# SOCIAL
	{ "SERACH",      "31", KEY_SUBTITLE },   //# SEARCH
	{ "3D",          "06", KEY_LIST },       //# 3D
	{ "MANUAL",      "03", KEY_PREVIOUS },   //# MANUAL
	{ "SIZE",        "83", KEY_NEXT },       //# SIZE
	{ "SUB",         "5B", KEY_SUBTITLE },
	{ "REWIND",      "5D", KEY_REWIND },
	{ "PAUSE",       "AD", KEY_PAUSE },
	{ "FASTFORWARD", "ED", KEY_FASTFORWARD },
	{ "RECORD",      "6D", KEY_RECORD },
	{ "PLAY",        "1D", KEY_PLAY },
	{ "STOP",        "9D", KEY_STOP },
	{ "",            "",   KEY_NULL }
};

static tButton cButtonsUfs910Rc660[] =
{
	{ "POWER",       "0c", KEY_POWER },
	{ "MUTE",        "0d", KEY_MUTE },
	{ "1",           "01", KEY_1 },
	{ "2",           "02", KEY_2 },
	{ "3",           "03", KEY_3 },
	{ "4",           "04", KEY_4 },
	{ "5",           "05", KEY_5 },
	{ "6",           "06", KEY_6 },
	{ "7",           "07", KEY_7 },
	{ "8",           "08", KEY_8 },
	{ "9",           "09", KEY_9 },
	{ "MENU",        "54", KEY_MENU },
	{ "0",           "00", KEY_0 },
	{ "TEXT",        "3c", KEY_TEXT },
	{ "RED",         "6d", KEY_RED },
	{ "GREEN",       "6e", KEY_GREEN },
	{ "YELLOW",      "6f", KEY_YELLOW },
	{ "BLUE",        "70", KEY_BLUE },
	{ "VOL+",        "10", KEY_VOLUMEUP },
	{ "INFO",        "0f", KEY_INFO },
	{ "PAGE+",       "1e", KEY_CHANNELUP },
	{ "VOL-",        "11", KEY_VOLUMEDOWN },
	{ "UP",          "58", KEY_UP },
	{ "PAGE-",       "1d", KEY_CHANNELDOWN },
	{ "LEFT",        "5a", KEY_LEFT },
	{ "OK",          "5c", KEY_OK },
	{ "RIGHT",       "5c", KEY_RIGHT },
	{ "EXIT",        "55", KEY_EXIT },
	{ "DOWN",        "59", KEY_DOWN },
	{ "EPG",         "cc", KEY_EPG },
	{ "REWIND",      "21", KEY_REWIND },
	{ "PLAY",        "38", KEY_PLAY },
	{ "FASTFORWARD", "20", KEY_FASTFORWARD },
	{ "PAUSE",       "39", KEY_PAUSE },
	{ "RECORD",      "37", KEY_RECORD },
	{ "STOP",        "31", KEY_STOP },
	{ "",            "",   KEY_NULL }
};

static tButton cButtonsUfs913Rc230[] =
{
	{ "TV/R",        "7e", KEY_TV2 },
	{ "POWER",       "f3", KEY_POWER },
	{ "1",           "fe", KEY_1 },
	{ "2",           "fd", KEY_2 },
	{ "3",           "fc", KEY_3 },
	{ "4",           "fb", KEY_4 },
	{ "5",           "fa", KEY_5 },
	{ "6",           "f9", KEY_6 },
	{ "7",           "f8", KEY_7 },
	{ "8",           "f7", KEY_8 },
	{ "9",           "f6", KEY_9 },
	{ "MENU",        "ab", KEY_MENU },
	{ "0",           "ff", KEY_0 },
	{ "TEXT",        "c3", KEY_TEXT },
	{ "VOL-",        "ee", KEY_VOLUMEDOWN },
	{ "PAGE+",       "e1", KEY_CHANNELUP },
	{ "VOL+",        "ef", KEY_VOLUMEUP },
	{ "MUTE",        "f2", KEY_MUTE },
	{ "PAGE-",       "e0", KEY_CHANNELDOWN },
	{ "RED",         "92", KEY_RED },
	{ "GREEN",       "91", KEY_GREEN },
	{ "YELLOW",      "90", KEY_YELLOW },
	{ "BLUE",        "8f", KEY_BLUE },
	{ "EPG",         "33", KEY_EPG },
	{ "UP",          "a7", KEY_UP },
	{ "MEDIA",       "b9", KEY_MEDIA },
	{ "LEFT",        "a5", KEY_LEFT },
	{ "OK",          "a3", KEY_OK },
	{ "INFO",        "F0", KEY_INFO },
	{ "RIGHT",       "a4", KEY_RIGHT },
	{ "EXIT",        "aa", KEY_EXIT },
	{ "DOWN",        "a6", KEY_DOWN },
	{ "PORTAL",      "2a", KEY_WWW },
	{ "REWIND",      "de", KEY_REWIND },
	{ "PLAY",        "c7", KEY_PLAY },
	{ "FASTFORWARD", "df", KEY_FASTFORWARD },
	{ "PAUSE",       "c6", KEY_PAUSE },
	{ "RECORD",      "c8", KEY_RECORD },
	{ "STOP",        "ce", KEY_STOP },
	{ "",            "",   KEY_NULL }
};

static tButton cButtonsSparkRc04[] =
{
	{ "POWER",       "45", KEY_POWER },
	{ "V.FORMAT",    "4D", KEY_SWITCHVIDEOMODE },
	{ "TIME",        "ED", KEY_PROGRAM },
	{ "MUTE",        "8D", KEY_MUTE },
	{ "Tms",         "57", KEY_TIME },
	{ "F1",          "37", KEY_F1 },
	{ "TV/SAT",      "DD", KEY_AUX },  // !
	{ "1",           "3F", KEY_1 },
	{ "2",           "07", KEY_2 },
	{ "3",           "BD", KEY_3 },
	{ "4",           "5D", KEY_4 },
	{ "5",           "1F", KEY_5 },
	{ "6",           "9D", KEY_6 },
	{ "7",           "55", KEY_7 },
	{ "8",           "17", KEY_8 },
	{ "9",           "95", KEY_9 },
	{ "0",           "27", KEY_0 },
	{ "TV/RADIO",    "65", KEY_TV2 },
	{ "RECALL",      "A5", KEY_BACK },
	{ "FIND",        "75", KEY_SUBTITLE },
	{ "REC",         "D5", KEY_RECORD },
	{ "SAT",         "A7", KEY_SAT },
	{ "FAV",         "B5", KEY_FAVORITES },
	{ "MENU",        "1D", KEY_MENU },
	{ "INFO",        "5F", KEY_INFO },
	{ "OK",          "BF", KEY_OK },
	{ "UP",          "9F", KEY_UP },
	{ "DOWN",        "AF", KEY_DOWN },
	{ "LEFT",        "3D", KEY_LEFT },
	{ "RIGHT",       "7F", KEY_RIGHT },
	{ "EXIT",        "2D", KEY_EXIT },
	{ "EPG",         "6F", KEY_EPG },
	{ "FOLDER",      "0D", KEY_FILE },
	{ "STOP",        "8F", KEY_STOP },
	{ "PAUSE",       "CD", KEY_PAUSE },
	{ "PLAY",        "4F", KEY_PLAY },
	{ "PREV",        "35", KEY_PREVIOUS },
	{ "NEXT",        "B7", KEY_NEXT },
	{ "FASTFORWARD", "77", KEY_FASTFORWARD },
	{ "REWIND",      "F5", KEY_REWIND },
	{ "FAST",        "97", KEY_F },
	{ "SLOW",        "15", KEY_SLOW },
	{ "PLAY_MODE",   "E5", KEY_P },
	{ "USB",         "67", KEY_MEDIA },
	{ "UHF",         "0F", KEY_U },
	{ "AUDIO",       "25", KEY_AUDIO },
	{ "RED",         "05", KEY_RED },
	{ "GREEN",       "87", KEY_GREEN },
	{ "YELLOW",      "C5", KEY_YELLOW },
	{ "BLUE",        "47", KEY_BLUE },
	{ "",            "",   KEY_NULL }
};

/* GALAXY RC */
static tButton cButtonsGalaxy[] =
{
	{ "POWER",        "25", KEY_POWER },
	{ "R",            "A5", KEY_R },
	{ "V.FORMAT",     "AD", KEY_SWITCHVIDEOMODE },
	{ "TIME",         "8D", KEY_PROGRAM },
	{ "MUTE",         "85", KEY_MUTE },
	{ "TV/SAT",       "C5", KEY_AUX },
	{ "Tms",          "E5", KEY_TIME },
	{ "PRESENTATION", "ED", KEY_PRESENTATION },
	{ "F1",           "CD", KEY_F1 },
	{ "0",            "57", KEY_0 },
	{ "1",            "B5", KEY_1 },
	{ "2",            "95", KEY_2 },
	{ "3",            "BD", KEY_3 },
	{ "4",            "F5", KEY_4 },
	{ "5",            "D5", KEY_5 },
	{ "6",            "FD", KEY_6 },
	{ "7",            "35", KEY_7 },
	{ "8",            "15", KEY_8 },
	{ "9",            "3D", KEY_9 },
	{ "TV/RADIO",     "77", KEY_TV2 },
	{ "RECALL",       "7F", KEY_BACK },
	{ "VOL+",         "C7", KEY_VOLUMEDOWN },
	{ "VOL-",         "DD", KEY_VOLUMEUP },
	{ "PAGE-",        "5F", KEY_CHANNELDOWN },
	{ "PAGE+",        "07", KEY_CHANNELUP },
	{ "FIND",         "9D", KEY_SUBTITLE },
	{ "SAT",          "1D", KEY_SAT },
	{ "REC",          "45", KEY_RECORD },
	{ "FAV",          "87", KEY_FAVORITES },
	{ "MENU",         "65", KEY_MENU },
	{ "INFO",         "A7", KEY_INFO },
	{ "EXIT",         "4D", KEY_EXIT },
	{ "EPG",          "8F", KEY_EPG },
	{ "OK",           "2F", KEY_OK },
	{ "UP",           "27", KEY_UP },
	{ "DOWN",         "0F", KEY_DOWN },
	{ "LEFT",         "6D", KEY_LEFT },
	{ "RIGHT",        "AF", KEY_RIGHT },
	{ "FOLDER",       "75", KEY_FILE },
	{ "STOP",         "F7", KEY_STOP },
	{ "PAUSE",        "37", KEY_PAUSE },
	{ "PLAY",         "B7", KEY_PLAY },
	{ "PREV",         "55", KEY_PREVIOUS },
	{ "NEXT",         "D7", KEY_NEXT },
	{ "REWIND",       "17", KEY_REWIND },
	{ "FORWARD",      "97", KEY_FORWARD },
	{ "USB",          "9F", KEY_MEDIA },
	{ "RED",          "7D", KEY_RED },
	{ "GREEN",        "FF", KEY_GREEN },
	{ "YELLOW",       "3F", KEY_YELLOW },
	{ "BLUE",         "BF", KEY_BLUE },
	{ "PLAY_MODE",    "1F", KEY_P },
	{ "SLOW",         "5D", KEY_SLOW },
	{ "FAST",         "DF", KEY_FASTFORWARD },
	{ "",             "",   KEY_NULL }
};

static tButton cButtonsSparkEdv[] =  // Edision argus pingulux, RC1/RC2 switchable
{  // HOF13E
	{ "POWER",       "87", KEY_POWER },
	{ "RC12",        "25", KEY_R },
	{ "V.FORMAT",    "2f", KEY_SWITCHVIDEOMODE },
	{ "TIME",        "65", KEY_PROGRAM },
	{ "MUTE",        "A5", KEY_MUTE },
	{ "0",           "37", KEY_0 },
	{ "1",           "A7", KEY_1 },
	{ "2",           "07", KEY_2 },
	{ "3",           "E5", KEY_3 },
	{ "4",           "97", KEY_4 },
	{ "5",           "27", KEY_5 },
	{ "6",           "D5", KEY_6 },
	{ "7",           "B7", KEY_7 },
	{ "8",           "17", KEY_8 },
	{ "9",           "F5", KEY_9 },
	{ "TV/RADIO",    "CD", KEY_TV2 },
	{ "RECALL",      "CF", KEY_BACK },
	{ "PAGE+",       "ED", KEY_CHANNELUP },
	{ "PAGE-",       "DD", KEY_CHANNELDOWN },
	{ "FIND",        "C5", KEY_SUBTITLE },
	{ "FOLDER",      "67", KEY_FILE },
	{ "VOL+",        "AF", KEY_VOLUMEUP },
	{ "VOL-",        "9F", KEY_VOLUMEDOWN },
	{ "MENU",        "C7", KEY_MENU },
	{ "INFO",        "1F", KEY_INFO },
	{ "UP",          "3F", KEY_UP },
	{ "DOWN",        "85", KEY_DOWN },
	{ "LEFT",        "BF", KEY_LEFT },
	{ "RIGHT",       "FD", KEY_RIGHT },
	{ "OK",          "05", KEY_OK },
	{ "EXIT",        "F9", KEY_EXIT },
	{ "EPG",         "45", KEY_EPG },
	{ "FAV",         "3D", KEY_FAVORITES },
	{ "PORTAL",      "25", KEY_WWW },
	{ "SAT",         "0D", KEY_SAT },
	{ "RED",         "6D", KEY_RED },
	{ "GREEN",       "8D", KEY_GREEN },
	{ "YELLOW",      "77", KEY_YELLOW },
	{ "BLUE",        "AD", KEY_BLUE },
	{ "REC",         "F7", KEY_RECORD },
	{ "STOP",        "BB", KEY_STOP },
	{ "PLAY",        "57", KEY_PLAY },
	{ "PAUSE",       "4D", KEY_PAUSE },
	{ "FASTFORWARD", "35", KEY_FASTFORWARD },
	{ "REWIND",      "7F", KEY_REWIND },
	{ "PREV",        "FB", KEY_PREVIOUS },
	{ "NEXT",        "3B", KEY_NEXT },
	{ "FAST",        "E7", KEY_F },
	{ "SLOW",        "7B", KEY_SLOW },
	{ "PLAY_MODE",   "B5", KEY_P },
	{ "USB",         "DF", KEY_MEDIA },
	{ "TimeShift",   "55", KEY_TIME },
	{ "TV/SAT",      "95", KEY_AUX },
	{ "F1",          "15", KEY_F1 },
	{ "F2",          "D1", KEY_F2 },
	{ "",            "",   KEY_NULL }
};

/* fixme: move this to a structure and
 * use the private structure of RemoteControl_t
 */
static struct sockaddr_un vAddr;


void Get_StbId()
{
	char *pch;
	int fn = open("/proc/cmdline", O_RDONLY);

//	printf("[evremote2 spark] %s >\n", __func__);
	if (fn > -1)
	{
		char procCmdLine[1024];
		int len = read(fn, procCmdLine, sizeof(procCmdLine) - 1);
		if (len > 0)
		{
			procCmdLine[len] = 0;
			pch = strstr(procCmdLine, "STB_ID=");
			if (pch != NULL)
			{
				strncpy(VendorStbId, pch + 7, 8);
				printf("[evremote2 spark] Vendor STB-ID = %s\n", VendorStbId);
			}
			else
			{
				printf("[evremote2 spark] No Vendor STB-ID found\n");
			}
		}
		close(fn);
	}
//	printf("[evremote2 spark] %s <\n", __func__);
}

int string2bin(const char *string)
{
	int predata;
	int i;

	predata = 0;
	for (i = 0; i < strlen(string); i++)
	{
		predata = predata << 4;
		predata += string[i] & 0x0f;
		if (string[i] > '9')
		{
			predata += 9;
		}
	}
	return predata;
}

static tButton *pSparkGetButton(int predata)
{
	tButton	*pButtons = cButtonsSparkDefault;

	printf("[evremote2 spark] Predata: 0x%04x\n", predata);
	if (predata == SPARK_RC05_PREDATA)  // also Edision RC2
	{
		pButtons = cButtonsEdisionSpark;
	}
	else if (predata == SPARK_RC08_PREDATA)
	{
		pButtons = cButtonsSparkRc08;
	}
	else if (predata == SPARK_RC09_PREDATA)  // also Edision RC1
	{
		static tButton *cButtons = NULL;

		if (!cButtons)
		{
			if (strstr(STB_ID_EDISION_PINGULUX, VendorStbId))
			{
				cButtons = cButtonsEdisionSpark;
			}
			else if (strstr(STB_ID_GALAXYINNOVATIONS_S8120, VendorStbId))
			{
				cButtons = cButtonsGalaxy;
			}
			else
			{
				cButtons = cButtonsSparkRc09; /* Amiko Alien 8900 */
			}
		}
		return cButtons;
	}
	else if (predata == SPARK_DEFAULT_PREDATA)
	{
		static tButton *cButtons = NULL;

		if (!cButtons)
		{
			if (strstr(STB_ID_SOGNO_TRIPLE_HD, VendorStbId))
			{
				cButtons = cButtonsSognoTriplex;
			}
//			else if (strstr(STB_ID_SAB_UNIX_TRIPLE_HD, VendorStbId))
//			{
//				cButtons = cButtonsSparkDefault;
//			}
			else
			{
				cButtons = cButtonsSparkDefault;
			}
		}
		return cButtons;
	}
	else if (predata == UFS910_RC660_PREDATA)
	{
		pButtons = cButtonsUfs910Rc660;
	}
	else if (predata == UFS913_RC230_PREDATA)
	{
		pButtons = cButtonsUfs913Rc230;
	}
	else if (predata == SAMSUNG_AA59_PREDATA)
	{
		pButtons = cButtonsSamsungAA59;
	}
	else if (predata == SPARK_RC12_PREDATA)
	{
		pButtons = cButtonsSparkRc12;
	}
	else if (predata == SPARK_RC04_PREDATA)
	{
		pButtons = cButtonsSparkRc04;
	}
	else if (predata == SPARK_EDV_RC1)
	{
		pButtons = cButtonsSparkEdv;
	}
	else if (predata == SPARK_EDV_RC2)
	{
		pButtons = cButtonsSparkEdv;
	}
	return pButtons;
}

static int pInit(Context_t *context, int argc, char *argv[])
{
	int vHandle;

//	printf("[evremote2 spark] %s >\n", __func__);
	Get_StbId();
	vAddr.sun_family = AF_UNIX;
	strcpy(vAddr.sun_path, "/var/run/lirc/lircd");

	vHandle = socket(AF_UNIX, SOCK_STREAM, 0);
	if (vHandle == -1)
	{
		perror("socket");
		return -1;
	}
	if (connect(vHandle, (struct sockaddr *)&vAddr, sizeof(vAddr)) == -1)
	{
		perror("connect");
		return -1;
	}
	if (! access("/etc/.rccode", F_OK))
	{
		char buf[10];
		int val;
		FILE* fd;

		fd = fopen("/etc/.rccode", "r");
		if (fd != NULL)
		{
			if (fgets(buf, sizeof(buf), fd) != NULL)
			{
				val = atoi(buf);
				if (val > 0 && val < 3)
				{
					cLongKeyPressSupport.rc_code = val;
					printf("[evremote2 spark] Selected RC Code: %d\n", cLongKeyPressSupport.rc_code);
				}
				else
				{
					cLongKeyPressSupport.rc_code = 1;  // set default RC code
				}
			}
			fclose(fd);
		}
	}
	else
	{
		cLongKeyPressSupport.rc_code = 1;  // set default RC code
	}
	printf("[evremote2 spark] RC code = %d\n", cLongKeyPressSupport.rc_code);
	return vHandle;
}

static int pShutdown(Context_t *context)
{
	close(context->fd);
	return 0;
}

static int pRead(Context_t *context)
{
	char vBuffer[128];
	char vData[10];
	const int cSize = 128;
	int vCurrentCode = -1;
	struct vfd_ioctl_data data;
	int rc;
	int predata;
	int ioctl_fd = -1;
	tButton *cButtons;

	// set default button table
	if (strstr(STB_ID_EDISION_PINGULUX, VendorStbId))
	{
		cButtons = cButtonsSparkEdv;
	}
	else
	{
		cButtons = cButtonsEdisionSpark;
	}
//	printf("[evremote2 spark] %s >\n", __func__);
	memset(vBuffer, 0, 128);
	// wait for new command
	rc = read(context->fd, vBuffer, cSize);
	if (rc <= 0)
	{
		return -1;
	}
	// determine predata (RC1 or RC2 in case of Edision)
	vData[0] = vBuffer[8];
	vData[1] = vBuffer[9];
	vData[2] = vBuffer[10];
	vData[3] = vBuffer[11];
	vData[4] = '\0';
	predata = string2bin(vData);
	cButtons = pSparkGetButton(predata);

	vData[0] = vBuffer[14];
	vData[1] = vBuffer[15];
	vData[2] = '\0';
//	printf("[evremote2 spark] Key: 0x%s -> %s", vData, &vBuffer[0]);
	printf("[evremote2 spark] Key: 0x%s\n", vData);
	vCurrentCode = getInternalCode(cButtons, vData);

	if (strstr(STB_ID_EDISION_PINGULUX, VendorStbId)
	&&  (predata == SPARK_EDV_RC1 || predata == SPARK_EDV_RC2 || predata == SPARK_RC09_PREDATA || predata == SPARK_RC05_PREDATA))  // Edision has RC1 / RC2 switching
	{
		if (((predata == SPARK_EDV_RC1 || predata == SPARK_EDV_RC2) && vData[0] == '0' && vData[1] == 'f')  // for HOF14
		||  ((predata == SPARK_RC09_PREDATA || predata == SPARK_RC05_PREDATA) && vData[0] == 'a' && vData[1] == '5'))  // for HOF11
		{
			if (vBuffer[17] == '0' && vBuffer[18] == '0')  // RC12 key pressed
			{
				if (! access("/etc/.rccode", F_OK))
				{
					char buf[2];
					int fd;
	
					memset(buf, 0, sizeof(buf));
	
					fd = open("/etc/.rccode", O_RDWR);
					if (fd >= 0)
					{
						if (read(fd, buf, 1) == 1)
						{
							rc = '1';
							if (predata == SPARK_EDV_RC2 || predata == SPARK_RC05_PREDATA)
							{
								rc++;
							}
							buf[0] = rc;
							lseek (fd, 0, SEEK_SET);
							if (write(fd, buf, 1) == 1)
							{
								context->r->LongKeyPressSupport->rc_code = rc & 0x03;
							}
						}
						else
						{
							rc = '1';
							context->r->LongKeyPressSupport->rc_code = 1;
						}
						printf("[evremote2 spark] RC Code set to: %c\n", rc);
		
						data.length = sprintf((char *)data.data, "Code RC%c\n", rc);
						data.length--;
						data.data[data.length] = 0;
						ioctl_fd = open("/dev/vfd", O_RDONLY);
						ioctl(ioctl_fd, VFDDISPLAYCHARS, &data);
						close(ioctl_fd);
					}	
					else
					{
						context->r->LongKeyPressSupport->rc_code = 1;  // set default RC code
					}
					close(fd);
				}
			}
			vCurrentCode = 0;  // ignore RC12 key
		}
		else
		{
			// predata and rc code must match
			if ((context->r->LongKeyPressSupport->rc_code == 1 && predata == SPARK_EDV_RC1)
			||  (context->r->LongKeyPressSupport->rc_code == 2 && predata == SPARK_EDV_RC2)
			||  (context->r->LongKeyPressSupport->rc_code == 1 && predata == SPARK_RC09_PREDATA)
			||  (context->r->LongKeyPressSupport->rc_code == 2 && predata == SPARK_RC05_PREDATA))
			{
				if (vCurrentCode != 0)
				{
					static int nextflag = 0;
		
					if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
					{
						nextflag++;
					}
					vCurrentCode += (nextflag << 16);
				}
			}
			else
			{
				vCurrentCode = 0;  // ignore key
			}
		}
	}
	else
	{
		if (vCurrentCode != 0)
		{
			static int nextflag = 0;

			if (('0' == vBuffer[17]) && ('0' == vBuffer[18]))
			{
				nextflag++;
			}
			vCurrentCode += (nextflag << 16);
		}
	}
//	printf("[evremote2 spark] %s <\n", __func__);
	return vCurrentCode;
}

static int pNotification(Context_t *context, const int cOn)
{
	struct aotom_ioctl_data vfd_data;
	int ioctl_fd = -1;

//	printf("[evremote2 spark] %s > cOn = %d\n", __func__, cOn);
	vfd_data.u.icon.on = cOn ? 1 : 0;
	vfd_data.u.icon.icon_nr = 35;
	if (!cOn)
	{
		usleep(100000);
	}
	ioctl_fd = open("/dev/vfd", O_RDONLY);
	ioctl(ioctl_fd, VFDICONDISPLAYONOFF, &vfd_data);
	close(ioctl_fd);
//	printf("[evremote2 spark] %s <\n", __func__);
	return 0;
}

RemoteControl_t Spark_RC =
{
	"Spark RemoteControl",
	Spark,
	cButtonsSparkDefault,
	NULL,
	NULL,
	1,
	&cLongKeyPressSupport
};

BoxRoutines_t Spark_BR =
{
	&pInit,
	&pShutdown,
	&pRead,
	&pNotification
};
// vim:ts=4

