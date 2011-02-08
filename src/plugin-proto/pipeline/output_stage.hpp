// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_OUTPUT_STAGE_HPP_zia9vdqt
#define PIPELINE_OUTPUT_STAGE_HPP_zia9vdqt

#include "simple_stages.hpp"

namespace pipeline {

  class outputter {
    virtual void write(packet) = 0;
  };

  class local_outputter  : outputter {
    local_pipe p_;
    void write(packet);
  };

  class thread_outputter : outputter {
    thread_pipe p_;
    void write(packet);
  };

  /*!
   * \ingroup grp_pipeline
   * Specialised outputting stage.
   */
  class output_stage : public observer_stage {
    void output(pkt, outputter) = 0;
    void reconfigure(pkt) = 0;

    // A pipe to the input thread.  This can be local or threaded.
    outputter input_events_;

    void observe(packet *pkt) {
      // fullfills the 'reconfigure' event.  Somebody has to check it.  If we do
      // it here then it avoids type checking of all stages.  Additionally, it
      // means we can later have a reconfgiure event which a configuration chaning
      // stage could deal with.

      // TODO:
      //   Perhaps better in a hypothetical output seqeuence.

      if (pkt.configuration != last_configuration) {
        this->reconfigure(pkt);
      }

      // hmmm...
      this->real_observe(pkt);
    }
  };

}

#endif
