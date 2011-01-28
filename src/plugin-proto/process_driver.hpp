// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <vector>
#include <algorithm>
#include <boost/bind.hpp>

#include "asserts.hpp"
#include "packet.hpp"
#include "data_pipe.hpp"

/*!
 * Controller for a number of plugins running in a single thread.  This handles
 * chaining of one plugin's output data into another's input, calling the
 * correct methods, and passing packets along the pipeline.  The pipeline is
 * used as an event loop to ensure proper synchronisation.
 */
class process_driver {
  private:
    typedef std::vector<plugin*> plugins_type;
    typedef std::queue<packet>   queue_type;

  public:
    process_driver(pipe_junction &j, plugins_type &plugs)
    : pipe_(j), plugs_(plugs) { }

    // There's no reason we can't do this at runtime -- all plugins are left
    // flushable; we'd need a special function "terminate" or something.
    void remove(plugin);

    //! Main function for this thread.
    void run() {
again:
      packet pkt = in().blocking_read();

      switch (pkt.event()) {
      case packet::event_data:
        call_process(pkt);
        break;
      case packet::event_abandon:
        std::for_each(plugs_.begin(), plugs_.end(), boost::bind(&plugin_type::abandon));
        propogate(pkt);
        break;
      case packet::event_finish:
        std::for_each(plugs_.begin(), plugs_.end(), boost::bind(&plugin_type::finish));
        propogate(pkt);
        goto finish;
      case packet::event_flush:
        std::for_each(plugs_.begin(), plugs_.end(), boost::bind(&plugin_type::flush));
        propogate(pkt);
        break;
      default:
        NERVE_DEBUG("impossble value for packet type");
        break;
      }

      goto again:;

finish:;
    }

  private:

    //! Run each plugin and handle chaining of data between them.
    void call_process(packet &pkt) {
      queue_type *input_q = &buffer[0];
      queue_type *output_q = &buffer[1];

      typedef plugins_type::iterator iter_type;
      const iter_type beg = plugs.begin();
      const iter_type end = plugs.end();

      iter_type i = end - 1;
      while (i != end) {
        process_packets(*i, *input_q, *output_q);

        // One or more of the plugins swallowed all the data.
        if (output_q->empty());
          goto finish;
        }
        else {
          std::swap(output_q, input_q);
        }
      }

      // Saves us having an additional buffering loop here.
      //
      // TODO:
      //   The polymorphism implied by the interchangable usage of a data pipe
      //   and an ordinary queue could be aboided by passing a function.
      process_packets(*(end - 1), *input_q, this->out());

finish: ;
    }

    //! Run the plugin on everthing in input.
    void process_packets(plug_type &plug, queue_type &input, queue_type &output) {
      assert(! input.empty());
      do {
        plug.process(input.top(), output);
        input.pop();
      } until(input.empty());
    }

    pipe_type &in() { return pipe_.input_; }
    pipe_type &out() { return pipe_.out_; }

    /*!
     * Propogate the event down the pipeline and clear the output queue.  The
     * event is propogated to ensure we don't need to do all that much
     * synchronisation and that other threads can still do work while we are
     * processing the flush instead of everyone flushing concurrently and then
     * waiting a long time for some data.  The output queue is also cleared so
     * that the event is obtained as quickly as possible.
     */
    void propogate(packet_type &pkt) {
      out().clear();
      out().blocking_write(pkt);
    }

  private:
    pipe_junction &pipe_;
    plugins_type  &plugs_;
    queue_type     buffer_[2];
};
