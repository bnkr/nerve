// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "ffmpeg.hpp"
#include <cstdlib>

using stages::ffmpeg;

void ffmpeg::abandon() {
}

void ffmpeg::flush() {
}

void ffmpeg::finish() {
}

void ffmpeg::configure(const char *k, const char *v) {
}

void ffmpeg::skip(skip_type) {
}

void ffmpeg::load(load_type) {
}

void ffmpeg::pause() {
}

pipeline::packet *ffmpeg::read() {
  return NULL;
}

#if 0
    ffmpeg::initialiser ff;

  ffmpeg::packet pkt;
  ffmpeg::file file(file_name);
  // file.dump(); //file_name);
  ffmpeg::audio_stream audio(file);

  ffmpeg::packet_reader pr(pkt, file);
  while (pr.read()) {
    // TODO:
    //   bug - if you drop all frames, then we wait for the exit signal forever.  Fixed now?

    ffmpeg::audio_decoder decoder(audio);
    ffmpeg::decoded_audio decoded(decoder, pkt);

    push_packet(decoded);
  }

#endif

