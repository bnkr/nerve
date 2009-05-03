#define BDBG_TRACE_DISABLE_TRC
#include <bdbg/trace/short_macros.hpp>

#include "read_packets.hpp"
#include "../../include/aligned_memory.hpp"
#include "shared_data.hpp"

void push_packet(void *sample_buffer) {
  synced_type::value_type &q = synced_queue.data();
  {
    boost::unique_lock<synced_type::lockable_type> lk(synced_queue.mutex());
    // Problem is that the fucking queue isn't pooled either.  I'll have to
    // make a better one.
    q.push(sample_buffer);
    synced_queue.wait_condition().notify_one();
  }

  // TODO:
  //   need to find a way to limit the size of the queue.  I guess we need to
  //   wait on the same conditoon. I can actually wait on the existing ones.
}

void read_packets(ffmpeg::file &file, ffmpeg::audio_stream &audio_stream, const sdl::audio_spec &audio_spec) {

  // so messy...
  finished = false;
  std::size_t sdl_buffer_size = audio_spec.buffer_size();
  assert(sdl_buffer_size > 0);

  ffmpeg::packet_state state(audio_spec.buffer_size(), audio_spec.silence());

  // x because it's fixed and I can't be bothered to rename the vars :)
  ffmpeg::audio_decoder decoder(state, audio_stream);

  do {
    ffmpeg::frame fr(file);
    if (fr.finished()) {
      trc("Finished!");
      // in the next version I need to swap over to the next file here and continue
      // on to the next buffer.  We can just move the file object up loop prolly?
      // Mixing them together is a whole other mission though.
      break;
    }

    decoder.decode_frame(fr);
    void *sample_buffer = decoder.get_packet();
    while (sample_buffer != NULL) {
      push_packet(sample_buffer);
      sample_buffer = decoder.get_packet();
    }
  } while (true);

  void *p = decoder.get_final_packet();
  push_packet(p);

  // put the remainder of the working buffer on the stream.
  finished = true;
}
