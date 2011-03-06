// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_TERMINATORS_HPP_u5uc90u4
#define PIPELINE_TERMINATORS_HPP_u5uc90u4

#include "ipc.hpp"

namespace pipeline {
  struct packet;

  //! \ingroup grp_pipeline
  //! Starts the pipeline.
  class start_terminator : public pipe {
    public:
    void write(packet *) { do_write(); }
    void write_wipe(packet *) { do_write(); }
    packet *read() {
      NERVE_NIMPL("reading from the start terminator");
      return NULL;
    }

    private:
    void do_write() { NERVE_ABORT("can't write to the start terminator"); }
  };

  //! \ingroup grp_pipeline
  //! Ends the pipeline and frees packets.
  class end_terminator : public pipe {
    public:
    void write(packet *) { do_write(); }
    void write_wipe(packet *) { do_write(); }

    packet *read() {
      NERVE_ABORT("can't write to the start terminator");
      return NULL;
    }

    private:
    void do_write() {
      NERVE_NIMPL("freeing end-terminator packets");
    }
  };
}

#endif
