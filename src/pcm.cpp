#include "pcm.h"
#include "definitions.h"

#include <iostream>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

bool file_to_pcm(const char* file, int stream_index, PCM_QUEUE* queue, error_callback* error) {
  av_log_set_level(AV_LOG_QUIET);
  
  avformat_network_init();

  AVFormatContext* format_ctx = nullptr;
  AVCodecContext* codec_ctx = nullptr;
  const AVCodec* codec = nullptr;
  AVPacket packet;
  AVFrame* frame = nullptr;
  SwrContext* swr_ctx = nullptr;

  // int audio_stream_index = -1;
  int ret = 0;

  if (avformat_open_input(&format_ctx, file, nullptr, nullptr) != 0) {
    error("Failed to open input file");  
    return false;
  }

  if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
    error("Failed to find stream info");
    return false;
  }

  if (stream_index != -1) {
    if (stream_index >= format_ctx->nb_streams) {
      error("Invalid stream index");
      return false;
    }
    if (format_ctx->streams[stream_index]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
      error("Stream is not audio");
      return false;
    }
  }

  for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      stream_index = i;
      break;
    }
  }

  if (stream_index == -1) {
    error("Failed to find audio stream");
    return false;
  }

  codec_ctx = avcodec_alloc_context3(nullptr);
  if (!codec_ctx) {
    error("Failed to allocate codec context");
    return false;
  }

  if (avcodec_parameters_to_context(codec_ctx, format_ctx->streams[stream_index]->codecpar) < 0) {
    error("Failed to copy codec parameters to codec context");
    return false;
  }

  codec = avcodec_find_decoder(codec_ctx->codec_id);
  if (!codec) {
    error("Failed to find decoder");
    return false;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    error("Failed to open codec");
    return false;
  }

  frame = av_frame_alloc();
  if (!frame) {
    error("Failed to allocate frame");
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
    error("Failed to allocate SwrContext");
    return false;
  }

  if (swr_init(swr_ctx) < 0) {
    error("Failed to initialize SwrContext");
    return false;
  }

  while (av_read_frame(format_ctx, &packet) >= 0) {
    if (packet.stream_index == stream_index) {
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
          error("Error while decoding");
          return false;
        }

        uint8_t* data = nullptr;
        if (av_samples_alloc(&data,
                              nullptr,
                              1, // mono
                              frame->nb_samples,
                              AV_SAMPLE_FMT_S16,
                              0) < 0) {
          error("Failed to allocate samples");
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