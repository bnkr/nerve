#define BDBG_TRACE_DISABLE_TRC
#include <bdbg/trace/short_macros.hpp>

#include "read_packets.hpp"
#include "aligned_memory.hpp"
#include "shared_data.hpp"

void read_packets(ffmpeg::file &file, ffmpeg::audio_stream &s, const sdl::audio_spec &audio_spec) {
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
  std::size_t sdl_buffer_size = audio_spec.buffer_size();
  assert(sdl_buffer_size > 0);

  // x because it's fixed and I can't be bothered to rename the vars :)
  ffmpeg::audio_decoder xdecoder(s, sdl_buffer_size);

  do {
    ffmpeg::frame fr(file);
    if (fr.finished()) {
      trc("Finished!");
      // in the next version I need to swap over to the next file here and continue
      // on to the next buffer.  We can just move the file object up loop prolly?
      // Mixing them together is a whole other mission though.
      break;
    }

    xdecoder.decode_frame(fr);
    void *sample_buffer = xdecoder.get_packet();
    while (sample_buffer != NULL) {
      synced_type::value_type &q = synced_queue.data();
      {
        boost::unique_lock<synced_type::lockable_type> lk(synced_queue.mutex());
        // Problem is that the fucking queue isn't pooled either.  I'll have to
        // make a better one.
        q.push(sample_buffer);
        trc("after push, queue size is now " << q.size());
        synced_queue.wait_condition().notify_one();
      }

      // TODO:
      //   need to find a way to limit the size of the queue.  I guess we need to
      //   wait on the same conditoon. I can actually wait on the existing ones.

      sample_buffer = xdecoder.get_packet();
    }
  } while (true);

// TODO: use xdecoder.get_final_packet();

  // put the remainder of the working buffer on the stream.
  finished = true;
}
