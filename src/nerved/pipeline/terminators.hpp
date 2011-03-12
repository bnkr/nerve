// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_TERMINATORS_HPP_u5uc90u4
#define PIPELINE_TERMINATORS_HPP_u5uc90u4

#include "ipc.hpp"
#include "packet.hpp"

namespace pipeline {
  struct packet;

  //! \ingroup grp_pipeline
  //! Starts the pipeline.
  class start_terminator : public pipe {
    public:
    start_terminator() {
      dummy_packet_.event(packet::event::data);
    }

    void write(packet *) { do_write(); }
    void write_wipe(packet *) { do_write(); }

    packet *read() {
      return &dummy_packet_;
    }

    private:
    void do_write() { NERVE_ABORT("can't write to the start terminator"); }

    // TODO:
    //   Almost certainly want something else here because the input stage might
    //   mess up the packet.
    //
    //   Also, I can control the input stage more effectively if there is more
    //   direct access.  If the input sequence knew about the input terminator
    //   then it'd be easier...
    packet dummy_packet_;
  };

  //! \ingroup grp_pipeline
  //! Ends the pipeline and frees packets.
  class end_terminator : public pipe {
    public:
    void write(packet *) { do_write(); }
    void write_wipe(packet *) { do_write(); }

    packet *read() {
      NERVE_ABORT("can't read from the end terminator");
      return NULL;
    }

    private:
    void do_write() {
      NERVE_NIMPL("freeing end-terminator packets");
    }
  };
}

#endif
