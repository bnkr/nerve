#include "chunkinate.hpp"
#include "../../wrappers/ffmpeg.hpp"
#include "shared_data.hpp"

#include <boost/thread.hpp>

#include <bdbg/trace/short_macros.hpp>

bool queue_small_enough() {
  return synced_queue.data().size() < 15;
}

// TODO:
//   I think the packet queue should be part of an object which wraps all
//   of this thread sharing stuff.
void push_packet(void *sample_buffer) {
  synced_type::value_type &q = synced_queue.data();
  {
    // trc("push packet " << sample_buffer);
    boost::unique_lock<synced_type::lockable_type> lk(synced_queue.mutex());
    // Problem is that the fucking queue isn't pooled either.  I'll have to
    // make a better one.
    q.push(sample_buffer);
    synced_queue.wait_condition().notify_one();
  }

  // TODO:
  //   could I make an additon_monitor? or something to extend an
  //   already acquired lock.
  //
  // while (queue.size() > 15) {
  //   lk(synced_queue.mutex());
  //   synced_queue.wait_condition().wait(lk);
  //   monitor_type m(synced_queue, &queue_small_enough);
  // }
}

// TODO:
//   clearly the chunkinate functions should be part of an object.

void chunkinate_file(ffmpeg::packet_state &state, const char * const file_name, bool dump_to_file) {
  // this would be replaced by some way of waiting until the playlist has members again.
  finished = false;

  trc("chunking " << file_name);
  trc("packetptr at " << state.ptr());
  trc("index is " << state.index());

  ffmpeg::file file(file_name);
  file.dump_format(file_name);
  ffmpeg::audio_stream audio(file);
  // TODO:
  //   if the format is not right then we need to reconfigure the sound card (which is
  //   not possible yet).  At least we need to check that the sound output has the right
  //   characteristics to play the media.

  while (true) {
    ffmpeg::frame fr(file);
    if (fr.finished()) {
      trc("frame is finished!");
      break;
    }

    // TODO:
    //   bug - if you drop all frames, then we wait for the exit signal forever.  Fixed now?

    // TODO:
    //   A deprecated function made this very tricky.  Now, the decoder must be
    //   passed the packet directly - we lost some nice abstraction.  A solution
    //   might be:
    //
    //     decoder dec(state, audio);
    //     fr.decode(dec);
    //
    //   Problem is I made the decoder stateful and I shouldn't have.  We need:
    //
    //     frame fr(file);
    //     fr.decode_entire_thing(stream, push_function);
    ffmpeg::audio_decoder decoder(state, audio);
    decoder.decode(fr);

    // TODO:
    //   Crappy way of doing it.  Better:
    //
    //     decoder.each_packet(push_function);
    //
    //   Or really, when we get plugins, this mode should not be necessary at
    //   all.  There will be a plugin pipeline stage which does the chunking.
    void *sample_buffer = decoder.get_packet();
    while (sample_buffer != NULL) {
      if (dump_to_file) {
        fwrite(sample_buffer, sizeof(uint8_t), state.size(), dump_output_file);
      }
      // trc("got buf: " << sample_buffer);
      push_packet(sample_buffer);
      sample_buffer = decoder.get_packet();
    }
  }

  trc("finished chunking " << file_name);
  trc("packetptr at " << state.ptr());
  trc("index is " << state.index());
}

void chunkinate_finish(ffmpeg::packet_state &state, bool dump_to_file) {
  trc("final packet");

  // partial dump.

  size_t buffer_size = state.size() - state.index();


  // quite messy here... obv chunkinate should be a struct with the packet_state member.
  // trc("packet is currently " << state.ptr());
  void *p = state.get_final();
  // trc("after clear, it is " << state.ptr());
  // trc("we got " << p);

  if (dump_to_file) {
    fwrite(p, sizeof(uint8_t), buffer_size, dump_output_file);
    fsync(fileno(dump_output_file));
  }

  push_packet(p);

  // we didn't bother iwht the thread if file dumping.
  boost::unique_lock<boost::mutex> lk(synced_queue.mutex());
  finished = true;
  // This is for the pathalogical case where we output no frames the output
  // function will block forever unless we do this notify.
  synced_queue.wait_condition().notify_all();
}

