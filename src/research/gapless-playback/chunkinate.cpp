#include "chunkinate.hpp"

#include <nerved_config.hpp>
#include "../../wrappers/ffmpeg.hpp"
#include "shared_data.hpp"
#include "dump_file.hpp"

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
    // TODO: this is also a really crappy way of doing things.
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
    //     decoder dec(fr, audio_stream);
    //     dec.decode(push_function);
    //
    //   The problem is I must keep the packet state somehow; otherwise I end up
    //   with gaps when the song byte boundaries don't align properly.
    //
    //   The best course is to move the loop code below into the dec.decode()
    //   part.
    //
    //   It's a bit of a WTF to do this stuff with objects, really.  It would be
    //   eauqlly OK to just have a function:
    //
    //     decode(state, audio, frame, push_function);
    //
    //   Perhaps therefore it makes most sense:
    //   - decode() does all the decoding into its own static buffer (as now)
    //     and calls a pushing function on that buffer.
    //   - the pushing function may as well do the chunking in that case.
    //   - the push function can be a functor which has the entire state in it;
    //     we do not need the packet_state to be part of the ffmpeg library.
    //     - this is OK even for C plugins.  They do not see the functor.
    //
    //   So, a rationale for chunking:
    //   - ffmpeg buffers are very big.
    //   - ffmpeg buffers must be aligned.
    //   - ffmpeg buffers are hardly ever full.
    //   - we have to chunk it at some point for the sound card.
    //   - huge buffers mean the response time is much lower.
    //   - huge buffers mean the granularity of the buffer size is very low.

    //   Another idea:
    //
    //   Note: remember that chunking is necessary because of ffmpeg huge
    //   buffers and alignment requirements.
    //
    //     // pooled allocated blocks which we fill up as we go
    //     chunk_buff generic_state;
    //     // has a chunk(frame); -- it must take the frame so it can get info
    //     // about it
    //     chunker chunk(generic_state, push_function);
    //
    //     ...
    //
    //     decoder(file, audio_stream);
    //     decoder.each_packet(chunk);
    //
    //   - decoder doesn't need to know about the packet state at all (much
    //     better genericness)
    //   - chunker function is still tied to ffmpeg because it needs to know
    //   - we can use a generic lib for actually chunking stuff
    //   - we don't need to do lots of complicated chunking to make all the
    //     packets an even size - later plugins can fuck with the plugins
    //     anyway.
    //
    //   NOTE: I need to look to the sanalyser version of ffmpeg as it has more
    //   or less implemented this design.
    //
    ffmpeg::audio_decoder decoder(state, audio);
    decoder.decode(fr);

    void *sample_buffer = decoder.get_packet();
    while (sample_buffer != NULL) {
      if (dump_to_file) {
        fwrite(sample_buffer, sizeof(uint8_t), state.size(), dump_output_file);
      }

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

