#include "output.hpp"

#include "shared_data.hpp"

#include <bdbg/trace/short_macros.hpp>



void sdl_callback(void*,uint8_t *stream, int length) {
  typedef sync_traits_type::monitor_type monitor_type;

  // trc("callback");
  void *buffer = NULL;
  {
    monitor_type m(synced_queue, &continue_predicate);
    // trc("woke up");

    synced_type::value_type &q = synced_queue.data();
    if (finished && q.empty()) {
      {
        // TODO:
        //   should use a proper monitor here, or at least a better way of
        //   storing these sync primitives.
        trc("notifying exit");
        boost::unique_lock<boost::mutex> lk(finish_mut);
        output_closed = true;
        finish_cond.notify_all();
      }
      boost::thread::yield();

      // normally we would just return from the thread here, but SDL controls
      // it so we have to wait for the main thread to shut sdl down.  We DON'T
      // wait on anything because otherwise we end up deadlocking; we might have
      // to deal with calling this code path a couple of times until SDL finally
      // terminates.
      return;
    }
    else {
      assert(! q.empty());
      buffer = q.front();
      q.pop();
    }
  }

  // trc("writing output data of length " << length);
  std::memcpy(stream, buffer, length);
  // pool.deallocate(buffer);
  // trc("freeing " << buffer);
  std::free(buffer);
}

