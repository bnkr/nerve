// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_data.hpp"
#include "connectors.hpp"
#include "job.hpp"

#include "../util/asserts.hpp"

#include <algorithm>
#include <boost/bind.hpp>

using namespace pipeline;

/**************
 * Connectors *
 **************/

connector *pipeline_data::start_terminator() {
  NERVE_NIMPL("the start terminator");
  return NULL;
}
connector *pipeline_data::end_terminator() {
  NERVE_NIMPL("the end terminator");
  return NULL;
}

connector *pipeline_data::create_pipe() {
  NERVE_NIMPL("creating a pipe");
  return NULL;
}

/*******************
 * Everything else *
 *******************/

job *pipeline_data::create_job() { return jobs_.alloc_back(); }

void pipeline_data::finalise() {
  std::for_each(jobs_.begin(), jobs_.end(), boost::bind(&job::finalise, _1));
}
