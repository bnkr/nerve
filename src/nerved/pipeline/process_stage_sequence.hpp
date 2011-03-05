// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PROCESS_STAGE_SEQUENCE_HPP_d78vgnts
#define PIPELINE_PROCESS_STAGE_SEQUENCE_HPP_d78vgnts

#include "stage_sequence.hpp"
#include "progressive_buffer.hpp"
#include "simple_stages.hpp"

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * The most generals sequence.  It can handle any number of stages, all of
   * which can do some buffering or withhold packets.
   */
  class process_stage_sequence : public stage_sequence {
    public:

    typedef progressive_buffer progressive_buffer_type;

    // This is a little messy because it's using values which are not complete
    // yet.  At this point the progressive buffer can really be seen as part of
    // this object, so I don't think it's worth fixing it.  At least not until
    // the connectors are completely sorted out.
    process_stage_sequence()
    : data_loop_(this->junction()) { }

    simple_stage *create_stage(config::stage_config &cfg);

    void finalise();

    stage_sequence::step_state sequence_step();

    private:
    typedef progressive_buffer_type::stages_type stages_type;

    stages_type &stages() { return data_loop_.stages(); }

    progressive_buffer_type data_loop_;
  };
} // ns pipeline

#endif
