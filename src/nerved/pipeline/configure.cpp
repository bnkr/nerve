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
static job *configure_job(output::logger &, pipeline_data &, job_config &job_conf);

configure_status ::pipeline::configure(pipeline_data &pd, pipeline_config &pc, const cli::settings &) {
  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;

  output::logger log(output::source::pipeline);
  log.trace("configuring pipeline\n");

  section_config *sec_conf = NERVE_CHECK_PTR(pc.pipeline_first());
  section *last_sec = NULL;

  // Only for trace output.
  job *last_job = NULL;
  std::map<job*, int> job_nums;

  // We must loop over sections instead of jobs because we meed to visit each
  // section in pipeline order so that the previous section is fully constructed
  // before the next.  Otherwise we have initialise the pipes (and possibly
  // some other stuff) in another pass of the data.
  do {
    section_config &sc = *NERVE_CHECK_PTR(sec_conf);
    job_config &jc = sc.parent_job();
    job &job =
      (jc.configured_job() != NULL) ? *jc.configured_job() : *configure_job(log, pd, jc);

    section_config *const prev = sc.pipeline_previous();
    section_config *const next = sc.pipeline_next();

    if (log.should_write(output::cat::trace)) {
      pipeline::job *const this_job = &job;
      if (! job_nums.count(this_job)) {
        job_nums[this_job] = last_job ? job_nums[last_job] + 1 : 1;
        last_job = this_job;
      }
      const int job_num = job_nums[this_job];

      log.trace(
        "configure section '%s' in job %d (%s -> [%s] -> %s)\n",
        sc.name(), job_num,
        prev ? prev->name() : "(start)", sc.name(), next ? next->name() : "(end)"
      );
    }

    pipe *const in = prev
      ? NERVE_CHECK_PTR(NERVE_CHECK_PTR(last_sec)->connection().out())
      : NERVE_CHECK_PTR(pd.start_terminator());
    pipe *const out = next
      ? NERVE_CHECK_PTR(static_cast<pipe*>(pd.create_thread_pipe()))
      : NERVE_CHECK_PTR(static_cast<pipe*>(pd.end_terminator()));

    section *const sec_to_configure = NERVE_CHECK_PTR(job.create_section(in, out));

    configure_sequences(log, *sec_to_configure, sc);
    last_sec = NERVE_CHECK_PTR(sec_to_configure);
  } while ((sec_conf = sec_conf->pipeline_next()) != NULL);

  pd.finalise();
  pc.clear();

  log.trace("finished configuration\n");

  return configure_ok;
}

job *configure_job(output::logger &log, pipeline_data &pd, job_config &job_conf) {
  log.trace("create job %d\n", pd.jobs().size() + 1);
  pipeline::job *const j = NERVE_CHECK_PTR(pd.create_job());
  job_conf.configured_job(j);
  return j;
}

void configure_sequences(output::logger &log, pipeline::section &sec, section_config &sec_conf) {
  stage_config::category_type last_cat = ::stage_cat::unset;

  pipeline::stage_sequence *sequence = NULL;
  typedef section_config::stage_iterator_type   stage_iter_t;

  pipeline::pipe *input_pipe = NERVE_CHECK_PTR(sec.connection().in());
  pipeline::pipe *output_pipe = NULL;

  for (stage_iter_t stage_conf = sec_conf.begin(); stage_conf != sec_conf.end(); ++stage_conf) {
    const stage_config::category_type this_cat = stage_conf->category();

    if (this_cat != last_cat) {
      log.trace("add %s sequence under section '%s'\n", stage_conf->category_name(), sec_conf.name());
      sequence = NERVE_CHECK_PTR(sec.create_sequence(this_cat, input_pipe, output_pipe));
      input_pipe = output_pipe;
      const bool last = (++stage_conf) == sec_conf.end();
      output_pipe = last ? NERVE_CHECK_PTR(sec.connection().out()) : NERVE_CHECK_PTR(sequence->create_local_pipe());
    }

    configure_stage(log, *NERVE_CHECK_PTR(sequence), *stage_conf);
  }
}

void configure_stage(output::logger &log, stage_sequence &seq, stage_config &stage_conf) {
  pipeline::simple_stage *const stage = NERVE_CHECK_PTR(seq.create_stage(stage_conf.stage_data()));

  log.trace("add stage %s\n", stage_conf.name());

  if (stage_conf.configs_given()) {
    typedef stage_config::configs_type configs_type;
    typedef configs_type::pairs_type pairs_type;
    typedef pairs_type::const_iterator iter_t;

    configs_type &cf = *NERVE_CHECK_PTR(stage_conf.configs());
    pairs_type &pairs = cf.pairs();

    for (iter_t conf = pairs.begin(); conf != pairs.end(); ++conf) {
      const char *const key = conf->field();
      const char *const value = conf->value();
      log.trace("stage %s: %s = %s\n", stage_conf.name(), key, value);
      stage->configure(key, value);
    }
  }
}
