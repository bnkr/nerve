// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "stage_data.hpp"

#include "../util/asserts.hpp"

using namespace pipeline;

const char *pipeline::get_category_name(pipeline::stage_category_type c) {
  switch (c) {
  case stage_cat::input:
    return "input";
  case stage_cat::output:
    return "output";
  case stage_cat::process:
    return "process";
  case stage_cat::observe:
    return "observe";
  case stage_cat::unset:
    return "(undefined)";
  }

  NERVE_ABORT("what are you doing here?");
}
