// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PROCESS_STAGE_SEQUENCE_HPP_d78vgnts
#define PIPELINE_PROCESS_STAGE_SEQUENCE_HPP_d78vgnts

#include "stage_sequence.hpp"
#include "progressive_buffer.hpp"
#include "simple_stages.hpp"

#include "../util/asserts.hpp"

#include <vector>

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * The most generals sequence.  It can handle any number of stages, all of
   * which can do some buffering or withhold packets.
   */
  class process_stage_sequence : public stage_sequence {
    public:

    typedef std::vector<process_stage*> stages_type;

    // TODO:
    //   This needs to be changed a bit because we add stages later.  There must
    //   be a finalise method.
    process_stage_sequence()
    : data_loop_(stages_) {
    }

    // Constant delay is guaranteed by the use of the progressive buffering loop.
    stage_sequence::step_state sequence_step() {
      // Checking for non-data only is necessary to discover non-data events as
      // quickly as possible.  Otherwise a buffering stage will delay the input
      // connector operation.
      //
      // TODO:
      //   This could be omtimised if we could iterate all simple stages in one
      //   go.  Then the section could check for most non-data events.  That
      //   would mean no need to pass flushes etc. down the local pipes so less
      //   checks all round.  There are slight problems with the input stage
      //   though (because it can "create" nd-events).
      //
      // TODO:
      //   It would improve latency further if we did this at the end of each
      //   stage.  Of course, check is useless on a local pipe...

      typedef stage_sequence::state state;

      packet *p = NERVE_CHECK_PTR(read_input());
      switch (p->event()) {
      case packet::event::data:
        data_loop_.step();
        return data_loop_.buffering() ? state::buffering : state::complete;
      case packet::event::abandon:
        data_loop_.abandon_reset();
        // fallthrough
      default:
        this->non_data_step(stages(), p);
        return state::complete;
      }
    }

    private:

    stages_type &stages() { return stages_; }

    stages_type stages_;
    progressive_buffer data_loop_;
  };
} // ns pipeline

#endif
