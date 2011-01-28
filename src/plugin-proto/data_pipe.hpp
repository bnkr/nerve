#include "para.hpp"

//! A thread-safe non-iterable queue.
template<class T>
class data_pipe {
  private:
    typedef typename std::queue<T>           queue_type;
    typedef basic_monitorable                monitorable_type;
    typedef scoped_monitor<monitorable_type> monitor_type;

  public:
    struct mover {
      data_pipe_mover(monitor_type &sync, queue_type &queue) {
      }

      monitor_type &sync_;
      queue_type   &queue_;
    };

    static mover move() { return mover(sync_, queue_); }

    data_pipe();
    data_pipe(mover &m) : sync_(), queue_(m.queue_) {
    }

    //! Flush the pipe.
    void clear() {
      scoped_monitor m(sync_);
      queue_.clear();
    }

    //! Pop from the front of the queue.
    value_type read() {
      scoped_monitor m(sync_);
      T ret = queue_.top();
      queue.pop();
      return ret;
    }

    void write(const reference_type copy) {
      scoped_monitor m(sync_);
      queue_.push(copy);
    }

  private:
    basic_monitorable sync_;
    queue_type        queue_;
};

//! A connection between two threaded pipes which binds one pipe to be the input
//! and another to be the output.
template<class T>
class pipe_junction {
  public:
    typedef typename data_pipe<T>     pipe_type;
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
