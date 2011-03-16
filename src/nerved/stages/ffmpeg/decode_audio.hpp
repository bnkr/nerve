// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_FFMPEG_DECODE_AUDIO_HPP_l3499fbm
#define STAGES_FFMPEG_DECODE_AUDIO_HPP_l3499fbm

namespace ffmpeg {
  class audio_stream;
  class frame;

  //! \ingroup grp_ffmpeg
  //! Basically just a specially aligned buffer.  Used with decoding.
  class sample_buffer {
    public:
  };

  //! \ingroup grp_ffmpeg
  //! A simple convenience structure which puts an audio stream with a
  //! frame read from a file.
  class audio_source {
    public:

    // TODO:
    //   Constness might not be right.

    audio_source(const frame &f, audio_stream &s)
    : stream_(s), frame_(f) { }

    private:
    audio_stream &stream_;
    const frame &frame_;
  };

  //! \ingroup grp_ffmpeg
  //! Decode from +source+ into +dest+.
  void decode_audio(sample_buffer &dest, audio_source &source);
}
#endif
