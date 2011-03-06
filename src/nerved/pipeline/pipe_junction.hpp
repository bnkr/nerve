// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PIPE_JUNCTION_HPP_ub435bcj
#define PIPELINE_PIPE_JUNCTION_HPP_ub435bcj

#include "../util/asserts.hpp"

namespace pipeline {
  //! \ingroup grp_pipeline
  //! Some convenience for making a connection between sequences using two
  //! connectors.
  class pipe_junction {
    public:

    // TODO:
    //   All this is really uncertain because the connectors are not creatable
    //   yet.  Assume it'll change.
    //
    //   Looks like there must be pointers stored in here.  It might also be a
    //   by-value class.  Another alternative is to have a pipe_handler class
    //   which has these methods and a pipe_junction which actually carries the
    //   pipes.

    //! Get a packet from the input connection.
    packet *read_input() {
      NERVE_NIMPL("reading a packet");
      return NULL;
    }

    //! Read a packet which caused a wipe.  This is O(1) because a priority
    //! packet will always wipe the queue.  Returns null if no wiping packet.
    packet *read_input_wipe() {
      NERVE_NIMPL("reading a wipe packet");
      return NULL;
    }

    //! Push onto the output queue.
    void write_output(packet *) {
      NERVE_NIMPL("writing a packet");
    }

    //! Replace the entire output queue.
    void write_output_wipe(packet *p) {
      NERVE_NIMPL("wiping the queue");
      write_output(p);
    }

    private:
  };
}

#endif
