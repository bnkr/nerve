// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_IPC_HPP_fcra0rok
#define PIPELINE_IPC_HPP_fcra0rok

// Note: this head is small because it's included all over the place and I want
// to avoid pulling the whole of the thread/local pipe dependencies (which can
// be a bit on the big size).

#include "../util/asserts.hpp"

namespace pipeline {
  struct packet;

  //! \ingroup grp_pipeline
  //! Polymorphic one-way data transfer between stages (the sequences does the
  //! actual usage of these)
  class pipe {
    public:
    virtual void write(packet *) = 0;
    virtual void write_wipe(packet *) = 0;
    virtual packet *read() = 0;
  };

  struct thread_pipe;
  struct local_pipe;
}

#endif
