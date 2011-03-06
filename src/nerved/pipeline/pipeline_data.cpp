// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "pipeline_data.hpp"
#include "job.hpp"
#include "terminators.hpp"
#include "thread_pipe.hpp"

#include "../util/asserts.hpp"

#include <algorithm>
#include <boost/bind.hpp>

using namespace pipeline;
// ambiguous otherwise
typedef pipeline::pipe pipe_type;

void pipeline_data::clear() {
  jobs_.clear();
}

job *pipeline_data::create_job() { return jobs_.alloc_back(); }

void pipeline_data::finalise() {
  std::for_each(jobs_.begin(), jobs_.end(), boost::bind(&job::finalise, _1));
}
