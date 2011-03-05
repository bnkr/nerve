// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "progressive_buffer.hpp"

using namespace pipeline;

void progressive_buffer::step() {
  packet *input;
  iterator_type real_start = start_;

  if (this->buffering()) {
    ++real_start;
    input = this->debuffer_input();
  }
  else {
    input = this->read_input();
  }

  const iterator_type end = stages().end();

  for (iterator_type s = real_start; s != end; ++s) {
    NERVE_ASSERT(! input->non_data(), "can't be doing a data loop on non-data");
    const stage_value_type ret = s->process(input);

    NERVE_ASSERT(! (ret.empty() && ret.buffering()), "empty xor buffering");
    if (ret.empty()) {
      return;
    }
    else if (ret.buffering()) {
      // Simply assigning this every time we meet a buffering stage means we
      // end up debuffering the latest stage which has stuff to debuffer.
      //
      // TODO:
      //   This means we ignore any stage earlier in the sequence which is
      //   buffering and therefore buffers expand when we push more data!  We
      //   must use a stack to reduce our bufferingness.
      start(s);
    }

    input = ret.packet();
  }

  // It is nicer to do the connection work here because otherwise the section
  // or sequence have to keep checking to see whether we returned anything.
  this->write_output(input);
}

packet *progressive_buffer::debuffer_input() {
  const stage_value_type ret = this->start()->debuffer();

  NERVE_ASSERT(! ret.empty(), "empty data from a buffering stage is forbidden");

  if (! ret.buffering()) {
    // TODO: pop iterator from stack
    reset_start();
  }

  return ret.packet();
}
