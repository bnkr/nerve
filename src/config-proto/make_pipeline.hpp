/*!
 * \file
 * A test iteration of the pipeline as we would to actually initialise it all.
 */

#include "pipeline_configs.hpp"

void make_pipeline(config::pipeline_config &pc) {
  using namespace config;

  typedef pipeline_config::job_iterator_type    job_iter_t;
  typedef job_config::section_iterator_type     section_iter_t;
  typedef section_config::stage_iterator_type   stage_iter_t;

  std::map<job_config *, int> job_index;

  {
    int ind = 0;
    for (job_iter_t j = pc.begin(); j != pc.end(); ++j) {
      job_index[&(*j)] = ind++;
    }
  }

  section_config *sec = &pc.first_section();
  do {
    std::cout << "- section: '" << NERVE_CHECK_PTR(sec->name()) << "'" << std::endl;

    section_config *const prev = sec->previous_section();
    section_config *const next = sec->next_section();

    const char *from = prev ? prev->name() : "(start pipe)";
    const char *to = next ? next->name() : "(end pipe)";

    std::cout << "  position: " << from << " -> " << to << std::endl;


    // I think the real implementation will need to iterate over jobs to avoid
    // this alloc-heavy map stuff.  Another solution would be to store a pointer
    // to the actual pipeline structure being used for the thread in the config
    // object which I don't like very much.
    std::cout << "  thread #" << job_index[&sec->parent_job()] << std::endl;

    for (stage_iter_t stage = sec->begin(); stage != sec->end(); ++stage) {
      // std::cout << "  - stage '" << stage->name() << std::endl;
    }
  } while ((sec = sec->next_section()) != NULL);
}
