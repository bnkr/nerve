#ifndef CHUNKINATE_HPP_6kzvs88q
#define CHUNKINATE_HPP_6kzvs88q

#include <cstdio>

class packet_state;

void chunkinate_file(packet_state &, const char *, bool);
void chunkinate_finish(packet_state &, bool);

#endif
