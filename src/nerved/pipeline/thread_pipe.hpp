// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_THREAD_PIPE_HPP_gargmedd
#define PIPELINE_THREAD_PIPE_HPP_gargmedd

#include "ipc.hpp"
#include "../util/asserts.hpp"

namespace pipeline {
  //! \ingroup grp_pipeline
  //! Thread-safe buffer.
  class thread_pipe : public pipe {
    public:

    void write(packet *) {
      NERVE_NIMPL("write to a thread pipe");
    }

    void write_wipe(packet *) {
      NERVE_NIMPL("wipe write to a thread pipe");
    }

    packet *read() {
      NERVE_NIMPL("read from a thread pipe");
      return NULL;
    }
  };
}
#endif
