#include <iostream>
#include <string>

extern "C" {
#include <SDL2/SDL.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
};

using std::cout;
using std::endl;
using std::string;

void format_init(AVFormatContext*, const string, int*);

class Video {
  string path;
  int width = -1;
  int height = -1;

  AVFormatContext* pFormatCtx = nullptr;
  int videoIndex;
  //AVCodecContext* pCodecCtx = nullptr;
  AVCodec* pCodec = nullptr;
  AVCodecContext* codec_ctx = nullptr;
  AVPacket* pPacket = nullptr;
  AVFrame* pFrame = nullptr;
  AVFrame* pFrameYUV = nullptr;
  struct SwsContext* img_convert_ctx;
  unsigned char* out_buffer;

  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Window* sdl_window;

 public:
  Video(string filepath) {
    path = filepath;
    pFormatCtx = avformat_alloc_context();

    av_register_all();
    avformat_network_init();
    avcodec_register_all();

    if (avformat_open_input(&pFormatCtx, filepath.c_str(), NULL, NULL) != 0) {
      cout << "Couldn't open input stream" << endl;
      return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
      cout << "Couldn't find stream information" << endl;
      return;
    }
    videoIndex = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
      if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoIndex = i;
        break;
      }
    }

    if (videoIndex == -1) {
      cout << "Didn't find a video stream." << endl;
      return;
    }

    cout << "index: " << videoIndex << endl;
    if (pFormatCtx == nullptr) {
      cout << "��ʼ��ʧ��" << endl;
    }
    //AVCodecContext* pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
    AVCodec* pCodec =
        avcodec_find_decoder(pFormatCtx->streams[videoIndex]->codec->codec_id);

    if (pCodec == nullptr) {
      cout << "Codec not found" << endl;
      return;
    }

    codec_ctx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(
        codec_ctx,
        pFormatCtx->streams[videoIndex]->codecpar);  // ��������
    if (avcodec_open2(codec_ctx, pCodec, nullptr) < 0) {
      cout << "Could not open codec" << endl;
      return;
    }

    pPacket = av_packet_alloc();
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();

    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(
    AV_PIX_FMT_YUV420P, codec_ctx->width, codec_ctx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, codec_ctx->width,
                         codec_ctx->height, 1);

    cout << "��ʼ��ffmpeg���" << endl;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      cout << "SDL could not initialized with error: " << SDL_GetError()
           << endl;
      return;
    }
    // ��������
    sdl_window = SDL_CreateWindow(
        "FFmpeg+SDL������Ƶ-by superli", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    if (sdl_window == NULL) {
      cout << "SDL could not create window with error: " << SDL_GetError()
           << endl;
      return;
    }
    // �Ӵ��崴����Ⱦ��
    renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    // ������Ⱦ�������ҵ���Ƶ�����ʽ��SDL_PIXELFORMAT_IYUV�����Դ�FFmpeg̽�⵽��ʵ�ʽ��������ɫ��ʽ
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    cout << "��ʼ��SDL���" << endl;
  };

  ~Video() {
    // sws_freeContext(img_convert_ctx);
    SDL_Quit();
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_close(codec_ctx);
    avformat_close_input(&pFormatCtx);
  };

  void p() {
    int ret, got_pic;
    while (av_read_frame(pFormatCtx, pPacket) >= 0) {
      if (pPacket->stream_index == videoIndex) {
        ret = avcodec_decode_video2(codec_ctx, pFrame, &got_pic, pPacket);
        if (ret < 0) {
          printf("Decode Error.\n");
          return;
        }
        if (got_pic) {
          sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data,
                    pFrame->linesize, 0, codec_ctx->height, pFrameYUV->data,
                    pFrameYUV->linesize);

          // SDL---------------------------
          SDL_UpdateYUVTexture(texture, NULL, pFrameYUV->data[0],
                               pFrameYUV->linesize[0], pFrameYUV->data[1],
                               pFrameYUV->linesize[1], pFrameYUV->data[2],
                               pFrameYUV->linesize[2]);

          SDL_RenderClear(renderer);
          SDL_RenderCopy(renderer, texture, NULL, NULL);
          SDL_RenderPresent(renderer);
          // SDL End-----------------------
          // Delay 40ms
          SDL_Delay(40);
        }
      }
      av_free_packet(pPacket);
    }
    // flush decoder
    // FIX: Flush Frames remained in Codec
    while (1) {
      ret = avcodec_decode_video2(codec_ctx, pFrame, &got_pic, pPacket);
      if (ret < 0) break;
      if (!got_pic) break;
      sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data,
                pFrame->linesize, 0, codec_ctx->height, pFrameYUV->data,
                pFrameYUV->linesize);
      // SDL---------------------------
      SDL_UpdateTexture(texture, NULL, pFrameYUV->data[0],
                        pFrameYUV->linesize[0]);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
      // SDL End-----------------------
      // Delay 40ms
      SDL_Delay(40);
    }




    //while (true) {
    //  int result = av_read_frame(pFormatCtx, pPacket);
    //  if (result < 0) {
    //    cout << "end of file" << endl;
    //    //            avcodec_send_packet(codec_ctx_video, NULL); //
    //    //            TODO��һ�����֡�ղ�����������Ҫ��δ�����ý��
    //    break;
    //  }

    //  if (pPacket->stream_index != videoIndex) {  // Ŀǰֻ��ʾ��Ƶ����
    //    av_packet_unref(pPacket);  // ע��������������ڴ�й©
    //    continue;
    //  }
    //  // ���͵��������߳̽��룬 avPacket�ᱻ����һ�ݣ�����avPacket����ֱ�������
    //  result = avcodec_send_packet(codec_ctx, pPacket);
    //  av_packet_unref(pPacket);  // ע��������������ڴ�й©
    //  if (result != 0) {
    //    cout << "av_packet_unref failed" << endl;
    //    continue;
    //  }
    //  while (true) {  // ���ս���������, �������ں�̨�̣߳�packet ��
    //                  // frame������һһ��Ӧ�������֡��ֹ©����
    //    result = avcodec_receive_frame(codec_ctx, pFrame);
    //    if (result != 0) {  // ��֡ʧ�ܣ�˵�����ڻ�û�н���õ�֡���ݣ��˳���֡������
    //      break;
    //    }
    //    // ���ظ�ʽ�պ���YUV420P�ģ����������ظ�ʽת��
    //    cout << "avFrame pts : " << pFrame->pts << " color format:" << pFrame->format << endl;
    //    //            result = SDL_UpdateTexture(texture, NULL,
    //    //            avFrame->data[0], avFrame->linesize[0]); //
    //    //            ʹ�øú��������crash
    //    result = SDL_UpdateYUVTexture(texture, NULL, pFrame->data[0],
    //                                  pFrame->linesize[0], pFrame->data[1],
    //                                  pFrame->linesize[1], pFrame->data[2],
    //                                  pFrame->linesize[2]);
    //    if (result != 0) {
    //      cout << "SDL_UpdateTexture failed" << endl;
    //      continue;
    //    }
    //    SDL_RenderClear(renderer);
    //    SDL_RenderCopy(renderer, texture, NULL, NULL);
    //    SDL_RenderPresent(renderer);
    //    SDL_Delay(40);  // ��ֹ��ʾ���죬���Ҫʵ�ֱ��ٲ��ţ�ֻ��Ҫ����delayʱ��Ϳ����ˡ�
    //  }
    //}
  }

 private:
};