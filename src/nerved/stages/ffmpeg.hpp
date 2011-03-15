// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_FFMPEG_HPP_bt4bpj6o
#define STAGES_FFMPEG_HPP_bt4bpj6o

#include "interfaces.hpp"

#include "ffmpeg/file.hpp"
#include "ffmpeg/audio_stream.hpp"
#include "ffmpeg/audio_decoder.hpp"

namespace stages {
  namespace ff = ::fmpeg;

  class ffmpeg : public pipeline::input_stage {
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
    ff::file file_;
    ff::audio_stream input_;
    ff::audio_decoder decoder_;
    ff::packet frame_;
  };
}

#endif
