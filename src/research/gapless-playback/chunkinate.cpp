#include "chunkinate.hpp"

#include <nerved_config.hpp>
#include "../../wrappers/ffmpeg.hpp"
#include "shared_data.hpp"
#include "dump_file.hpp"
#include "degapifier.hpp"
#include "packet_state.hpp"

#include <boost/thread.hpp>

#include <bdbg/trace/short_macros.hpp>

bool queue_small_enough() {
  return synced_queue.data().size() < 15;
}

// TODO:
//   I think the packet queue should be part of an object which wraps all
//   of this thread sharing stuff.
//
// NOTE:
//   Regarding pooling, the stl list is already pooled *but* I can't control
//   locality for it, nor can I do a reduced allocate which just uses blocks
//   from a stack.  The best solution might be a private inheriteed list, which
//   gets rid of all the multiple-block allocating functions, but still uses an
//   pool... only problem is we really want to tag each pool differently...
//   meh this is all such low level stuff it's not clear exactly what is the
//   cost of doing it.
void push_packet(void *sample_buffer) {
  synced_type::value_type &q = synced_queue.data();
  {
    // trc("push packet " << sample_buffer << " to queue " << (void*) &q);
    // trc("lock is: " << (void*) &synced_queue.mutex());

    boost::unique_lock<synced_type::lockable_type> lk(synced_queue.mutex());

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

void chunkinate_file(packet_state &state, const char * const file_name, bool dump_to_file) {
  // this would be replaced by some way of waiting until the playlist has members again.
  finished = false;

  trc("chunking " << file_name);
  trc("packetptr at " << state.ptr());
  trc("index is " << state.index());

  ffmpeg::frame pkt;
  degapifier degap(state);
  ffmpeg::file file(file_name);
  file.dump(); //file_name);
  ffmpeg::audio_stream audio(file);
  // TODO:
  //   if the format is not right then we need to reconfigure the sound card (which is
  //   not possible yet).  At least we need to check that the sound output has the right
  //   characteristics to play the media.

  ffmpeg::packet_reader pr(pkt, file);
  while (pr.read()) {
    // TODO:
    //   bug - if you drop all frames, then we wait for the exit signal forever.  Fixed now?

    ffmpeg::audio_decoder decoder(audio);
    ffmpeg::decoded_audio decoded(decoder, pkt);
    degap.degapify(decoded);

    // have to do this bollocks because of the packet state nonsense - really
    // the output plugin should be doing this.
    void *sample_buffer;
    while ((sample_buffer = degap.get_packet()) != NULL) {
      if (dump_to_file) {
        fwrite(sample_buffer, sizeof(uint8_t), state.size(), dump_output_file);
      }

      push_packet(sample_buffer);
    }
  }

  trc("finished chunking " << file_name);
  trc("packetptr at " << state.ptr());
  trc("index is " << state.index());
}

void chunkinate_finish(packet_state &state, bool dump_to_file) {
  trc("final packet");

  // partial dump.

  size_t buffer_size = state.size() - state.index();


  // quite messy here... obv chunkinate should be a struct with the packet_state member.
  // trc("packet is currently " << state.ptr());

  // TODO:
  //   this is broken due to the degapifier.  We need to access that to get the
  //   actual data back.
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

