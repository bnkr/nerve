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
}
