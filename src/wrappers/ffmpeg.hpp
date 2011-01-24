/*!
\defgroup grp_ffmpeg FFmpeg Wrapper

\section s_grp_ffmpeg_intro Introduction

This module wraps the audio interface of ffmpeg into some friendly C++ classes.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.

Also, the accessors to the ffmpeg structures are prefixed `av_', for example
file::av_format_context().  If you start using these structs you're probably
doing something the library hasn't be been designed for.

FFmpeg is a pretty complicated library and this wrapper aims to simplify it as
well as add needed functionality like splitting the stream into packets for sdl
output.  In order to do this, the classes here wrap the ffmpeg structs and expose
a subset of their fields.  These are the ones you actually need, rather than
internal stuff.

This wrapper could hypothetically be expanded to cover video as well as audio,
but you would need to drop back to ffmpeg again for that.

\section s_grp_ffmpeg_example Example

Here is a rather approximate example:

\code
extern const char *file_name;
extern void do_something(const ffmpeg::decoded_audio &f);

// ...

try {
  ffmpeg::initialiser init;

  ffmpeg::file file(file_name);
  ffmpeg::audio_stream audio_stream(file);

  ffmpeg::decode_audio(audio_stream, do_something);
}
catch (ffmpeg::error &e) {
  // ...
}
\endcode

Look at ffmpeg::decode_audio() for some more low-level stuff.  You can go a bit
lower than that by manually reading the packets from an ffmpeg::file.
*/

/*!
\file
\ingroup grp_ffmpeg
\brief C++ interface for ffmpeg audio decoding.
*/

#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

extern "C" {

// ffnpeg can actually be configured to avoid using this, but the particular
// headers I have don't.
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

// These will probably be wrong, but they move around so much between versions
// that it's better to organise it with -I flags (or at worse symlinks).
#include <avcodec.h>
#include <avformat.h>
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

