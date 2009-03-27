#include "ffmpeg.hpp"

#include "play.hpp"
#include "load_file_test.hpp"

#include "sdl.hpp"
#include <iostream>
#include <fstream>

#include <pthread.h>
#include <cstdlib>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::ifstream in;

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>


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

// boost::scoped_array<char> last_log;


#include <queue>
#include <para/locking.hpp>
// TODO:
//   two problems here.  One is the void*.  I need to use an aligned memory
//   class here.  The second is the queue accessor.  Maybe I use
//

typedef para::sync_traits<std::queue<void *>, boost::mutex, boost::lock_guard<boost::mutex>, boost::condition_variable> sync_traits_type;

typedef sync_traits_type::monitor_tuple_type synced_type;
synced_type synced_queue;



bool finished = false;

// do we stop waiting?
bool continue_predicate() {
  // TODO:
  //   problematic to say the least.  I wanted data to be untouched
  //   outside of lockness.  Well, I might have to pass the data to
  //   the predicate?  It's messay though.
  return ! queue.data().empty() || finished;
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
  while (true)  {
    // TODO: access_ptr is a silly name.  Call it monitored_ptr.
    container_type::value_type buffer = NULL;
    {
      monitor_type m(synced_queue, &continue_predicate);
      synced_queue::data_type &q = m.data();
      if (finished && q.empty()) {
        // wait for more data
        // TODO:
        //   accesses to terminate must be sunchronised as well - ctonainer_type
        //   should actually be a type which has this added.
        q.cond().wait(q.lock()); // abbreviate to q.wait();
        continue;
      }

      buffer = q->pop_back();
    }

    std::memcpy(stream, buffer, length);
    // pool.deallocate(buffer);
    free(buffer);
  }
}

namespace ffmpeg {
class stream_reader {
  public:
};

//! \brief Initialised by pulling a frame from a \link ffmpeg::file \endlink.
class frame {
  public:
    frame(ffmpeg::file &file) {
      ret_ = av_read_frame(&file.format_context(), &packet_);
      finished_ = (ret_ != 0);
    }

    ~frame() { av_free_packet(&packet_); }

    //! Stream finished?
    bool finished() const { return finished_; }

    const uint8_t *data() { return packet_.data; }
    int size() { return packet_.size; }

  private:
    AVPacket packet_;
    bool finished_;

};
}

void read_packets(ffmpeg::file file, ffmpeg::audio_stream &s) {
  /*
  From the docs:

  For the input buffer we over allocate by FF_INPUT_BUFFER_PADDING_SIZE
  because optimised readers will read in longer bitlengths.  We never
  actually read data up to that length.

  Output must be 16-byte alligned because SSE needs it.

  Input must be "at least 4 byte aligned".  ffmpeg doesn't always do it
  in data.
  */

  // TODO: won't always be this size.
  int sdl_buffer_size = 1024;

  // so messy...
  finished = false;
  // wtf do I need that for?
  // ffmpeg::stream_reader sr(s);
  do {
    // TODO:
    //   need to find a way to limit the size of the queue.  I guess we need to
    //   wait on the same conditoon.
    ffmpeg::frame fr(file);
    if (fr.finished()) {
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

    uint8_t *output_buf = (uint8_t *) std::malloc(sdl_buffer_size);

    do {
      int output_buf_size = sdl_buffer_size;
      // TODO:
      //   data(), size() might have to be aligned, and also the last byte should
      //   be set null.  Why doesn't ffmpeg do it, though.  It's their bloody data
      //   and I don't want to copy it!

      int count = fr.decode_audio((int16_t*) output_buf, &output_buf_size, fr.data(), fr.size());
      std::cout << "Read data:" << std::endl;
      std::cout << "  bytes read:" << count << std::endl;
      std::cout << "  output buf size:" << output_buf_size << std::endl;

      // Skip error frame.
      if (count < 0) {
        break;
      }

      // Data not ready yet.
      if (output_buf_size <= 0) {
        continue;
      }

      if (count < sdl_buffer_size) {
        std::cout << "less bytes read than are needed in the buffer:" << count << " vs. " << sdl_buffer_size  << std::endl;
        // TODO: what happens now?  Probably this will happen at the end of file...
      }

      if (count < fr.size()) {
        std::cout << "less bytes read than the input size: " << count << " vs. " << fr.size() << std::endl;
        // TODO: what happens here?
      }

      // further  processing on audio buffer here.

      {
        // No need to monitor wait, just push the data as soon as we get a lock
        guarded_type::locked_ptr q(queue);
        // Problem is that the fucking queue isn't pooled either.  I'll have to
        // make a better one.
        q->push(output_buf);

        break;
      }

    } while (true);

  } while (true);

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
    file.dump_format(file);
    ffmpeg::audio_stream audio(ffile);

#if SIMPLE_WAVE_FILE_READER
    in.open(file);
    assert(in.good());
#endif

    {
#if SIMPLE_WAVE_FILE_READER
      sdl::audio_spec::callback_type = file_reading_callback;
#else
      sdl::audio_spec::callback_type = ffmpeging_callback;
#endif
      sdl::audio_spec desired(file_reading_callback);
      std::cout << "Opening audio." << std::endl;
      std::cout << "wanted:\n" << desired << std::endl;
      dev.reopen(desired);
      std::cout << "got:\n" << dev.obtained() << std::endl;
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
    // TODO: do file reading here.
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
