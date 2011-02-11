// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <map>
#include "../plugin-proto/asserts.hpp"

#include "pipeline_configs.hpp"

void config::dump_config_yaml(config::pipeline_config &pc) {
  using namespace config;

  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;
  typedef section_config::stage_iterator_type   stage_iter_t;

  std::map<job_config *, int> job_index;

  std::cout << "---" << std::endl;
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
      const char *from = prev ? prev->name() : "(start pipe)";
      const char *to = next ? next->name() : "(end pipe)";

      std::cout << "    prev: \"" << from << "\"" << std::endl;
      std::cout << "    next: \"" << to << "\"" << std::endl;
      std::cout << "    sequences:" << std::endl;

      stage_config::categories last_cat = stage_config::cat_unset;

      for (stage_iter_t stage = sec->begin(); stage != sec->end(); ++stage) {
        if (stage->category() != last_cat) {
          std::cout << "    - type: \"" << stage->category_name() << '"' << std::endl;
          std::cout << "      stages: " << std::endl;
        }

        const char *where = ":internal";
        const char *ldelim = "";
        const char *rdelim = "";
        if (stage->type() == stage_config::id_plugin) {
          where = stage->path();
          ldelim = "\"";
          rdelim = "\"";
        }

        std::cout
          << "      - [\"" << stage->name() << "\", "
          << ldelim << where << rdelim << "]"
          << std::endl;
      }
    } while ((sec = sec->job_next()) != NULL);

    ++thread_num;
  }
}
