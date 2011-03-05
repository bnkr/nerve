// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "information.hpp"

#include "../util/asserts.hpp"

using namespace stages;

const char *stages::get_category_name(stages::category_type c) {
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

const char *stages::get_plugin_name(stages::plugin_id_type p) {
  switch (p) {
  case plug_id::unset:
    return "(undefined)";
  }

  NERVE_ABORT("what are you doing here?");
}
