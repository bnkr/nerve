// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_PROGRESSIVE_BUFFER_HPP_5bymbxa6
#define PIPELINE_PROGRESSIVE_BUFFER_HPP_5bymbxa6

#include "packet.hpp"
#include "packet_return.hpp"
#include "simple_stages.hpp"

#include "../util/asserts.hpp"

#include <vector>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/type_traits/remove_pointer.hpp>

namespace pipeline {
  struct process_stage;

/*!
 * \ingroup grp_pipeline
 *
 * Algorithm object for the following goals:
 *
 * - enforces the constant delay rule
 * - stages can produce any number of packets
 * - buffers don't expand (i.e if something is debuffering then no more input is
 *   added)
 *
 * The drawback is that there is overhead for stages which will only ever return
 * one or less packet.
 */
class progressive_buffer {
  public:

  typedef std::vector<process_stage*> stages_type;
  typedef stages_type::iterator iterator_type;
  typedef boost::remove_pointer<stages_type::value_type>::type stage_type;

  typedef packet_return stage_value_type;

  // TODO:
  //   Deal with the input/output pipes.  Need to pass a junction (or whatever
  //   will be used).
  progressive_buffer(stages_type &stages)
  : stages_(stages)
  { }

  //! Call when the stage list is fully set up.
  void reset_start() { start(this->stages().begin()); };

  // This resets the progressive buffer (not the stages) for When the stages
  // themselves will be abandoned.  This means all the buffers will be empty, so
  // we need to go to the start again.
  void abandon_reset() { this->reset_start(); }

  // Is there a buffering stage here?
  bool buffering() { return this->start() != this->stages().begin(); }

  // Perform an iteration of the stages where no stage is visited twice (but
  // some might be unvisited).  Pulls data from the input queue only when
  // necessary.
  void step() {
    packet *input;
    iterator_type real_start = start_;

    if (this->buffering()) {
      ++real_start;
      input = this->debuffer_input();
    }
    else {
      input = this->read_input();
    }

    const iterator_type end = stages().end();

    for (iterator_type s = real_start; s != end; ++s) {
      NERVE_ASSERT(! input->non_data(), "can't be doing a data loop on non-data");
      const stage_value_type ret = NERVE_CHECK_PTR(*s)->process(input);

      NERVE_ASSERT(! (ret.empty() && ret.buffering()), "empty xor buffering");
      if (ret.empty()) {
        return;
      }
      else if (ret.buffering()) {
        // Simply assigning this every time we meet a buffering stage means we
        // end up debuffering the latest stage which has stuff to debuffer.
        //
        // TODO:
        //   This means we ignore any stage earlier in the sequence which is
        //   buffering and therefore buffers expand when we push more data!  We
        //   must use a stack to reduce our bufferingness.
        start(s);
      }

      input = ret.packet();
    }

    // It is nicer to do the connection work here because otherwise the section
    // or sequence have to keep checking to see whether we returned anything.
    this->write_output(input);
  }

  private:

  iterator_type start() const;
  void start(iterator_type) const;

  // TODO:
  //   Unspecified for now.  May become a simple return -- we have to see how
  //   the sequence works out.
  packet *read_input();
  void write_output(packet *p);

  // packet *pipe_input() { return pipes_.in().read(); }
  // void pipe_input(packet *p) { return pipes_.out().write(p); }

  //! Also resets buffering stage next time if necessary.
  packet *debuffer_input() {
    const stage_value_type ret = NERVE_CHECK_PTR(*this->start())->debuffer();

    NERVE_ASSERT(! ret.empty(), "empty data from a buffering stage is forbidden");

    if (! ret.buffering()) {
      reset_start();
    }

    return ret.packet();
  }

  stages_type &stages() { return stages_; }
  // junction &pipes() { return pipes_; }

  stages_type &stages_;
  // junction pipes_;
  iterator_type start_;
};

} //ns pipeline

#endif
