// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_INPUT_STAGE_SEQUENCE_HPP_h3g7uj1i
#define PIPELINE_INPUT_STAGE_SEQUENCE_HPP_h3g7uj1i

#include "stage_sequence.hpp"
#include "input_stage.hpp"

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * Specialist stage sequence for dealing with input event stages.  This is
   * necesasry to avoid having every stage check if their packets are and
   * because the input stage results in the creation of non-data events.
   */
  class input_stage_sequence : public stage_sequence {
    public:

    void sequence_step() {
      packet &p = *NERVE_CHECK_PTR(read_input());
      switch (p.event()) {
      case packet::event::load:
        load_event();
        break;
      case packet::event::skip:
        skip_event();
        break;
        // TODO : more events
      default:
        NERVE_ABORT("event should never happen here");
      }

      // change p.event to "abandon" unless we just read data
      write_output_wipe(&p);
    }

    // TODO:
    //   Calls load on the input stage with details of what to load.  Not
    //   entirely sure on that yet so let's leave it.
    void load_event();
    void skip_event();

    private:
    input_stage *is_;
  };
}
#endif
