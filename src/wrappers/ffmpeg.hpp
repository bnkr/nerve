/*!
\file
\ingroup grp_ffmpeg
\brief C++ interface for ffmpeg audio decoding.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.

Also, the accessors to the ffmpeg structures are prefixed `av_', for example
file::av_format_context().  If you start using these structs you're probably
doing something the library hasn't be been designed for.

Ffmpeg is a pretty complicated library and this wrapper aims to simplify it as
well as add needed functionality like splitting the stream into packets for sdl
output.  In order to do this, the classes here wrap the ffmpeg structs and expose
a subset of their fields.  These are the ones you actually need, rather than
internal stuff.

This wrapper could hypothetically be expanded to cover video as well as audio,
but you would need to drop back to ffmpeg again for that.

TODO:
  Try and merge this with nerve.  It should be entirely possible once we have
  got rid of the packet state nonsense, and also the hacking gap removal stuff.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

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

#ifdef __GNUC__
#  define FF_ATTRIBUTE_DEPRECATED __attribute__((deprecated))
#else
#  define FF_ATTRIBUTE_DEPRECATED
#endif

#include "ffmpeg/aligned_memory.hpp"
#include "ffmpeg/data.hpp"
#include "ffmpeg/file.hpp"
#include "ffmpeg/audio_stream.hpp"
#include "ffmpeg/packet.hpp"
#include "ffmpeg/packet_reader.hpp"
#include "ffmpeg/audio_decoder.hpp"

//! \ingroup grp_ffmpeg
//! \brief All components of the ffmpeg wrapper.
namespace ffmpeg {}

#endif

