// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef PIPELINE_CONNECTION_HPP_me99hwtd
#define PIPELINE_CONNECTION_HPP_me99hwtd

#include "ipc.hpp"
#include <boost/utility.hpp>

namespace pipeline {
  //! \ingroup grp_pipeline
  //! Some convenience for making a connection between sequences using two
  //! pipes.
  template<class InPipe, class OutPipe>
  class connection : boost::noncopyable {
    public:
    typedef InPipe  in_type;
    typedef OutPipe out_type;

    connection() : in_(NULL), out_(NULL) {}

    //! Get a packet from the input connection.
    packet *read_input() {
      return NERVE_CHECK_PTR(this->in())->read();
    }

    //! Read a packet which caused a wipe.  This is O(1) because a priority
    //! packet will always wipe the queue.  Returns null if no wiping packet.
    packet *read_input_wipe() {
      NERVE_NIMPL("reading a wipe packet");
      return NULL;
    }

    //! Push onto the output queue.
    void write_output(packet *p) {
      NERVE_CHECK_PTR(this->out())->write(p);
    }

    //! Replace the entire output queue.
    void write_output_wipe(packet *p) {
      NERVE_CHECK_PTR(this->out())->write_wipe(p);
    }

    void in(in_type *p) { in_ = p; }
    void out(out_type *p) { out_ = p; }

    in_type *in() const { return in_; }
    out_type *out() const { return out_; }

    private:
    InPipe *in_;
    OutPipe *out_;
  };

  typedef connection<pipe, pipe>               polymorphic_connection;
  typedef connection<local_pipe, local_pipe>   local_connectiom;
  typedef connection<thread_pipe, thread_pipe> thread_connection;
}
#endif
