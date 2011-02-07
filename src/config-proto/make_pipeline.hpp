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

  for (job_iter_t job = pc.begin(); job != pc.end(); ++job) {
    std::cout << "thread #1" << std::endl;

    for (section_iter_t section = job->begin(); section != job->end(); ++section) {
      std::cout << "- section '" << section->name() << std::endl;
      std::cout << "  read from '" << section->after_section()->name() << std::endl;

      // do the separation into different types here
      for (stage_iter_t stage = section->begin(); stage != section->end(); ++stage) {
        // std::cout << "  - stage '" << stage->name() << std::endl;
      }
    }
  }
}
