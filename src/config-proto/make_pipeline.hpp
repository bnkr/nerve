// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * A test iteration of the pipeline as we would to actually initialise it all.
 */

#include "pipeline_configs.hpp"

#include <map>
#include "../plugin-proto/asserts.hpp"

void make_pipeline(config::pipeline_config &pc) {
  using namespace config;

  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;
  typedef section_config::stage_iterator_type   stage_iter_t;

  std::map<job_config *, int> job_index;

  // TODO:
  //   This method can be removed now we have a thred-order iteration.
  {
    int ind = 0;
    for (job_iter_t j = pc.begin(); j != pc.end(); ++j) {
      job_index[&(*j)] = ind++;
    }
  }

  // other method:
  //
  //   for j in jobs
  //     for s in sections
  //       next if not s->parent_job() == j
  //       ...
  //
  // Or, now that job ordering exists, we could do:
  //
  //   each job
  //     each section
  //       ...
  //

  section_config *sec = NERVE_CHECK_PTR(pc.pipeline_first());
  do {
    std::cout << "- section: '" << NERVE_CHECK_PTR(sec->name()) << "'" << std::endl;

    section_config *const prev = sec->pipeline_previous();
    section_config *const next = sec->pipeline_next();

    const char *from = prev ? prev->name() : "(start pipe)";
    const char *to = next ? next->name() : "(end pipe)";

    std::cout << "  position: " << from << " -> " << to << std::endl;
    std::cout << "  thread #" << job_index[&sec->parent_job()] << std::endl;

    for (stage_iter_t stage = sec->begin(); stage != sec->end(); ++stage) {
      // std::cout << "  - stage '" << stage->name() << std::endl;
    }
  } while ((sec = sec->pipeline_next()) != NULL);
}
