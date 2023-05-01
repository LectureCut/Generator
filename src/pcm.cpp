#include "pcm.h"
#include "definitions.h"

#include <iostream>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

bool file_to_pcm(const char* file, PCM_QUEUE* queue) {
  av_log_set_level(AV_LOG_QUIET);
  
  avformat_network_init();

  AVFormatContext* format_ctx = nullptr;
  AVCodecContext* codec_ctx = nullptr;
  const AVCodec* codec = nullptr;
  AVPacket packet;
  AVFrame* frame = nullptr;
  SwrContext* swr_ctx = nullptr;

  int audio_stream_index = -1;
  int ret = 0;

  if (avformat_open_input(&format_ctx, file, nullptr, nullptr) != 0) {
    std::cout << "Failed to open input file" << std::endl;  
    return false;
  }

  if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
    std::cout << "Failed to find stream info" << std::endl;
    return false;
  }

  for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
      break;
    }
  }

  if (audio_stream_index == -1) {
    std::cout << "Failed to find audio stream" << std::endl;
    return false;
  }

  codec_ctx = avcodec_alloc_context3(nullptr);
  if (!codec_ctx) {
    std::cout << "Failed to allocate codec context" << std::endl;
    return false;
  }

  if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar) < 0) {
    std::cout << "Failed to copy codec parameters to codec context" << std::endl;
    return false;
  }

  codec = avcodec_find_decoder(codec_ctx->codec_id);
  if (!codec) {
    std::cout << "Failed to find decoder" << std::endl;
    return false;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    std::cout << "Failed to open codec" << std::endl;
    return false;
  }

  frame = av_frame_alloc();
  if (!frame) {
    std::cout << "Failed to allocate frame" << std::endl;
    return false;
  }

  AVChannelLayout mono = AVChannelLayout AV_CHANNEL_LAYOUT_MONO;

  int res = swr_alloc_set_opts2(&swr_ctx,
                                &mono,
                                AV_SAMPLE_FMT_S16,
                                16000,
                                &codec_ctx->ch_layout,
                                codec_ctx->sample_fmt,
                                codec_ctx->sample_rate,
                                0,
                                nullptr);

  if (!swr_ctx) {
    std::cout << "Failed to allocate SwrContext" << std::endl;
    return false;
  }

  if (swr_init(swr_ctx) < 0) {
    std::cout << "Failed to initialize SwrContext" << std::endl;
    return false;
  }

  while (av_read_frame(format_ctx, &packet) >= 0) {
    if (packet.stream_index == audio_stream_index) {
      ret = avcodec_send_packet(codec_ctx, &packet);
      if (ret < 0) {
        break; // Error or end of stream.
      }

      while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN)) { // Need more data.
          break;
        } else if (ret == AVERROR_EOF) { // End of stream.
          break;
        } else if (ret < 0) { // Error.
          std::cout << "Error while decoding" << std::endl;
          return false;
        }

        uint8_t* data = nullptr;
        if (av_samples_alloc(&data,
                              nullptr,
                              1, // mono
                              frame->nb_samples,
                              AV_SAMPLE_FMT_S16,
                              0) < 0) {
          std::cout << "Failed to allocate samples" << std::endl;
          return false;
        }
        int num_samples = swr_convert(swr_ctx,
                                      &data,
                                      frame->nb_samples,
                                      (const uint8_t**)frame->data,
                                      frame->nb_samples);

        if (num_samples > 0) {
          // size_t num_bytes = num_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
          int16_t *ptr = (int16_t*)data;
          // audio_data.insert(audio_data.end(), ptr, ptr + num_bytes / sizeof(int16_t));
          queue->push(ptr, num_samples);
        }
      }
    }

    av_packet_unref(&packet);
  }

  queue->set_done();

  return true;
}