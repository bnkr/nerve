#ifndef READ_PACKETS_HPP_iu6o9luw
#define READ_PACKETS_HPP_iu6o9luw

#include "ffmpeg.hpp"
#include "sdl.hpp"

void read_packets(ffmpeg::file &file, ffmpeg::audio_stream &s, const sdl::audio_spec &);

#endif
