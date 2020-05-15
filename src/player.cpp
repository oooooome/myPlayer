#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include "packetqueue.h"
#include "video.h"
#include "audio.h"
using namespace std;

#define _STDC_CONSTANT_MACROS
#define SDL_MAIN_HANDLED
extern "C" {
#include <SDL2/SDL.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
};

const int MAXSIZE = 3;
const int VIDEOTYPE = 1;
const int AUDIOTYPE = 2;

AVFormatContext* pFormatCtx;
AVCodecParameters* para4video;
AVCodecParameters* para4audio;
AVCodecContext* codecCtx4video;
AVCodecContext* codecCtx4audio;
AVCodec* pCodec4video;
AVCodec* pCodec4audio;
int videoIndex = -1, audioIndex = -1;
AVFrame* frame4Video = av_frame_alloc();
AVFrame* frame4Audio = av_frame_alloc();

extern std::mutex pkMutex4Video;	
extern std::mutex pkMutex4Audio;
extern queue<AVPacket*> videoQueue;
extern queue<AVPacket*> audioQueue;

//存储时间戳信息，用于音画同步
AVRational streamTimeBase4Video{ 1,0 };
AVRational streamTimeBase4Audio{ 1,0 };
std::atomic<uint64_t> currentTimestamp4Audio{ 0 };

#define REFRESH_EVENT (SDL_USEREVENT + 1)

#define BREAK_EVENT (SDL_USEREVENT + 2)

int audio_samples = -1;



int initAVCodecContext(string filePath) {
	pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
		cout << "Couldn't open input stream" << endl;
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {  //获取视频文件信息
		cout << "Couldn't find stream information" << endl;
		return -1;
	}
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
			cout << "get video index " << videoIndex << endl;
			streamTimeBase4Video = pFormatCtx->streams[i]->time_base;
		}
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioIndex = i;
			cout << "get audio index " << audioIndex << endl;
			streamTimeBase4Audio = pFormatCtx->streams[i]->time_base;
		}
	}
	if (videoIndex == -1) {
		cout << "Didn't find a video stream" << endl;
		return -1;
	}
	if (audioIndex == -1) {
		cout << "Didn't find a audio stream" << endl;
		return -1;
	}

	para4video = pFormatCtx->streams[videoIndex]->codecpar;
	para4audio = pFormatCtx->streams[audioIndex]->codecpar;
	pCodec4video = avcodec_find_decoder(para4video->codec_id);  //视频编解码器
	pCodec4audio = avcodec_find_decoder(para4audio->codec_id);
	if (pCodec4video == nullptr || pCodec4audio == nullptr) {
		cout << "Codec not found." << endl;
		return -1;
	}
	codecCtx4video = avcodec_alloc_context3(pCodec4video);
	codecCtx4audio = avcodec_alloc_context3(pCodec4audio);
	if (!codecCtx4video || !codecCtx4audio) {
		cout << "Could not allocate video codec context" << endl;
		return -1;
	}
	if (avcodec_parameters_to_context(codecCtx4video, para4video) != 0
		|| avcodec_parameters_to_context(codecCtx4audio, para4audio) != 0) {
		cout << "Could not copy codec context" << endl;
		return -1;
	}
	if (avcodec_open2(codecCtx4video, pCodec4video, nullptr) < 0
		|| avcodec_open2(codecCtx4audio, pCodec4audio, nullptr) < 0) {  //打开解码器
		cout << "Could not open codec" << endl;
		return -1;
	}
	return 0;
}

void play(string filePath) {
	initAVCodecContext(filePath);					//初始化ffmpeg相关组件

	thread grabPacket{ grabPacketToQueue };	//开启线程往队列中喂食packet
	grabPacket.detach();
	//初始化sdl
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		string errMsg = "Could not initialize SDL -";
		errMsg += SDL_GetError();
		cout << errMsg << endl;
		throw std::runtime_error(errMsg);
	}
	std::thread audioThread{ startAudioBySDL };
	audioThread.join();
	std::thread videoThread{ startVideoBySDL };
	videoThread.join();
}