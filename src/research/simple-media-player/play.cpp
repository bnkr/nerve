#include <nerved_config.hpp>

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

#include <iostream>

#include "play.hpp"

/*
TODO:
  This will be based on the following tutorial with ffmpeg and sdl:

    http://www.dranger.com/ffmpeg/tutorial03.html

  Note that seeking gets its own chapter!

    http://www.dranger.com/ffmpeg/tutorial07.html

  See also:

    find -name \*example\* -not -name \*svn\*

  in the ffmpeg svn thingy.
*/

void play(const char * const file) {
  std::cout << "playing: " << file << std::endl;
}
