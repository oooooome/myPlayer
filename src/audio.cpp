#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include "packetqueue.h"
#include "video.h"
using namespace std;

extern "C" {
#include <SDL2/SDL.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
};


extern const int AUDIOTYPE;

extern AVFormatContext* pFormatCtx;
extern AVCodecParameters* para4audio;
extern AVCodecContext* pCodecCtx4audio;
extern AVFrame* frame4Audio;
extern AVRational streamTimeBase4Audio;
extern std::atomic<uint64_t> currentTimestamp4Audio;

extern int audio_samples;



struct AudioInfo {
	int64_t layout;
	int sampleRate;
	int channels;
	AVSampleFormat format;

	AudioInfo() {
		layout = -1;
		sampleRate = -1;
		channels = -1;
		format = AV_SAMPLE_FMT_S16;
	}

	AudioInfo(int64_t l, int rate, int c, AVSampleFormat f)
		: layout(l), sampleRate(rate), channels(c), format(f) {}
};

AudioInfo in;
AudioInfo out;


int allocDataBuf(AudioInfo in, AudioInfo out, uint8_t** outData, int inputSamples) {
	int bytePerOutSample = -1;
	switch (out.format) {
	case AV_SAMPLE_FMT_U8:
		bytePerOutSample = 1;
		break;
	case AV_SAMPLE_FMT_S16P:
	case AV_SAMPLE_FMT_S16:
		bytePerOutSample = 2;
		break;
	case AV_SAMPLE_FMT_S32:
	case AV_SAMPLE_FMT_S32P:
	case AV_SAMPLE_FMT_FLT:
	case AV_SAMPLE_FMT_FLTP:
		bytePerOutSample = 4;
		break;
	case AV_SAMPLE_FMT_DBL:
	case AV_SAMPLE_FMT_DBLP:
	case AV_SAMPLE_FMT_S64:
	case AV_SAMPLE_FMT_S64P:
		bytePerOutSample = 8;
		break;
	default:
		bytePerOutSample = 2;
		break;
	}

	int guessOutSamplesPerChannel = av_rescale_rnd(inputSamples, out.sampleRate, in.sampleRate, AV_ROUND_UP);
	int guessOutSize = guessOutSamplesPerChannel * out.channels * bytePerOutSample;

	guessOutSize *= 1.2; 

	*outData = (uint8_t*)av_malloc(sizeof(uint8_t) * guessOutSize);
	return guessOutSize;
}


tuple<int, int> reSample(uint8_t* dataBuffer, int dataBufferSize,
	const AVFrame* frame) {
	SwrContext* swr = swr_alloc_set_opts(nullptr, out.layout, out.format, out.sampleRate,
		in.layout, in.format, in.sampleRate, 0, nullptr);
	if (swr_init(swr)) {
		cout << "swr_init error." << endl;
		throw std::runtime_error("swr_init error.");
	}
	int outSamples = swr_convert(swr, &dataBuffer, dataBufferSize,
		(const uint8_t**)&frame->data[0], frame->nb_samples);
	if (outSamples <= 0) {
		throw std::runtime_error("error: outSamples=" + outSamples);
	}
	int outDataSize = av_samples_get_buffer_size(NULL, out.channels, outSamples, out.format, 1);

	if (outDataSize <= 0) {
		throw std::runtime_error("error: outDataSize=" + outDataSize);
	}
	return { outSamples, outDataSize };
}

void audio_callback(void* userdata, Uint8* stream, int len) {

	AVPacket* packet;
	while (true) {

		packet = getPacketFromQueue(AUDIOTYPE);
		if (packet == nullptr) {
			cout << "audio finished or get some error" << endl;
		}
		int ret = -1;
		ret = avcodec_send_packet(pCodecCtx4audio, packet);
		if (ret == 0) {
			av_packet_free(&packet);
			packet = nullptr;
		}
		else if (ret == AVERROR(EAGAIN)) {
			// buff full, can not decode any more, nothing need to do.
			// keep the packet for next time decode.
		}
		else if (ret == AVERROR_EOF) {
			cout << "[WARN]  no new AUDIO packets can be sent to it." << endl;
		}
		else {
			string errorMsg = "+++++++++ ERROR avcodec_send_packet error: ";
			errorMsg += ret;
			cout << errorMsg << endl;
			cout << ret << endl;
			throw std::runtime_error(errorMsg);
		}

		ret = avcodec_receive_frame(pCodecCtx4audio, frame4Audio);
		if (ret >= 0) {
			break;
		}
		else if (ret == AVERROR(EAGAIN)) {
			continue;
		}
		else {
			cout << "can't get frame" << endl;
			throw std::runtime_error("can't get frame");
		}
	}
	auto t = frame4Audio->pts * av_q2d(streamTimeBase4Audio) * 1000;
	currentTimestamp4Audio.store((uint64_t)t);
	static uint8_t* outBuffer = nullptr;
	static int outBufferSize = 0;

	if (outBuffer == nullptr) {
		outBufferSize = allocDataBuf(in, out, &outBuffer, frame4Audio->nb_samples);
	}
	else {
		memset(outBuffer, 0, outBufferSize);
	}

	int outSamples;
	int outDataSize;
	std::tie(outSamples, outDataSize) =
		reSample(outBuffer, outBufferSize, frame4Audio);
	audio_samples = outSamples;
	if (outDataSize != len) {
		cout << "WARNING: outDataSize[" << outDataSize << "] != len[" << len << "]" << endl;
	}

	std::memcpy(stream, outBuffer, outDataSize);
	av_freep(&outBuffer);
	outBuffer = nullptr;
}

void getsamples() {
	AVPacket* packet;
	while (true) {

		packet = getPacketFromQueue(AUDIOTYPE);
		if (packet == nullptr) {
			cout << "audio finished or get some error" << endl;
		}
		int ret = -1;
		ret = avcodec_send_packet(pCodecCtx4audio, packet);
		if (ret == 0) {
			av_packet_free(&packet);
			packet = nullptr;
		}
		else if (ret == AVERROR(EAGAIN)) {
			// buff full, can not decode any more, nothing need to do.
			// keep the packet for next time decode.
		}
		else if (ret == AVERROR_EOF) {
			cout << "[WARN]  no new AUDIO packets can be sent to it." << endl;
		}
		else {
			string errorMsg = "+++++++++ ERROR avcodec_send_packet error: ";
			errorMsg += ret;
			cout << errorMsg << endl;
			cout << ret << endl;
			throw std::runtime_error(errorMsg);
		}

		ret = avcodec_receive_frame(pCodecCtx4audio, frame4Audio);
		if (ret >= 0) {
			break;
		}
		else if (ret == AVERROR(EAGAIN)) {
			continue;
		}
		else {
			cout << "can't get frame" << endl;
			throw std::runtime_error("can't get frame");
		}
	}

	static uint8_t* outBuffer = nullptr;
	static int outBufferSize = 0;

	if (outBuffer == nullptr) {
		outBufferSize = allocDataBuf(in, out, &outBuffer, frame4Audio->nb_samples);
	}
	else {
		memset(outBuffer, 0, outBufferSize);
	}

	int outSamples;
	int outDataSize;
	std::tie(outSamples, outDataSize) =
		reSample(outBuffer, outBufferSize, frame4Audio);
	audio_samples = outSamples;
}

void startAudioBySDL() {
	SDL_AudioSpec wanted_spec;
	SDL_AudioSpec specs;


	int64_t inLayout = para4audio->channel_layout;
	int inChannels = para4audio->channels;
	int inSampleRate = para4audio->sample_rate;
	AVSampleFormat inFormate = AVSampleFormat(pCodecCtx4audio->sample_fmt);
	in = AudioInfo(inLayout, inSampleRate, inChannels, inFormate);
	out = AudioInfo(AV_CH_LAYOUT_STEREO, inSampleRate, 2, AV_SAMPLE_FMT_S16);
	while (audio_samples <= 0) {
		getsamples();
	}
	wanted_spec.freq = para4audio->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = para4audio->channels;
	wanted_spec.samples = audio_samples;  // set by output samples
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = nullptr;
	SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);

	SDL_AudioDeviceID audioDeviceId =
		SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, &specs, 0);  //[1]
	if (audioDeviceId == 0) {
		cout << "Failed to open audio device:" << SDL_GetError() << endl;
	}
	cout << "wanted_specs.freq:" << wanted_spec.freq << endl;
	// cout << "wanted_specs.format:" << wanted_specs.format << endl;
	std::cout << "wanted_specs.format: " << wanted_spec.format << endl;
	cout << "wanted_specs.channels:" << (int)wanted_spec.channels << endl;
	cout << "wanted_specs.samples:" << (int)wanted_spec.samples << endl;

	cout << "------------------------------------------------" << endl;
	cout << "specs.freq:" << specs.freq << endl;
	// cout << "specs.format:" << specs.format << endl;
	std::printf("specs.format: Ox%X\n", specs.format);
	cout << "specs.channels:" << (int)specs.channels << endl;
	cout << "specs.silence:" << (int)specs.silence << endl;
	cout << "specs.samples:" << (int)specs.samples << endl;

	//cout << "waiting audio play..." << endl;

	SDL_PauseAudioDevice(audioDeviceId, 0);  // [2]

}