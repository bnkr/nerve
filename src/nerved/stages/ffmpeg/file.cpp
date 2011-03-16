// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "file.hpp"

using namespace ffmpeg;

file::file(const char *const file) {
  // I need a get_last_error message function, or at least some kind of errno?.

  // no docs somewhere...
  // TODO: I will need to use this later on.
  AVInputFormat *const forced_format = NULL;
  // http://www.dranger.com/ffmpeg/data.html#AVFormatParameters
  AVFormatParameters *const parameters = NULL;
  // what is this buffer size for?
  const std::size_t buffer_size = 0;

  int ret = ::av_open_input_file(&format_, file, forced_format, buffer_size, parameters);
  if (ret != 0) {
    // TODO: why?  Have to do some of my own validation here I guess.
    throw file_error("couldn't open file");
  }
  assert(format_ != NULL);

  ret = ::av_find_stream_info(format_);
  // TODO:
  //   this fails whenever I open one of my .avi with error code 22.  I can't work
  //   out what this means, and since the code is exactly the same as the tutorial
  //   it's a bit of a mystery.
  if (ret != 0) {
    std::cerr << "error num: " <<  ret << " becomes " << strerror(ret) << std::endl;
    // TODO: say why.
    //
    switch (ret) {
      // case AVERROR_UNKNOWN: // duplicated AVERROR_INVALIDDATA
      // std::cout << "AVERROR_UNKNOWN" << std::endl; break;
    case AVERROR_IO:
      std::cout << "AVERROR_IO" << std::endl; break;
    case AVERROR_NUMEXPECTED:
      std::cout << "AVERROR_NUMEXPECTED" << std::endl; break;
    case AVERROR_INVALIDDATA:
      std::cout << "AVERROR_INVALIDDATA" << std::endl; break;
    case AVERROR_NOMEM:
      std::cout << "AVERROR_NOMEM" << std::endl; break;
    case AVERROR_NOFMT:
      std::cout << "AVERROR_NOFMT" << std::endl; break;
    case AVERROR_NOTSUPP:
      std::cout << "AVERROR_NOTSUPP" << std::endl; break;
    case AVERROR_NOENT:
      std::cout << "AVERROR_NOENT" << std::endl; break;
    case AVERROR_EOF:
      std::cout << "AVERROR_EOF" << std::endl; break;
    case AVERROR_PATCHWELCOME:
      std::cout << "AVERROR_PATCHWELCOME" << std::endl; break;
    default:
      std::cout << "unexpected error code" << std::endl; break;
    }

    throw stream_error("could not find codec parameters");
  }
}

void file::dump() const {
  // TODO: what are those otehr parameters for?
  const int is_output = 0;
  const int index = 0; // I think this is the stream index...
  ::dump_format(format_, index, file_name(), is_output);
}


human_duration_type file::human_durtion() const {
  human_duration_type d;
  d.secs = duration() / AV_TIME_BASE;
  const int us = duration() % AV_TIME_BASE;
  d.mins = d.secs / 60;
  d.secs %= 60;
  d.hours = d.mins / 60;
  d.mins %= 60;
  d.ms = (100 * us) / AV_TIME_BASE;
  return d;
}
