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

void chunkinate_file(ffmpeg::packet_state &state, const char * const file_name) {
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

    ffmpeg::audio_decoder decoder(state, audio);
    decoder.decode(fr);

    void *sample_buffer = decoder.get_packet();
    while (sample_buffer != NULL) {
      // trc("got buf: " << sample_buffer);
      push_packet(sample_buffer);
      sample_buffer = decoder.get_packet();
    }
  }

  trc("finished chunking " << file_name);
  trc("packetptr at " << state.ptr());
  trc("index is " << state.index());
}

void chunkinate_finish(ffmpeg::packet_state &state) {
  trc("final packet");
  // quite messy here... obv chunkinate should be a struct with the packet_state member.
  trc("packet is currently " << state.ptr());
  void *p = state.get_final();
  trc("after clear, it is " << state.ptr());
  trc("we got " << p);
  push_packet(p);

  boost::unique_lock<boost::mutex> lk(synced_queue.mutex());
  finished = true;
  // This is for the pathalogical case where we output no frames the output
  // function will block forever unless we do this notify.
  synced_queue.wait_condition().notify_all();
}

