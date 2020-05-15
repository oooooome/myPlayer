#include <mutex>
#include <queue>
#include <atomic>
#include <iostream>
#include "packetqueue.h"
using namespace std;

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

std::mutex pkMutex4Video;		//队列同步锁
std::mutex pkMutex4Audio;
queue<AVPacket*> packetVideoQueue;
queue<AVPacket*> packetAudioQueue;

extern AVFormatContext* pFormatCtx;
extern int videoIndex, audioIndex;


void grabPacketToQueue() {
	while (true) {
		while (getAqSize() < MAXSIZE || getVqSize() < MAXSIZE) {
			AVPacket* packet = av_packet_alloc();
			int ret = av_read_frame(pFormatCtx, packet);
			if (ret < 0) {
				//TODO 文件结束的处理

				cout << "file finish or error" << endl;
				break;
			}
			else if (packet->stream_index == audioIndex) {
				std::lock_guard<std::mutex> lk(pkMutex4Audio);
				packetAudioQueue.push(packet);
			}
			else if (packet->stream_index == videoIndex) {
				std::lock_guard<std::mutex> lk(pkMutex4Video);
				packetVideoQueue.push(packet);
			}
			else {
				av_packet_free(&packet);
				cout << "WARN: unknown streamIndex: [" << packet->stream_index << "]" << endl;
			}
		}

	}
}

AVPacket* getPacketFromQueue(int type) {

	if (type == VIDEOTYPE) {
		std::lock_guard<std::mutex> lk(pkMutex4Video);
		if (packetVideoQueue.empty() || packetVideoQueue.front() == nullptr) {
			return nullptr;
		}
		else {
			AVPacket* front = packetVideoQueue.front();
			packetVideoQueue.pop();
			return front;
		}
	}
	else if (type == AUDIOTYPE) {
		std::lock_guard<std::mutex> lk(pkMutex4Audio);
		if (packetAudioQueue.empty() || packetAudioQueue.front() == nullptr) {
			return nullptr;
		}
		else {
			AVPacket* front = packetAudioQueue.front();
			packetAudioQueue.pop();
			return front;
		}
	}
}


int getAqSize() {
	std::lock_guard<std::mutex> lk(pkMutex4Audio);
	int size = packetAudioQueue.size();
	//	cout << "AqSize=" << size << endl;
	return size;
}

int getVqSize() {
	std::lock_guard<std::mutex> lk(pkMutex4Video);
	int size = packetVideoQueue.size();
	//	cout << "VqSize:" << size << endl;
	return size;
}