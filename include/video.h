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

  //ffmpeg-----------------------------------------------------------
  ////AVFormatContext* pFormatCtx = nullptr;
  //int videoIndex;
  ////AVCodecContext* pCodecCtx = nullptr;
  //AVCodec* pCodec = nullptr;
  //AVCodecContext* codec_ctx = nullptr;
  //AVPacket* pPacket = nullptr;
  //AVFrame* pFrame = nullptr;
  //AVFrame* pFrameYUV = nullptr;
  //struct SwsContext* img_convert_ctx;
  //unsigned char* out_buffer;

  AVFormatContext* pFormatCtx;
  int videoIndex;
  AVCodecContext* pCodecCtx;
  AVCodec* pCodec;
  AVFrame *pFrame, *pFrameYUV;
  unsigned char* out_buffer;
  AVPacket* pPacket;
  int y_size;
  int ret, got_picture;
  struct SwsContext* img_convert_ctx;
  AVRational frame_rate;

  //SDL-----------------------------------------------------------------
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  SDL_Window* window;

 public:
  Video(string filepath) {
    path = filepath;

    av_register_all();
     avformat_network_init();
     pFormatCtx = avformat_alloc_context();

     if (avformat_open_input(&pFormatCtx, filepath.c_str(), NULL, NULL) != 0) {
      printf("Couldn't open input stream.\n");
      return;
    }
     if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
      printf("Couldn't find stream information.\n");
      return;
    }
     videoIndex = -1;
     for (int i = 0; i < pFormatCtx->nb_streams; i++)
      if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoIndex = i;
        break;
      }
     if (videoIndex == -1) {
      printf("Didn't find a video stream.\n");
      return;
    }

     pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
     pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
     if (pCodec == NULL) {
      printf("Codec not found.\n");
      return;
    }

     width = pCodecCtx->width;
    height = pCodecCtx->height;

     if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
      printf("Could not open codec.\n");
      return;
    }

      frame_rate.den == -1;
     frame_rate.num == -1;
     frame_rate = pCodecCtx->framerate;
     if (frame_rate.den == -1 || frame_rate.num == -1) {
      cout << "get frame rate failed" << endl;
    }

     pFrame = av_frame_alloc();
     pFrameYUV = av_frame_alloc();
     out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(
        AV_PIX_FMT_YUV420P, width, height, 1));
     av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, width,
                         height, 1);

     pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    // Output Info-----------------------------
     printf("--------------- File Information ----------------\n");
     av_dump_format(pFormatCtx, 0, filepath.c_str(), 0);
     printf("-------------------------------------------------\n");
     img_convert_ctx =
        sws_getContext(width, height,
        pCodecCtx->pix_fmt,
                       width, height,
                       AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

     if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
      printf("Could not initialize SDL - %s\n", SDL_GetError());
      return;
    }

    // SDL 2.0 Support for multiple windows
    window = SDL_CreateWindow("myPlayer",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, width, height,
                              SDL_WINDOW_OPENGL);

     if (!window) {
      printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
      return;
    }

     renderer = SDL_CreateRenderer(window, -1, 0);
    // IYUV: Y + U + V  (3 planes)
    // YV12: Y + V + U  (3 planes)
     texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width, height);











    //av_register_all();
    //avformat_network_init();
    //avcodec_register_all();
    //pFormatCtx = avformat_alloc_context();

    //if (avformat_open_input(&pFormatCtx, filepath.c_str(), NULL, NULL) != 0) {
    //  cout << "Couldn't open input stream" << endl;
    //  return;
    //}
    //if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    //  cout << "Couldn't find stream information" << endl;
    //  return;
    //}
    //videoIndex = -1;
    //for (int i = 0; i < pFormatCtx->nb_streams; i++) {
    //  if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
    //    videoIndex = i;
    //    break;
    //  }
    //}

    //if (videoIndex == -1) {
    //  cout << "Didn't find a video stream." << endl;
    //  return;
    //}

    //if (pFormatCtx == nullptr) {
    //  cout << "初始化失败" << endl;
    //}
    //pCodecCtx = pFormatCtx->streams[videoIndex]->codec;
    //pCodec = avcodec_find_decoder(pFormatCtx->streams[videoIndex]->codec->codec_id);

    //if (pCodec == nullptr) {
    //  cout << "Codec not found" << endl;
    //  return;
    //}

    //if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    //  cout << "Could not open codec" << endl;
    //  return;
    //}

    //frame_rate.den == -1;
    //frame_rate.num == -1;
    //frame_rate = pCodecCtx->framerate;
    //if (frame_rate.den == -1 || frame_rate.num == -1) {
    //  cout << "get frame rate failed" << endl;
    //}

    //pPacket = av_packet_alloc();
    //pFrame = av_frame_alloc();
    //pFrameYUV = av_frame_alloc();
    //out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(
    //    AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    //av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
    //                     AV_PIX_FMT_YUV420P, pCodecCtx->width,
    //                     pCodecCtx->height, 1);

    //cout << "初始化ffmpeg完毕" << endl;

    //if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    //  cout << "SDL could not initialized with error: " << SDL_GetError()
    //       << endl;
    //  return;
    //}
    //// 创建窗体
    //sdl_window = SDL_CreateWindow(
    //    "FFmpeg+SDL播放视频-by superli", SDL_WINDOWPOS_UNDEFINED,
    //    SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
    //if (sdl_window == NULL) {
    //  cout << "SDL could not create window with error: " << SDL_GetError()
    //       << endl;
    //  return;
    //}
    //// 从窗体创建渲染器
    //renderer = SDL_CreateRenderer(sdl_window, -1, 0);
    //// 创建渲染器纹理，我的视频编码格式是SDL_PIXELFORMAT_IYUV，可以从FFmpeg探测到的实际结果设置颜色格式
    //texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
    //cout << "初始化SDL完毕" << endl;
  };


  ~Video() {
    // sws_freeContext(img_convert_ctx);
    SDL_Quit();
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
  };

  void p() {
    while (av_read_frame(pFormatCtx, pPacket) >= 0) {
      if (pPacket->stream_index == videoIndex) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, pPacket);
        if (ret < 0) {
          printf("Decode Error.\n");
          return;
        }
        if (got_picture) {
          sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
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
          SDL_Delay(1000/av_q2d(frame_rate));
        }
      }
      av_free_packet(pPacket);
    }
  }

 private:
};