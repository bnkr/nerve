// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_data.hpp"
#include "connectors.hpp"
#include "job.hpp"

#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

using namespace pipeline;

/**************
 * Connectors *
 **************/

connector *pipeline_data::start_terminator() {
  std::cerr << __PRETTY_FUNCTION__ << ": not implemented: returning null" << std::endl;
  return NULL;
}
connector *pipeline_data::end_terminator() {
  std::cerr << __PRETTY_FUNCTION__ << ": not implemented: returning null" << std::endl;
  return NULL;
}

connector *pipeline_data::create_pipe() {
  std::cerr << __PRETTY_FUNCTION__ << ": not implemented: returning null" << std::endl;
  return NULL;
}

/*******************
 * Everything else *
 *******************/

job *pipeline_data::create_job() {
  std::cerr << __PRETTY_FUNCTION__ << ": not implemented: returning null" << std::endl;
  return NULL;
}

void pipeline_data::finalise() {
  std::for_each(jobs_.begin(), jobs_.end(), boost::bind(&job::finalise, _1));
}
