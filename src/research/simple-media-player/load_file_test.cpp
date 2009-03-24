#include "ffmpeg.hpp"

#include <iostream>

typedef void (*log_callback_type)(void*, int, const char*, va_list);
void test_loading_log_cb(void *ptr, int level, const char *fmt, va_list args) {
  AVClass *avc = ptr ? *(AVClass**) ptr : NULL;
  // need to sprintf into a string... can I use fprintf of an iostream?  Or do my
  // own parse?
  // ptr is used to get the name of the decode module.
  // last_log.reset();
}

void load_file_test(const char *file) {
  // http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/avformat_8h.html
  // http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/avcodec_8h.html
  // http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html

  av_log_set_level(AV_LOG_DEBUG);

  av_register_all();
  AVFormatContext *format_ctx = NULL;

  // I need a get_last_error message function, or at least some kind of errno?.
  //av_log_set_callback(test_loading_log_cb);

  // bleh... must be via malloc it seems.
  if (av_open_input_file(&format_ctx, file, NULL, 0, NULL) != 0) {
    // TODO: what should the error return be?   I want the error message.
    std::cerr << "couldnae open file" << std::endl;
    goto errors;
  }

  if (av_find_stream_info(format_ctx) != 0) {
    std::cerr << "av_find_stream_info" << std::endl;
    goto errors;
  }

  dump_format(format_ctx, 0, file, 0);

  /// TODO: move through the file but just chuck away the data

  std::cout << "Yay!" << std::endl;
  return;

errors:
  std::cerr << "Phail." << std::endl;
  exit(EXIT_FAILURE);
}

