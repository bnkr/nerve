// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "ffmpeg_input.hpp"

#include "../util/asserts.hpp"

#include <cstdlib>

namespace ff = ::ffmpeg;
using stages::ffmpeg_input;

// TODO:
//   We need some kind of "initialise" function other than the constructor so
//   that it can fail properly.

void ffmpeg_input::abandon() {
}

void ffmpeg_input::flush() {
}

void ffmpeg_input::finish() {
}

void ffmpeg_input::configure(const char *k, const char *v) {
  // TODO:
  //   This is another one which needs errors reported.
}

void ffmpeg_input::skip(skip_type) {
}

void ffmpeg_input::load(load_type loc) {
  ff::file new_file;
  if (! new_file.open(loc)) {
    // TODO:
    //   how is an error reported?  We need to generate a new load event
    //   somehow.  Also a proper useful error string would be very good.
    return;
  }

  ff::audio_stream new_stream;

  if (! new_stream.open(new_file)) {
    // report error
    return;
  }

  // So we don't have to worry about memory so much.
  file_.swap(new_file);
  stream_.swap(new_stream);
}

pipeline::packet *ffmpeg_input::read() {
  ff::read_frame(frame_, file_);
  ff::audio_source source(frame_, stream_);
  ff::decode_audio(buffer_, source);

  // TODO:
  //   Set up the pipeline::packet.  Note: we want the option to later separate
  //   out he decoder and the file reader, so I'll need the ability to put
  //   whatever special stuff necessary on to actual resultant packet.


  NERVE_NIMPL("returning data from a whatsit");
  return NULL;
}

void ffmpeg_input::pause() {
}
