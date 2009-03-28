#include "chunkinate.hpp"
#include "ffmpeg.hpp"
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

  ffmpeg::audio_decoder decoder(state, audio);
  while (true) {
    ffmpeg::frame fr(file);
    if (fr.finished()) {
      trc("frame is finished!");
      break;
    }

    // TODO:
    //   Somewhere around here I need to get rid of the gaps.  I will probably need to
    //   know about the stream context since I am going to need to know about the end
    //   of  the frame.  I am also going to need to process the stream *before* it
    //   gets pushed into the state.  Therefore, somewhere in get_packet() or decode().
    //
    //   decode() - good choice as I can work backwards and just truncate the buffer.
    //     I must know if it's the last frame in the file.
    //
    //     Actually decode() is the only choice.  If the frame is entirely silence then
    //     I can just drop the entire frame there and then.  I guess the real problem is
    //     dropping silent frames when I don't want them dropped.
    //
    //   get_packet() - might need to do it anyway if I have generalised stream
    //     processing because that means you can process in even sized chunks which
    //     might be desireable.
    //
    // It should probably be something like
    //
    // ffmpeg::audio_decoder dec(fr);
    // // one of which is 'if last frame, truncate buffer'
    // dec.preprocess(plugins_list);
    //
    // each {|sample_block|
    //   block_process(sample_block);
    //   observer_process(sample_block);
    // }
    //
    // An interesting extension would be to organise a multi-threaded
    // processing pipeline.  Each stage has a monitored queue for
    // packets to mess with.
    //

    // TODO:
    //   the decoder should be declared here now I have detached the state.
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
  finished = true;
}

