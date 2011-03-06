// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#include "run.hpp"

#include "../pipeline/pipeline_data.hpp"
#include "../server/local_server.hpp"
#include "../output/logging.hpp"

#include <boost/asio/io_service.hpp>

using pipeline::pipeline_data;

player::run_status player::run(pipeline::pipeline_data &pl, const cli::settings &) {
  output::logger log(output::source::player);
  log.trace("player starting\n");

  boost::asio::io_service io_service;
  const char *const file = "/tmp/nerve.socket";
  std::remove(file);
  server::local_server server(io_service, file);

  boost::thread_group threads;
  typedef pipeline_data::jobs_type::iterator iter_type;
  for (iter_type i = pl.jobs().begin(); i != pl.jobs().end(); ++i) {
    threads.create_thread(boost::bind(&pipeline::job::job_thread, boost::ref(*i)));
  }

  io_service.run();
  threads.join_all();

  return run_ok;
}
