// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PACKET_RETURN_HPP_2ivoc82g
#define PIPELINE_PACKET_RETURN_HPP_2ivoc82g

#include <cstdlib>

#include "../util/asserts.hpp"

namespace pipeline {

  // TODO:
  //   should perhaps be with stages.  Could also be named stage_return or
  //   something.

  struct packet;

  //! \ingroup grp_pipeline
  //! Informational data attached to a packet.
  class packet_return {
    public:

    packet_return() : buffering_(false), packet_(NULL) {}

    bool buffering() const { return buffering_; }
    bool empty() const { return packet_ != NULL; }
    pipeline::packet *packet() const {
      NERVE_ASSERT(! this->empty(), "access to packet is not allowed for an empty return");
      return packet_;
    }

    private:

    bool buffering_;
    pipeline::packet *packet_;
  };
}

#endif
