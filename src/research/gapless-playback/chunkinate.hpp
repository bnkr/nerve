#ifndef CHUNKINATE_HPP_6kzvs88q
#define CHUNKINATE_HPP_6kzvs88q

#include <cstdio>

namespace ffmpeg {
  class packet_state;
}

void chunkinate_file(ffmpeg::packet_state &, const char *, bool);
void chunkinate_finish(ffmpeg::packet_state &, bool);

#endif
