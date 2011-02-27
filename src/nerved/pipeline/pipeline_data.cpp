// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_data.hpp"
#include "connectors.hpp"
#include "job.hpp"

using namespace pipeline;

/**************
 * Connectors *
 **************/

connector *pipeline_data::start_terminator() {
  return NULL;
}
connector *pipeline_data::end_terminator() {
  return NULL;
}

connector *pipeline_data::create_pipe() {
  return NULL;
}

/*******************
 * Everything else *
 *******************/

job *pipeline_data::create_job() {
  return NULL;
}

void pipeline_data::finalise() {
}
