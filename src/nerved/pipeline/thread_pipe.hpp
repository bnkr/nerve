// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_THREAD_PIPE_HPP_gargmedd
#define PIPELINE_THREAD_PIPE_HPP_gargmedd

#include "ipc.hpp"
#include "../util/asserts.hpp"
#include "../para/pipes.hpp"

#include <boost/thread.hpp>

namespace pipeline {
  struct packet;

  //! \ingroup grp_pipeline
  //! Thread-safe buffer.
  class thread_pipe : public pipe {
    public:

    thread_pipe() {}

    void write(packet *p) { p_.write(p); }
    void write_wipe(packet *p) { p_.write_clear(p); }
    packet *read() { return p_.read(); }

    private:

    typedef boost::condition_variable condition_type;
    typedef boost::mutex lockable_type;
    typedef para::basic_monitor_sync<condition_type, lockable_type> boost_sync_type;

    para::pipe<packet*, boost_sync_type> p_;
  };
}

#endif
