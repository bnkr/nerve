// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"

#include "../cli/settings.hpp"
#include "../config/pipeline_configs.hpp"
#include "../pipeline/job.hpp"
#include "../pipeline/input_stage.hpp"
#include "../pipeline/output_stage.hpp"
#include "../pipeline/process_stage_sequence.hpp"
#include "../pipeline/observer_stage_sequence.hpp"
#include "../pipeline/input_stage_sequence.hpp"

#include "../util/pooled.hpp"

#include <iostream>

using namespace pipeline;
using namespace config;

namespace {
  void configure_section(pipeline::job &job, section_config &sec_conf);
  void configure_stage(pipeline::stage_sequence &seq, stage_config &stage_conf);
  void configure_sequences(pipeline::section &sec, section_config &sec_conf);
}

configure_status ::pipeline::configure(pipeline_data &pd, pipeline_config &pc, const cli::settings &cli) {
  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;

  for (job_iter_t job_conf = pc.begin(); job_conf != pc.end(); ++job_conf) {
    pipeline::job &job = *NERVE_CHECK_PTR(pd.create_job());

    section_config *sec_conf = job_conf->job_first();
    do {
      NERVE_ASSERT(&(*job_conf) == &(sec_conf->parent_job()), "link order shouldn't go into another job_conf");
      configure_section(job, *sec_conf);
    } while ((sec_conf = sec_conf->job_next()) != NULL);
  }

  pd.finalise();
  pc.clear();

  return configure_ok;
}

//! Add a new section to the job complete with contained sequences etc.
void configure_section(pipeline::job &job, section_config &sec_conf) {
  section_config *const prev = sec_conf.pipeline_previous();
  section_config *const next = sec_conf.pipeline_next();

  pipeline::section &sec = *job.create_section();

  // TODO:
  //   This doesn't work because the section is not supposed to know about the
  //   connectors.  That said, it might be easier for that to be the case.  The
  //   create_sequence method would do the piping for us.
  //
  //   It might even be possible for the create_section method to do the
  //   thread_pipe bits for sections.  That design seems appealing to me because
  //   it keeps this part down to strictly iteration.  It might mean that the
  //   create_x methods end up having some kind of state.

  if (prev) {
    sec.in_connector(x);
  }
  else {
    sec.in_connector(start_terminator);
  }

  if (next) {
    sec.out_connector(x);
  }
  else {
    sec.in_connector(start_terminator);
  }

  configure_sequences(sec, sec_conf);
}

void configure_sequences(pipeline::section &sec, section_config &sec_conf) {
  stage_config::categories last_cat = stage_config::cat_unset;

  pipeline::stage_sequence *sequence = NULL;
  typedef section_config::stage_iterator_type   stage_iter_t;
  for (stage_iter_t stage_conf = sec_conf.begin(); stage_conf != sec_conf.end(); ++stage_conf) {
    const stage_config::category_type this_cat = stage_conf->category();

    if (this_cat != last_cat) {
      sequence = sec.create_sequence(this_cat);
    }

    configure_stage(*sequence, *stage_conf);
  }
}

void configure_stage(stage_sequence &seq, stage_config &stage_conf) {
  // Passing the stage_conf here breaks the encapsulation we had in the rest of
  // this configure function, but the only other way is a huge switch statement
  // and billions of downcasts.
  //
  // TODO:
  //   Perhaps we could have stage_creator sc(stage_conf); seq.create_stage(sc);
  pipeline::simple_stage *const stage = seq.create_stage(stage_conf);

  if (stage_conf.configs_given()) {
    typedef stage_config::configs_type configs_type;
    typedef configs_type::pairs_type pairs_type;
    typedef pairs_type::const_iterator iter_t;

    configs_type &cf = *NERVE_CHECK_PTR(stage_conf.configs());
    pairs_type &pairs = cf.pairs();

    for (iter_t conf = pairs.begin(); conf != pairs.end(); ++conf) {
      const char *const key = conf->field();
      const char *const value = conf->value();
      stage->configure(key, value);
    }
  }
}
