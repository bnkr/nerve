// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"

#include "../cli/settings.hpp"
#include "../config/pipeline_configs.hpp"
#include "../pipeline/job.hpp"
#include "../pipeline/output_stage.hpp"
#include "../pipeline/process_stage_sequence.hpp"
#include "../pipeline/observer_stage_sequence.hpp"
#include "../pipeline/input_stage_sequence.hpp"

#include "../util/pooled.hpp"

#include <iostream>

using namespace pipeline;
using namespace config;

namespace stage_cat = config::stage_cat;

static void configure_stage(pipeline::stage_sequence &seq, stage_config &stage_conf);
static void configure_sequences(pipeline::section &sec, section_config &sec_conf);

configure_status ::pipeline::configure(pipeline_data &pd, pipeline_config &pc, const cli::settings &) {
  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;

  for (job_iter_t job_conf = pc.begin(); job_conf != pc.end(); ++job_conf) {
    pipeline::job &job = *NERVE_CHECK_PTR(pd.create_job());

    struct {
      section_config *cur_conf;
      section *last_sec;
    } loop_state;

    loop_state.cur_conf = NERVE_CHECK_PTR(job_conf->job_first());
    loop_state.last_sec = NULL;

    do {
      NERVE_ASSERT(&(*job_conf) == &(loop_state.cur_conf->parent_job()), "link order shouldn't go into another job_conf");

      section_config &cc = *NERVE_CHECK_PTR(loop_state.cur_conf);

      section_config *const prev = cc.pipeline_previous();
      section_config *const next = cc.pipeline_next();

      connector *const in = prev ? NERVE_CHECK_PTR(loop_state.last_sec)->output_pipe() : pd.start_terminator();
      connector *const out = next ? pd.create_pipe() : pd.end_terminator();

      section *const sec_to_configure = NERVE_CHECK_PTR(job.create_section(in, out));
      configure_sequences(*sec_to_configure, cc);
      loop_state.last_sec = sec_to_configure;
    } while ((loop_state.cur_conf = NERVE_CHECK_PTR(loop_state.cur_conf)->job_next()) != NULL);
  }

  pd.finalise();
  pc.clear();

  return configure_ok;
}

void configure_sequences(pipeline::section &sec, section_config &sec_conf) {
  stage_config::category_type last_cat = ::stage_cat::unset;

  pipeline::stage_sequence *sequence = NULL;
  typedef section_config::stage_iterator_type   stage_iter_t;
  for (stage_iter_t stage_conf = sec_conf.begin(); stage_conf != sec_conf.end(); ++stage_conf) {
    const stage_config::category_type this_cat = stage_conf->category();

    if (this_cat != last_cat) {
      sequence = NERVE_CHECK_PTR(sec.create_sequence(this_cat));
    }

    configure_stage(*NERVE_CHECK_PTR(sequence), *stage_conf);
  }
}

void configure_stage(stage_sequence &seq, stage_config &stage_conf) {
  // TODO:
  //   This will use an object "stages::stage_data" which is shared knowledge
  //   between pipeline and config so it doesn't break modularity.
  pipeline::simple_stage *const stage = NERVE_CHECK_PTR(seq.create_stage(stage_conf));

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
