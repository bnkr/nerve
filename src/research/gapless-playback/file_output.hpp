/*!
\file
\brief Output raw audio data to a file.

I guess this serves as a prototype for generic output engines.
*/

#error Doesn't work.  Don't include.  Use the hack.  Argh.

#include "shared_data.hpp"

#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <bdbg/trace/short_macros.hpp>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

// for some fucking stupid reason I can't pass this as an argument
// to the thread.
std::ofstream out;
FILE *outfile;
// TODO:
//   really I want just the function here, tbh.  Also a nice way would
//   be to have a thread-makign wrapper and various helpers to.
//
// this is identical to the sdl outputter.  Maybe I can abstract all
// the monitor stuff and just return buffers.   The ideal would be
// that when the input is over, I wait forever inside the monitor
// bit and stay there until the thread is finally closed.  Then the
// entire code is:
//
//   // I guess a real output will always want these structs.
//   void initialise(spec, *got_spec) {
//   }
//
//   // will be necesasy I'm sure - tricky because we need to wait until
//   // the very end of the sond before we actually organise this.  Means
//   // that the output callback needs to wait somehow.  Either that or
//   // we use libsamplerate to resample.  Currently I think that reinitting
//   // for each track is going to cause issues because some sound cards
//   // won't support the right rate.  Yah.  Sample rate is the only way
//   // to go.
//   void reinitialise(spec, *got_spec) {}
//
//   void my_output_plugin(void *output, std::size_t length) {
//     again:
//     buffer = pop_buffer();
//     std::memcpy(output, buffer, length);
//   }
//
//   void shutdown() {
//   }
//
// The real choice is whether to use C++ or C.
void dump_to_file(std::size_t buffer_len) {
  typedef sync_traits_type::monitor_type monitor_type;

  // trc("callback");
  void *buffer = NULL;
  while (true) {
    {
      trc("wait here...");
      monitor_type m(synced_queue, &continue_predicate);
      trc("woke up");

      synced_type::value_type &q = synced_queue.data();
      if (finished && q.empty()) {
        {
          // unconditional monitor - it's a write_ptr again
          trc("notifying exit");
          boost::unique_lock<boost::mutex> lk(finish_mut);
          output_closed = true;
          finish_cond.notify_all();
        }
        boost::thread::yield();
        // out.close();
        return;
      }
      else {
        assert(! q.empty());
        buffer = q.front();
        q.pop();
      }
    }

    trc("write " << buffer  << " with length " << buffer_len);
    // fuck you bullshit iostreams
    // assert(out.is_open());
    // out.write((const char *)buffer, buffer_len);
    // assert(! out.bad());
    // trc("wrote: " << v);
    // std::free(buffer);
    //
    std::size_t w = fwrite(buffer, 1, buffer_len, outfile);
    assert(w == buffer_len);
    fflush(outfile);
#ifndef WIN32
    fsync(fileno(outfile));
#endif
  }
}

// for some reason boost bind fucks everything up
struct wrap {
  std::ostream &out_;
  std::size_t size_;
  wrap(std::ostream &o, std::size_t s)  : out_(o), size_(s) {}

  void operator()() {
    // dump_to_file(out_, size_);
  }
};

namespace output {
  //! \brief Output engine dumps data to a file.
  class file {
    public:
      file(const char *const filename, std::size_t period_size)
      :  buf_size_(period_size) {
        outfile = fopen("./dump.raw",  "wb");
        assert(outfile != NULL);
        boost::thread out_th(boost::bind(&dump_to_file, period_size));
      }

      ~file() {
        out.close();
      }

    private:
      std::size_t buf_size_;
  };
}
