// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_INPUT_STAGE_SEQUENCE_HPP_h3g7uj1i
#define PIPELINE_INPUT_STAGE_SEQUENCE_HPP_h3g7uj1i

#include "stage_sequence.hpp"
#include "input_stage.hpp"

#include "../util/asserts.hpp"

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   *
   * Specialist stage sequence for dealing with input event stages.  This is
   * necesasry to avoid having every stage check if their packets are special
   * input events and because the input stage results in the creation of
   * non-data events while others don't.
   */
  class input_stage_sequence : public stage_sequence {
    public:

    input_stage_sequence() : is_(NULL) {}

    void finalise() {}

    // TODO:
    //   Try to avoid using pipeline_configs.  A better solution would be to
    //   have the config class contain some data which is in the stages module.
    simple_stage *create_stage(config::stage_config &cfg);
    stage_sequence::step_state sequence_step();

    private:
    void load_event();
    void skip_event();

    private:
    input_stage *is_;
  };
}
#endif
