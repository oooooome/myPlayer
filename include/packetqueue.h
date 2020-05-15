extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
};


void grabPacketToQueue();

AVPacket* getPacketFromQueue(int type);

int getVqSize();

int getAqSize();

