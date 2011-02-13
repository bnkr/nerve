#include "configure.hpp"

pipeline::configure_status pipeline::configure(pipeline_data &, const config::pipeline_config &, const cli::settings &) {
  return configure_fail;
}
