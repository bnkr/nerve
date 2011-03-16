// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_FFMPEG_INPUT_HPP_bt4bpj6o
#define STAGES_FFMPEG_INPUT_HPP_bt4bpj6o

#include "interfaces.hpp"

#include "ffmpeg/file.hpp"
#include "ffmpeg/audio_stream.hpp"
#include "ffmpeg/decode_audio.hpp"
#include "ffmpeg/frame.hpp"

namespace stages {
  class ffmpeg_input : public pipeline::input_stage {
    public:
    typedef pipeline::input_stage::skip_type skip_type;
    typedef pipeline::input_stage::load_type load_type;

    void abandon();
    void flush();
    void finish();
    void configure(const char *k, const char *v);
    void skip(skip_type);
    void load(load_type);
    void pause();
    pipeline::packet *read();

    private:
    ::ffmpeg::file file_;
    ::ffmpeg::audio_stream stream_;
    ::ffmpeg::frame frame_;
    ::ffmpeg::sample_buffer buffer_;
  };
}

#endif
