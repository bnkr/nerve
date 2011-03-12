// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <map>
#include "../util/asserts.hpp"

#include "pipeline_configs.hpp"

//! Dump the structures which will be created.
static void thread_yaml(config::pipeline_config &pc);
//! Dump pipeline order.
static void order_yaml(config::pipeline_config &pc);

void config::dump_config_yaml(config::pipeline_config &pc) {
  std::cout << "---" << std::endl;
  std::cout << "threads: " << std::endl;
  thread_yaml(pc);
  std::cout << "stage_order: " << std::endl;
  order_yaml(pc);
}

void thread_yaml(config::pipeline_config &pc) {
  using namespace config;

  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;
  typedef section_config::stage_iterator_type   stage_iter_t;

  int thread_num = 1;
  for (job_iter_t job = pc.begin(); job != pc.end(); ++job) {
    std::cout << "- thread: " << thread_num << std::endl;
    std::cout << "  sections:" << std::endl;

    section_config *sec = job->job_first();
    do {
      NERVE_ASSERT(&(*job) == &(sec->parent_job()), "link order shouldn't go into another job");
      std::cout << "  - name: \"" << sec->name() << "\"" << std::endl;

      section_config *const prev = sec->pipeline_previous();
      section_config *const next = sec->pipeline_next();
      const char *from = prev ? prev->name() : ":start_pipe";
      const char *to = next ? next->name() : ":end_pipe";
      const char *to_delim = next ? "\"" : "";
      const char *from_delim = prev ? "\"" : "";

      std::cout << "    prev: " << from_delim << from << from_delim << std::endl;
      std::cout << "    next: " << to_delim << to << to_delim << std::endl;
      std::cout << "    sequences:" << std::endl;

      stage_config::category_type last_cat = stage_cat::unset;

      for (stage_iter_t stage = sec->begin(); stage != sec->end(); ++stage) {
        if (stage->category() != last_cat) {
          std::cout << "    - type: \"" << stage->category_name() << '"' << std::endl;
          std::cout << "      stages: " << std::endl;
        }

        std::cout << "      - name: \"" << stage->name() << "\"" << std::endl;

        const char *where = ":internal";
        const char *ldelim = "";
        const char *rdelim = "";
        if (! stage->internal()) {
          where = stage->path();
          ldelim = "\"";
          rdelim = "\"";
        }

        std::cout << "        location: " << ldelim << where << rdelim << std::endl;
        std::cout << "        config: ";
        if (stage->configs_given()) {
          std::cout << std::endl;

          typedef stage_config::configs_type configs_type;
          typedef configs_type::pairs_type pairs_type;
          typedef pairs_type::const_iterator iter_t;

          configs_type &cf = *NERVE_CHECK_PTR(stage->configs());
          pairs_type &pairs = cf.pairs();

          for (iter_t conf = pairs.begin(); conf != pairs.end(); ++conf) {
            const char *const key = conf->field();
            const char *const value = conf->value();
            std::cout << "        - [\"" << key << "\", \"" << value << "\"]" << std::endl;;
          }
        }
        else {
          std::cout << "[]" << std::endl;
        }
      }
    } while ((sec = sec->job_next()) != NULL);

    ++thread_num;
  }
}


void order_yaml(config::pipeline_config &pc) {
  using namespace config;

  section_config *sec = pc.pipeline_first();
  do {
    section_config &sc = *NERVE_CHECK_PTR(sec);
    typedef section_config::stage_iterator_type iter_type;

    for (iter_type stage = sc.begin(); stage != sc.end(); ++stage) {
      std::cout << "- " << stage->name() << std::endl;
    }

  } while ((sec = sec->pipeline_next()) != NULL);
}
