/*!
\file
\brief C++ helpers for ffmpeg.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.

Ffmpeg is a pretty complicated library and this wrapper aims to simplify it as
well as add needed functionality like splitting the stream into packets for sdl
output.  In order to do this, the classes here wrap the ffmpeg structs and expose
a subset of their fields.  These are the ones you actually need, rather than
internal stuff.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

#include <iostream>
#include <cstring>
#include <cstdlib>

#ifdef __GNUC__
#define FF_ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#define FF_ATTRIBUTE_DEPRECATED
#endif

// FFmpeg developers hate C++, and not without good reason!  But then I have
// always been a masochist...
extern "C" {
#ifdef HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
#  include <ffmpeg/libavcodec/avcodec.h>
// Assume this to get a easier to diagnose error message.
#else /*if defined(HAVE_FFMPEG_AVCODEC_H)*/
#  include <ffmpeg/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
#  include <ffmpeg/libavformat/avformat.h>
#else /*if defined(HAVE_FFMPEG_AVFORMAT_H)*/
#  include <ffmpeg/avformat.h>
#endif
}

#include "../include/aligned_memory.hpp"

#include "ffmpeg/data.hpp"
#include "ffmpeg/file.hpp"
#include "ffmpeg/audio_stream.hpp"
#include "ffmpeg/packet.hpp"
#include "ffmpeg/packet_reader.hpp"

namespace ffmpeg {
  typedef ffmpeg::packet frame;
}

#include "ffmpeg/packet_state.hpp"
#include "ffmpeg/audio_decoder.hpp"

namespace ffmpeg { }

#endif

