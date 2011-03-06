// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_LOCAL_PIPE_HPP_bnozulb3
#define PIPELINE_LOCAL_PIPE_HPP_bnozulb3
#include "ipc.hpp"
#include "../util/asserts.hpp"

#include <algorithm>

namespace pipeline {
  //! \ingroup grp_pipeline
  //! Non-thread safe non-buffered pipe.
  class local_pipe : public pipe {
    public:
    local_pipe() : data_(NULL) {}

    void write(packet *p) { do_write(p); }
    void write_wipe(packet *p) { do_write(p); }

    void clear() {}

    packet *read() {
      NERVE_ASSERT(data_ != NULL, "can't read from an empty local pipe");
      packet *ret = NULL;
      std::swap(ret, data_);
      return data_;
    }

    private:

    void do_write(packet *p) {
      NERVE_ASSERT(data_ == NULL, "can't write to a full local pipe");
      data_ = p;
    }

    private:
    packet *data_;
  };
}
#endif
