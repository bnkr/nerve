// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_FFMPEG_AUDIO_STREAM_HPP_svxdkfa8
#define STAGES_FFMPEG_AUDIO_STREAM_HPP_svxdkfa8

namespace ffmpeg {
  struct file;

  //! \ingroup grp_ffmpeg
  //! Reference to an audio stream in a given file.
  class audio_stream {
    public:
    bool open(ffmpeg::file &f);
    void swap(audio_stream &other);
  };
}

#endif
