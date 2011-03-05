// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef PIPELINE_OBSERVER_STAGE_SEQUENCE_HPP_4sykuldr
#define PIPELINE_OBSERVER_STAGE_SEQUENCE_HPP_4sykuldr

#include "packet.hpp"
#include "simple_stages.hpp"
#include "stage_sequence.hpp"

#include <algorithm>
#include <boost/bind.hpp>
#include <vector>

namespace pipeline {
  /*!
   * \ingroup grp_pipeline
   * Simple sequence which can only handle those stages that look but don't
   * touch.
   */
  class observer_stage_sequence : public stage_sequence {
    public:
    typedef std::vector<observer_stage*> stages_type;

    stage_sequence::step_state sequence_step() {
      packet *p = read_input();

      switch(NERVE_CHECK_PTR(p)->event()) {
      case packet::event::data:
        std::for_each(
          stages().begin(), stages().end(),
          boost::bind(&observer_stage::observe, _1, p)
        );
        write_output(p);
        break;
      default:
        this->non_data_step(stages(), p);
        break;
      }

      return stage_sequence::state::complete;
    }

    simple_stage *create_stage(config::stage_config &cfg) {
      std::cerr << __FUNCTION__ << ": not implemented: returning null" << std::endl;
      return NULL;
    }

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
