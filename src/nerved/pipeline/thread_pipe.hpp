// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_THREAD_PIPE_HPP_gargmedd
#define PIPELINE_THREAD_PIPE_HPP_gargmedd
#include "ipc.hpp"

namespace pipeline {
  //! \ingroup grp_pipeline
  //! Thread-safe buffer.
  class thread_pipe : public pipe {
    public:
    void write(packet *);
    void write_wipe(packet *);
    packet *read();
  };
}
#endif
