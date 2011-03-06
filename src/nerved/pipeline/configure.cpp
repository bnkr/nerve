// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "configure.hpp"

#include "../cli/settings.hpp"
#include "../config/pipeline_configs.hpp"
#include "../output/logging.hpp"
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

static void configure_sequences(output::logger &, pipeline::section &, section_config &);
static void configure_stage(output::logger &, pipeline::stage_sequence &, stage_config &);

configure_status ::pipeline::configure(pipeline_data &pd, pipeline_config &pc, const cli::settings &) {
  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;

  output::logger log(output::source::pipeline);

  struct {
    section_config *cur_conf;
    section *last_sec;
  } loop_state;
  loop_state.last_sec = NULL;

  // debugging
  std::map<section_config*, section*> created;

  int job_num = 0;
  for (job_iter_t job_conf = pc.begin(); job_conf != pc.end(); ++job_conf) {
    pipeline::job &job = *NERVE_CHECK_PTR(pd.create_job());
    ++job_num;

    loop_state.cur_conf = NERVE_CHECK_PTR(job_conf->job_first());

    do {
      NERVE_ASSERT(&(*job_conf) == &(loop_state.cur_conf->parent_job()), "link order shouldn't go into another job_conf");

      section_config &cc = *NERVE_CHECK_PTR(loop_state.cur_conf);

      section_config *const prev = cc.pipeline_previous();
      section_config *const next = cc.pipeline_next();

      if (log.should_write(output::cat::info)) {
        log.info(
          "configure section '%s' in thread %d (%s -> [%s] -> %s)\n",
          loop_state.cur_conf->name(), job_num,
          prev ? prev->name() : "(start)",
          loop_state.cur_conf->name(),
          next ? next->name() : "(end)"
        );
      }

      if (prev) {
        // TODO:
        //   These asserts fire on weird-order.conf because the sections are not
        //   specified in pipeline order.  We iterate jobs, so it ends up that
        //   we traverse in non-pipeline order.  This is fixible by:
        //
        //   - iterating sections in pipeline order
        //   - allocating jobs the first time we see them
        //   - store allocated jobs in the job config:
        //
        //   if (! sec_conf->parent_job()->allocated_job()) {
        //     allocate_job as per config
        //     store job
        //   }
        NERVE_ASSERT(created.count(prev), "we must have allocated something for the previous section");
        NERVE_ASSERT(
          created[prev] == loop_state.last_sec,
          "the last_sec state must denote what we created for this section config"
        );
      }

      connector *const in = prev ? NERVE_CHECK_PTR(loop_state.last_sec)->output_pipe() : pd.start_terminator();
      connector *const out = next ? pd.create_pipe() : pd.end_terminator();
      std::cerr << "previous section is " << (void*) loop_state.last_sec << std::endl;

      section *const sec_to_configure = NERVE_CHECK_PTR(job.create_section(in, out));
      std::cerr << "created section is " << (void*) sec_to_configure << std::endl;

      created[loop_state.cur_conf] = sec_to_configure;

      configure_sequences(log, *sec_to_configure, cc);
      loop_state.last_sec = NERVE_CHECK_PTR(sec_to_configure);

    } while ((loop_state.cur_conf = NERVE_CHECK_PTR(loop_state.cur_conf)->job_next()) != NULL);
  }

  pd.finalise();
  pc.clear();

  log.info("finished configuration\n");

  return configure_ok;
}

void configure_sequences(output::logger &log, pipeline::section &sec, section_config &sec_conf) {
  stage_config::category_type last_cat = ::stage_cat::unset;

  pipeline::stage_sequence *sequence = NULL;
  typedef section_config::stage_iterator_type   stage_iter_t;
  for (stage_iter_t stage_conf = sec_conf.begin(); stage_conf != sec_conf.end(); ++stage_conf) {
    const stage_config::category_type this_cat = stage_conf->category();

    if (this_cat != last_cat) {
      log.info("add %s sequence under section '%s'\n", stage_conf->category_name(), sec_conf.name());
      sequence = NERVE_CHECK_PTR(sec.create_sequence(this_cat));
    }

    configure_stage(log, *NERVE_CHECK_PTR(sequence), *stage_conf);
  }
}

void configure_stage(output::logger &log, stage_sequence &seq, stage_config &stage_conf) {
  pipeline::simple_stage *const stage = NERVE_CHECK_PTR(seq.create_stage(stage_conf.stage_data()));

  log.info("add stage %s\n", stage_conf.name());

  if (stage_conf.configs_given()) {
    typedef stage_config::configs_type configs_type;
    typedef configs_type::pairs_type pairs_type;
    typedef pairs_type::const_iterator iter_t;

    configs_type &cf = *NERVE_CHECK_PTR(stage_conf.configs());
    pairs_type &pairs = cf.pairs();

    for (iter_t conf = pairs.begin(); conf != pairs.end(); ++conf) {
      const char *const key = conf->field();
      const char *const value = conf->value();
      log.info("stage %s: %s = %s\n", stage_conf.name(), key, value);
      stage->configure(key, value);
    }
  }
}
