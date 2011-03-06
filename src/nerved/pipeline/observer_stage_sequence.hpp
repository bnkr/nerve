// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef PIPELINE_OBSERVER_STAGE_SEQUENCE_HPP_4sykuldr
#define PIPELINE_OBSERVER_STAGE_SEQUENCE_HPP_4sykuldr

#include "stage_sequence.hpp"

#include "../util/asserts.hpp"
#include "../util/indirect.hpp"

#include <vector>

namespace pipeline {
  struct observer_stage;

  /*!
   * \ingroup grp_pipeline
   * Simple sequence which can only handle those stages that look but don't
   * touch.
   */
  class observer_stage_sequence : public stage_sequence {
    public:
    typedef indirect_owned_polymorph<observer_stage> stages_type;

    stage_sequence::step_state sequence_step();
    simple_stage *create_stage(stages::stage_data &cfg);

    void finalise() {}

    private:

    stages_type &stages() { return stages_; }
    stages_type stages_;
  };

  //! \ingroup grp_pipeline
  //! Identical sequence for now.
  typedef observer_stage_sequence output_stage_sequence;
}
#endif
