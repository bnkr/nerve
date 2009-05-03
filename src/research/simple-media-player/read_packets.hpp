#ifndef READ_PACKETS_HPP_iu6o9luw
#define READ_PACKETS_HPP_iu6o9luw

#include "../../wrappers/ffmpeg.hpp"
#include "../../wrappers/sdl.hpp"

void read_packets(ffmpeg::file &file, ffmpeg::audio_stream &s, const sdl::audio_spec &);

#endif
