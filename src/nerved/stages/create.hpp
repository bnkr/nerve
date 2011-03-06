// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_CREATE_HPP_vo8h7vvo
#define STAGES_CREATE_HPP_vo8h7vvo

#include "../pipeline/input_stage.hpp"
#include "../pipeline/simple_stages.hpp"
#include "../util/pooled.hpp"

namespace stages {
  class stage_data;

  //! \ingroup grp_stage
  //! Allocation strategy for stage objects.
  typedef void*(*alloc_func)(size_t);

  //! \ingroup grp_stage
  //! Create an input stage.  A non-input stage is invalid.
  pipeline::input_stage *create_input_stage(stage_data &, alloc_func = &pooled::tracked_byte_alloc);
}

#endif
