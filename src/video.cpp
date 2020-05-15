#include <iostream>
#include <thread>
#include <atomic>
#include "packetqueue.h"

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

#define REFRESH_EVENT (SDL_USEREVENT + 1)

#define BREAK_EVENT (SDL_USEREVENT + 2)

extern const int VIDEOTYPE = 1;
extern const int AUDIOTYPE = 2;

extern AVFormatContext* pFormatCtx;
extern AVCodecParameters* parser4video;
extern AVCodecContext* pCodecCtx4video;
extern int videoIndex;
extern AVFrame* frame4Video;
extern AVRational streamTimeBase4Video;
extern std::atomic<uint64_t> currentTimestamp4Audio;




void refreshPic(int timeInterval, bool& exitRefresh, bool& faster) {
	while (!exitRefresh) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		if (faster) {
			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval / 2));
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval));
		}
	}
	cout << "[THREAD] picRefresher thread finished." << endl;
}


void startVideoBySDL() {
	int screen_w, screen_h;
	SDL_Window* screen;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Thread* video_tid;
	SDL_Event event;

	screen_w = parser4video->width;
	screen_h = parser4video->height;

	screen = SDL_CreateWindow("myPplayer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h, SDL_WINDOW_OPENGL);
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING, parser4video->width,
		parser4video->height);

	bool faster = false;
	bool exitRefresh = false;
	cout << av_q2d(pCodecCtx4video->framerate) << endl;
	AVRational frame_rate =
		av_guess_frame_rate(pFormatCtx, pFormatCtx->streams[videoIndex], nullptr);
	cout << av_q2d(frame_rate) << endl;
	double frameRate = av_q2d(frame_rate);
	std::thread refreshThread{ refreshPic, (int)(1000 / frameRate), ref(exitRefresh), ref(faster) };

	AVPacket* packet;
	bool skip = false;
	int ret = -1;
	while (1) {
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT) {
			if (!skip) {
				packet = getPacketFromQueue(VIDEOTYPE);
				if (packet == nullptr) {
					cout << "error occured in get packet or finished" << endl;
				}

				ret = avcodec_send_packet(pCodecCtx4video, packet);
				if (ret == 0) {
					av_packet_free(&packet);
					packet = nullptr;
				}
				else if (ret == AVERROR(EAGAIN)) {
					// buff full, can not decode any more, nothing need to do.
					// keep the packet for next time decode.
				}
				else if (ret == AVERROR_EOF) {
					cout << "[WARN]  no new VIDEO packets can be sent to it." << endl;
				}
				else {
					string errorMsg = "+++++++++ ERROR avcodec_send_packet error: ";
					errorMsg += ret;
					cout << errorMsg << endl;
					throw std::runtime_error(errorMsg);
				}
				ret = avcodec_receive_frame(pCodecCtx4video, frame4Video);
			}
			if (ret == 0) {
				auto ats = currentTimestamp4Audio.load();
				auto vts = (uint64_t)(frame4Video->pts * av_q2d(streamTimeBase4Video) * 1000);
				if (vts > ats && vts - ats > 30) {
					skip = true;
					faster = false;
					continue;
				}
				else if (vts < ats && ats - vts>30) {
					skip = false;
					faster = true;
				}
				else {
					skip = false;
					faster = false;
				}
				SDL_UpdateYUVTexture(sdlTexture, NULL, frame4Video->data[0],
					frame4Video->linesize[0], frame4Video->data[1],
					frame4Video->linesize[1], frame4Video->data[2],
					frame4Video->linesize[2]);
				SDL_RenderClear(sdlRenderer);
				SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
				SDL_RenderPresent(sdlRenderer);
			}
			else if (ret == AVERROR_EOF) {
				cout << "+++++++++++++++++++++++++++++ MediaProcessor no more output frames. video" << endl;
			}
			else if (ret == AVERROR(EAGAIN)) {
				// need more packet.
			}
			else {
				string errorMsg = "avcodec_receive_frame error: ";
				errorMsg += ret;
				cout << errorMsg << endl;
				throw std::runtime_error(errorMsg);
			}
		}
		else if (event.type == SDL_WINDOWEVENT) {
			SDL_GetWindowSize(screen, &screen_w, &screen_h);
		}
		else if (event.type == SDL_QUIT) {
			exitRefresh = true;
			break;
		}
		else if (event.type == BREAK_EVENT) {
			break;
		}

	}


}