// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_CREATE_HPP_vo8h7vvo
#define STAGES_CREATE_HPP_vo8h7vvo

namespace pipeline {
  struct input_stage;
  struct output_stage;
  struct simple_stage;
  struct observer_stage;
  struct process_stage;
}

#include "../util/pooled.hpp"

namespace stages {
  class stage_data;

  // TODO:
  //   We must integrate the indirect_x containers here.  Tricky because we end
  //   up needing templates somehow.  Perhaps boost bind can help?  The problem
  //   is that the create function decides on the type to allocate...

  //! \ingroup grp_stages
  //! Allocation strategy for stage objects.
  typedef void*(*alloc_func)(size_t);

  //! \ingroup grp_stages
  //! Create an input stage.  A non-input stage is invalid.
  pipeline::input_stage *create_input_stage(stage_data &, alloc_func = &pooled::tracked_byte_alloc);

  //! \ingroup grp_stages
  //! Create an observer stage.  A non-input stage is invalid.
  pipeline::observer_stage *create_observer_stage(stage_data &, alloc_func = &pooled::tracked_byte_alloc);
}

#endif
