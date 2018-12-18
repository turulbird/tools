/*
 * output class
 *
 * Copyright (C) 2014  martii
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "writer.h"
#include "libthread.h"

struct subtitleData
{
	uint32_t start_ms;
	uint32_t duration_ms;
	uint32_t end_ms;
	std::string text;
	subtitleData()
		:start_ms(0), duration_ms(0), end_ms(0){}
};

struct DVBApiVideoInfo
{
	int width;
	int height;
	int frame_rate;
	int progressive;
	int aspect;
	DVBApiVideoInfo()
		:width(-1), height(-1), frame_rate(-1), progressive(-1), aspect(-1){}
};

class Player;

class Output
{
	friend class Player;

	private:
		int videofd;
		int audiofd;
		std::map<uint32_t, subtitleData> embedded_subtitle;
		DVBApiVideoInfo videoInfo;
		Writer *videoWriter, *audioWriter;
		Mutex audioMutex, videoMutex, subtitleMutex;
		Track *audioTrack, *videoTrack;
		Player *player;
		bool GetEvent();
		const char *ass_get_text(char *str);
	public:
		Output();
		~Output();
		bool Open();
		bool Close();
		bool Play();
		bool Stop();
		bool Pause();
		bool Continue();
		bool Mute(bool);
		bool Flush();
		bool FastForward(int speed);
		bool SlowMotion(int speed);
		bool AVSync(bool);
		bool Clear();
		bool ClearAudio();
		bool ClearVideo();
		bool GetPts(int64_t &pts);
		bool GetFrameCount(int64_t &framecount);
		bool SwitchAudio(Track *track);
		bool SwitchVideo(Track *track);
		bool Write(AVStream *stream, AVPacket *packet, int64_t Pts);
		bool WriteSubtitle(AVStream *stream, AVPacket *packet, int64_t pts);
		void sendLibeplayerMessage(int msg);
		bool GetSubtitles(std::map<uint32_t, subtitleData> &subtitles);
		void GetVideoInfo(DVBApiVideoInfo &video_info);
};

#endif
// vim:ts=4
