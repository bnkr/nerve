// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_OUTPUT_STAGE_HPP_zia9vdqt
#define PIPELINE_OUTPUT_STAGE_HPP_zia9vdqt

#include "simple_stages.hpp"

namespace pipeline {
  class packet;

  //! \ingroup grp_pipeline
  //! Polymoprphic outputtter which is intended to abstract the thread and local
  //! pipes.
  class outputter {
    public:
    virtual void write(packet *) = 0;
    virtual ~outputter() {}
  };

  //! \ingroup grp_pipeline
  class local_outputter : public outputter {
    public:
    void write(packet*);
  };

  //! \ingroup grp_pipeline
  class thread_outputter : public outputter {
    public:
    void write(packet *);
  };

  /*!
   * \ingroup grp_pipeline
   * Specialised outputting stage.
   */
  class output_stage : public observer_stage {
    public:
    virtual void output(packet *, outputter *) = 0;
    virtual void reconfigure(packet *) = 0;

    outputter *input_events_;

    // Fullfills the 'reconfigure' event.  Somebody has to check it.  If we do
    // it here then it avoids type checking of all stages.  Additionally, it
    // means we can later have a reconfgiure event which a configuration chaning
    // stage could deal with.
    void observe(packet *pkt) {
      // TODO:
      //   Perhaps better in a hypothetical output seqeuence.
      // if (pkt.configuration != last_configuration) {
      //   this->reconfigure(pkt);
      // }

      // Prolly this should pass a specialised event interface.
      this->output(pkt, input_events_);
    }
  };

}

#endif
