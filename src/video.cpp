#include <iostream>
#include <string>

#include "audio.h"

using std::cout;
using std::endl;
using std::string;

extern "C" {
#include <SDL2/SDL.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
};

void format_init(AVFormatContext* format_ctx, const string filepath,
                 int* videoIndex) {
  format_ctx = avformat_alloc_context();

  if (avformat_open_input(&format_ctx, filepath.c_str(), NULL, NULL) != 0) {
    cout << "Couldn't open input stream" << endl;
    return;
  }
  if (avformat_find_stream_info(format_ctx, NULL) < 0) {
    cout << "Couldn't find stream information" << endl;
    return;
  }
  *videoIndex = -1;
  for (int i = 0; i < format_ctx->nb_streams; i++) {
    if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      *videoIndex = i;
      break;
    }
  }

  if (*videoIndex == -1) {
    printf("Didn't find a video stream.\n");
    return;
  }
}
