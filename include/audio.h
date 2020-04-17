#include <string>
#include <stdlib.h>

using std::string;

extern "C" {
#include <SDL2/SDL.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

void audio_test(string);
void fill_audio(void *udata, Uint8 *stream, int len);