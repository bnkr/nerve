// Copyright (C) 2009-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_pipes
 *
 * Inter-process communication pipes.
 */

#ifndef PARA_PIPES_HPP_773ia2gj
#define PARA_PIPES_HPP_773ia2gj

#include "syncs.hpp"
#include "monitors.hpp"

#include <deque>
#include <boost/bind.hpp>

namespace para {
  //! \ingroup grp_pipes
  //! Using somthing other than this class means you can deal with allocation
  //! etc.
  struct simple_queue_selector {
    template<class T>
    struct queue { typedef std::deque<T> type; };
  };

  // TODO: should pass a pipe_types with function types and so on

  //! \ingroup grp_pipes
  //! A thread-safe non-iterable queue.
  template<class T, class MonitorSync, class QueueSelector = simple_queue_selector>
  class pipe {
    private:
    typedef typename QueueSelector::template queue<T>::type queue_type;
    typedef MonitorSync sync_type;
    typedef para::monitor<sync_type> monitor_type;
    typedef pipe<T,MonitorSync,QueueSelector> self_type;

    public:
    typedef typename queue_type::value_type value_type;
    typedef typename queue_type::reference reference_type;

    /*
    struct mover {
      data_pipe_mover(monitor_type &sync, queue_type &queue) {
      }

      monitor_type &sync_;
      queue_type   &queue_;
    };

    static mover move(pipe<T> &p) { return mover(p.sync_, p.queue_); }
    */

    pipe() {}

    /*
    pipe(mover &m)
    : sync_(), queue_(m.queue_)
    { }
    */

    //! Pop from the front of the queue.
    value_type read() {
      monitor_type m(sync_, boost::bind(&self_type::read_pred, this));
      value_type ret = queue_.front();
      queue_.pop_front();
      return ret;
    }

    //! Flush the pipe.
    void clear() {
      monitor_type m(sync_, boost::bind(&self_type::write_pred, this));
      queue_.clear();
    }

    //! Push onto the pipe.
    void write(const reference_type copy) {
      monitor_type m(sync_, boost::bind(&self_type::write_pred, this));
      queue_.push_back(copy);
    }

    //! Replace the queue's contents with the new data.
    void write_clear(const reference_type copy) {
      typedef typename sync_type::scoped_lock_type lock_type;
      lock_type lock(sync_.lockable());
      queue_.clear();
      queue_.push_back(copy);
      sync_.waitable().notify_one();
    }

    bool read_pred() const { return ! queue_.empty(); }
    bool write_pred() const { return queue_.size() < 10; }

    private:
    sync_type  sync_;
    queue_type queue_;
  };

  /*

  //! \ingroup grp_pipes
  //! A connection between two threaded pipes which binds one pipe to be the input
  //! and another to be the output.
  template<class T>
  class pipe_junction {
    public:
    typedef typename para::pipe<T>    pipe_type;
    typedef pipe_type::value_type     value_type;
    typedef pipe_type::reference_type reference_type;

    pipe_junction(pipe_type &in, pipe_type &out)
    : in_(in), out_(out) {}

    pipe_type &in() { return in_; }
    pipe_type &out() { return out_; }

    value_type read_input() { return this->in().read(); }
    void write_output(const reference_type copy) { this->in().write(copy); }

    private:
    pipe_type &in_;
    pipe_type &out_;
  };

  */
}
#endif
