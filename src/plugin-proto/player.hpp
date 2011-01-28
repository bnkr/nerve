// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <boost/thread.hpp>

//! Main bit of the player.
class player {
  public:
    enum status { status_ok, status_fail };

    enum status play();
};

enum player::status player::play() {
  input_driver inp;
  process_driver proc;
  output_driver out;
  socket_server server;

  boost::thread proc_thread(boost::bind(proc, &process_driver::run));
  boost::thread input_thread(boost::bind(inp, &input_driver::run));
  boost::thread out_thread(boost::bind(out, &output_driver::run));

  server.run();

  proc_thread.join();
  input_thread.join();
  out_thread.join();
}
