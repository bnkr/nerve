// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "ffmpeg.hpp"

#include "../util/asserts.hpp"

#include <cstdlib>

using stages::ffmpeg;

// TODO:
//   We need some kind of "initialise" function other than the constructor so
//   that it can fail properly.

void ffmpeg::abandon() {
}

void ffmpeg::flush() {
}

void ffmpeg::finish() {
}

void ffmpeg::configure(const char *k, const char *v) {
  // TODO:
  //   This is another one which needs errors reported.
}

void ffmpeg::skip(skip_type) {
}

void ffmpeg::load(load_type loc) {
  ff::file new_file;
  if (! new_file.open(loc)) {
    // TODO:
    //   how is an error reported?  We need to generate a new load event
    //   somehow.  Also a proper useful error string would be very good.
    return;
  }

  ff::audio_stream new_stream;

  if (! new_stream.open(file)) {
    // report error
    return;
  }

  // So we don't have to worry about memory so much.
  file_.swap(new_file);
  stream_.swap(new_stream);
  decoder_.stream(&stream_);
}

pipeline::packet *ffmpeg::read() {
  // could be neater in free functions?
  //
  //   ff::read_frame(frame_, file_);
  //   ff::decode_frame(buffer_, frame_, stream_);
  file_.read_frame(frame_);
  decoder_.decode(frame_);

  // TODO:
  //   Set up the pipeline::packet.  Note: we want the option to later separate
  //   out he decoder and the file reader, so I'll need the ability to put
  //   whatever special stuff necessary on to actual resultant packet.
  decoder_.buffer();


  NERVE_NIMPL("returning data from a whatsit");
  return NULL;
}

void ffmpeg::pause() {
}

#endif

