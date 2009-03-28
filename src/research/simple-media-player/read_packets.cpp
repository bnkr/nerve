#define BDBG_TRACE_DISABLE_TRC
#include <bdbg/trace/short_macros.hpp>

#include "read_packets.hpp"
#include "aligned_memory.hpp"
#include "shared_data.hpp"

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

// TODO:
//   somewhere around this area I need to test the size of the queue and
//   if it's too big wait for the callback to notify me.  Of course this
//   requires another monitor usage.  Which gives us a problem really. I
//   have used the sync types already and they only have a single wait
//   condition.  Well...  I could wait on the existing one actually...
//   but there's definitely the possibility for more than one condition
//   to be necessary for back-and-forth communication.

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

  TODO:
    it seems weird that ffmpeg's own data is not OK to put directly
    into the decoder.
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
      //   will work, I should abstract it.  This and the working_data and process_data
      //   stuff should all go in a stateful object.  We just repeatedly pull blocks
      //   from the obj like, while (x.has_blocks()) { queue.push(x.read_block()); }
      //   Of course we need it up a scope because it stores data between frames.
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
        // TODO: what happens here?
        std::cout << "less bytes read than the input size: " << count << " vs. " << fr.size() << std::endl;
      }

      // further  processing on audio buffer here.

      process_data(working_buffer, output_buf, output_buf_size);
      break;
    } while (true);

  } while (true);

  // put the remainder of the working buffer on the stream.
  finished = true;
}
