#include "ffmpeg.hpp"
#include "sdl.hpp"

#include "play.hpp"
#include "load_file_test.hpp"
#include "aligned_memory.hpp"

#include <iostream>
#include <fstream>

#include <pthread.h>
#include <cstdlib>

#define BDBG_TRACE_DISABLE_ALL
#include <bdbg/trace/short_macros.hpp>
#include <bdbg/trace/static_definitions.hpp>

#include <algorithm>

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>

#if SIMPLE_WAVE_FILE_READER
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::ifstream in;

int amount_read = 0;
std::size_t file_buffer_size = 0;
uint8_t *file_buffer = NULL;

int buffer_file() {
  in.read((char*) file_buffer, file_buffer_size);
  amount_read = in.gcount();
}

// calls buffer_file
void file_reading_callback(void *userdata, uint8_t *stream, int length) {
  // also need to check that I have played the entire buffer here when buffering is
  // done in the  other thread.
  if (in.eof() /* && ! data_queue_empty */) {
    std::cout << "notify eof" << std::endl;
    int r = pthread_cond_signal(&cond);
    assert(r == 0);
    // just in case the callback is called again before the shutdown happens
    amount_read = 0;
    return;
  }

  if (amount_read < length) {
    // Zero a partial buffer if there is one.
    std::memset(stream + amount_read, 0, length - amount_read);
  }
  std::memcpy(stream, file_buffer, amount_read);
  // SDL_MixAudio(stream, file_buffer, len, SDL_MIX_MAXVOLUME);

  buffer_file();
}

#endif


std::size_t sdl_buffer_size = 0;

#include <queue>
#include <para/locking.hpp>
// TODO:
//   two problems here.  One is the void*.  I need to use an aligned memory
//   class here.  The second is the queue accessor.  Maybe I use
//

typedef para::sync_traits<std::queue<void *>, boost::mutex, boost::unique_lock<boost::mutex>, boost::condition_variable> sync_traits_type;
typedef sync_traits_type::monitor_tuple_type synced_type;
synced_type synced_queue;

// messy - prolly needs a bool callback_finished_printing_data.
boost::mutex finish_mut;
boost::condition_variable finish_cond;

bool finished = false;

// do we stop waiting?
bool continue_predicate() {
  // TODO:
  //   problematic to say the least.  I wanted data to be untouched
  //   outside of lockness.  Well, I might have to pass the data to
  //   the predicate?  It's messay though.
  return finished || ! synced_queue.data().empty();
}

// nevr mind this for a while.  Check boost anyway.  The crucial part of
// this is the ordering.  Of course I need to implement a stack inside the
// pool.
//
//! \brief Pooled allocator which allocates blocks from a pool.
//TODO:
//  It's better to make class T a buffer type.  Also, it's tricky to
//  deal with alignment in this class.  Allocating aligned buffers
//  is inefficant - it would be much better to align the base ptr
//  and only allocate aligned addresses.  This is OK if the size of
//  the blocks is not very bifg
template <class T>
class stacked_block_pool_allocator {
  public:
    stacked_block_pool_allocator(std::size_t block_size = 0) {
    }

    T *allocate(std::size_t num = 1) { return new T[1]; }
    void deallocate(std::size_t num, T *ptr) { delete[] ptr; }
};

// typedef   block_type;
// typedef stacked_block_pool_allocator<block_type>  pool_type;

// derp...no.  Block size must be dynamic due to different SDL buffer
// sizes.  this must be in the algorithm's scope.
// pool_type pool();

//! \brief Read buffers from a queue and push them to the output stream.
void ffmpeging_callback(void *userdata, uint8_t *stream, int length) {
  typedef sync_traits_type::monitor_type monitor_type;

  // trc("callback");
  void *buffer = NULL;
  {
    monitor_type m(synced_queue, &continue_predicate);
    // trc("woke up");

    // TODO: anoying.  Two interfaces to the data.  Monitored<T> will fix it I guess.
    synced_type::value_type &q = synced_queue.data();
    if (finished && q.empty()) {
      // TODO:
      //   accesses to terminate must be sunchronised as well - ctonainer_type
      //   should actually be a type which has this added.
      // wait for more data

      {
        // TODO:
        //   it blocks forever if the decoding thread is slower than this
        //   one.  The main thread needs to monitor a boolean which we set to
        //   "I've quit".
        trc("notifying exit");
        boost::unique_lock<boost::mutex> lk(finish_mut);
        finish_cond.notify_all();
        boost::thread::yield();
      }

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
  std::free(buffer);
}

// it would make sense to hold this in the audio decoder object
struct processing_data {
  uint8_t *samples;
  std::size_t index;
  const std::size_t size;

  processing_data(std::size_t buffer_size) : size(buffer_size) {
    samples = (uint8_t *) std::malloc(size);
    index = 0;
  }
};

void process_data(processing_data &working_buffer, const void *output, const std::size_t output_buf_size) {
  const uint8_t *output_buf = (uint8_t *) output;
  trc("* Process a frame.");
  std::size_t index = 0;
  while (index < output_buf_size) {
    trc("continue filling the sample buffer starting at position " << working_buffer.index << " from the stream buf at pos " << index);
    uint8_t *sample_buffer = &working_buffer.samples[working_buffer.index];
    std::size_t bytes_left = working_buffer.size - working_buffer.index;
    std::size_t stream_available = output_buf_size - index;
    std::size_t copy_len = std::min(stream_available, bytes_left);
    std::size_t uncopied_bytes = stream_available - copy_len;

    trc("from a size of " << working_buffer.size << ", we have " << bytes_left << " bytes to fill.");
    trc("we start copying from the stream at " << index);
    trc("the stream has " << stream_available << " bytes available.");
    trc("we will therefore copy " << copy_len << " bytes");
    trc("there will be " << uncopied_bytes  << " bytes left in the stream.");

    std::memcpy(sample_buffer, &output_buf[index], copy_len);
    index += copy_len;
    working_buffer.index += copy_len;

    trc("stream index has become " << index);
    trc("output index has become " << working_buffer.index);

    assert(working_buffer.index <= working_buffer.size);

    if (working_buffer.index == working_buffer.size) {
      trc("* working buffer is full");
      // No need to monitor wait, just push the data as soon as we get a lock
      // TODO:
      //   clearly this has added nothing over using standard synchronisation.  monitoed<T>
      //   makes it a bit better, but at least having a nicer way to obtain that uinque_lock
      //   type makes it better.  Probably sync_traits should place them somewhere visible.
      //   Perahps we want a writer_ptr which notifies the wait_condition?
      synced_type::value_type &q = synced_queue.data();
      {
        boost::unique_lock<synced_type::lockable_type> lk(synced_queue.mutex());
        // Problem is that the fucking queue isn't pooled either.  I'll have to
        // make a better one.
        q.push(working_buffer.samples);
        trc("after push, queue size is now " << q.size());
        synced_queue.wait_condition().notify_one();
      }

      // std::cerr << "narf... queue size: " << q.size() << std::endl;
      // boost::system_time abs_time = boost::get_system_time() + boost::posix_time::milliseconds(10);
      // boost::thread::sleep(abs_time);


      // use a pool of course.  Not that it helps much.
      working_buffer.samples = (uint8_t*) std::malloc(working_buffer.size);
      // std::memset(working_buffer.samples, 0, working_buffer.size);
      working_buffer.index = 0;
    }

    trc("index into output buffer is " << index << ".  Continue = " << (index < output_buf_size));
  }
}

void read_packets(ffmpeg::file &file, ffmpeg::audio_stream &s) {
  /*
  From the docs:

  For the input buffer we over allocate by FF_INPUT_BUFFER_PADDING_SIZE
  because optimised readers will read in longer bitlengths.  We never
  actually read data up to that length and the last byte must be zero
  (again ffmpeg doesn't always do that).

  Output must be 16-byte alligned because SSE needs it.

  Input must be "at least 4 byte aligned".  ffmpeg doesn't always do it
  in data.

  Finally, the output must be at least AVCODEC_MAX_AUDIO_FRAME_SIZE.
  */

  // so messy...
  finished = false;
  ffmpeg::audio_decoder decoder(s);

  assert(sdl_buffer_size > 0);
  processing_data working_buffer(sdl_buffer_size);

  const std::size_t buffer_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
  // assumption is that it needs some N of 16 bit integers.
  assert(buffer_bytes % sizeof(int16_t) == 0);
  const std::size_t buffer_size = buffer_bytes / sizeof(int16_t);
  const std::size_t alignment = 16;
  aligned_memory<alignment, buffer_size, int16_t> buffer;

  do {
    // TODO:
    //   need to find a way to limit the size of the queue.  I guess we need to
    //   wait on the same conditoon.
    ffmpeg::frame fr(file);
    if (fr.finished()) {
      trc("Finished!");
      // in the next version I need to swap over to the next file here and continue
      // on to the next buffer.  What happens if the last iteration there was only
      // a partial-buffer?
      break;
    }

    // TODO:
    //   see audio_decode_frame from the tute.  Complicatedness with
    //   static variable.

    //TODO: use a pool
    // uint8_t *output_buf = (uint8_t *) pool.allocate();

    do {
      int16_t *output_buf = buffer.ptr();
      int output_buf_size = buffer_bytes;
      // TODO:
      //   data(), size() might have to be aligned, and also the last byte should
      //   be set null.  Why doesn't ffmpeg do it, though.  It's their bloody data
      //   and I don't want to copy it!

      // TODO:
      //   this is not a nice interface.  When I have more data about how this
      //   will work, I should abstract it.
      int count = decoder.decode(buffer.ptr(), &output_buf_size, fr.data(), fr.size());
#if 0
      std::cout << "Read data:" << std::endl;
      std::cout << "  bytes inputted:      " << fr.size() << std::endl;
      std::cout << "  output buffer size:  " << buffer_bytes << std::endl;
      std::cout << "  bytes of input used: " << count << std::endl;
      std::cout << "  output buf size:     " << output_buf_size << std::endl;
#endif

      // Skip error frame.
      if (count < 0) {
        trc("skip error frame");
        break;
      }

      // Data not ready yet.
      if (output_buf_size <= 0) {
        trc("nothing to decore");
        continue;
      }

      if (count < fr.size()) {
        std::cout << "less bytes read than the input size: " << count << " vs. " << fr.size() << std::endl;
        // TODO: what happens here?
      }

      // further  processing on audio buffer here.
      {
        // split/combine it in to sdl-sized blocks ready for output


        process_data(working_buffer, output_buf, output_buf_size);

        break;
      }

    } while (true);

  } while (true);

  // put the remainder of the working buffer on the stream.
  finished = true;
}

void play(const char * const file) {
  // load_file_test(file); return;


  assert(file != NULL);
  try {
    std::cout << "Begin, file is: " << file << std::endl;
    sdl::audio aud;
    std::cout << "Audio status is: " << aud.status_name() << std::endl;
    sdl::device dev(aud);
    ffmpeg::initialiser ff;
    ffmpeg::file ffile(file);
    ffile.dump_format(file);
    ffmpeg::audio_stream audio(ffile);

#if SIMPLE_WAVE_FILE_READER
    in.open(file);
    assert(in.good());
#endif

    {
#if SIMPLE_WAVE_FILE_READER
      sdl::audio_spec::callback_type cb = &file_reading_callback;
#else
      sdl::audio_spec::callback_type cb = &ffmpeging_callback;
#endif
      sdl::audio_spec desired(cb);
      std::cout << "Opening audio." << std::endl;
      std::cout << "wanted:\n" << desired << std::endl;
      dev.reopen(desired);
      std::cerr << "SDL configuration:\n" << dev.obtained() << std::endl;
      assert(desired.buffer_size() == dev.obtained().buffer_size());
      sdl_buffer_size = dev.obtained().buffer_size();
    }

    std::cout << "pre-buffering" << std::endl;
#if SIMPLE_WAVE_FILE_READER
    file_buffer_size = dev.obtained().buffer_size();
    // TODO: raii of course.
    file_buffer = (uint8_t *) std::malloc(sizeof(uint8_t) * file_buffer_size);
    // Discard the 44 byte .wav header.
    in.read((char *) file_buffer, 44);
    std::memset(file_buffer, 0, file_buffer_size);
    buffer_file();
#else
    // TODO:
    //   call it a coule of times?  Eventually there'll be special routine for it I
    //   guess... need to abstract it and just buffer N packets if there are packets
    //   to buffer.
#endif
    std::cout << "playing" << std::endl;

    dev.unpause();

#if SIMPLE_WAVE_FILE_READER
    std::cout << "unpaused; now waiting" << std::endl;
    int r = pthread_mutex_lock(&mutex);
    assert(r == 0);
    std::cout << "locked" << std::endl;
    r = pthread_cond_wait(&cond, &mutex);
    sdl::audio_lock_guard lock_for_shutdown(dev);
    assert(r == 0);
    std::cout << "got notified" << std::endl;
    r = pthread_mutex_unlock(&mutex);
    assert(r == 0);
#else
    read_packets(ffile, audio);

    std::cout << "wait to be told to exit" << std::endl;
    boost::unique_lock<boost::mutex> lk(finish_mut);
    finish_cond.wait(lk);
#endif

    std::cout << "shutting down" << std::endl;
  }
  catch (sdl::init_error &e) {
    std::cerr << "Error: could not initialise sdl: " << e.what() << std::endl;
    exit(1);
  }
  catch (sdl::open_error &e) {
    std::cerr << "Error: could not open the sound card: " << e.what() << std::endl;
    exit(1);
  }

#if SIMPLE_WAVE_FILE_READER
  std::free(file_buffer);
#endif

  std::cout << "finished!" << std::endl;
}

// ** notes on the gapless version ** //
//
// TODO:
//   In the gapless version, I'll need to mix the two buffers together.  This
//   indicates that I will absolutely have to organise the buffering in another
//   thread.
//
//   Still... it needs to be synchronus anyway... I absolutely must avoid any
//   kind of stutter; if that means doing all the loading in this thread then
//   that's fine.
//
// TODO:
//   Another problem: what about when the sample format changes mid-stream?  I
//   guess we need to convert it... we can't really re-initialise the stream,
//   can we?  SDL's audioCVT thing might help...
//
// TODO:
//   Abstracted output?  I can't just replace the callback because the output
//   might might have a different interface.  Also, how do I make it *fast* -
//   I am going to need to compose all these stream manipulator functions.  If
//   there are a tonne of function calls it's going to be pretty messy or have
//   a tonne of maintannce because I need to create a new composed callback for
//   every output type.
//
//   Abstracted input is much easier if it's all done in the main thread.
//
//   Playlists should be a simple exetension to gapless playback, except that
//   sometimes the playlist can be empty and we have to keep waiting (and
//   preferably) cause the player thread to block, rather than keep polling.
//
// TODO:
//   When the playlist is finished, I must play silence; some soundcards pop if
//   that is not done.  I guess the answer is just to dump one period of silence
//   in whenever the playlist is empty?  If we get a new one then we can nuke the
//   silence... also there's still the probelm of partial buffers...
//
// TODO:
//   Other libs do not involve their own audio thread.  I will need to account for that
//   somehow.
