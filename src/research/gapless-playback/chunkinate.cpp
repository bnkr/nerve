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
    //   bug - if you drop all frames, then we wait for the exit signal forever.

    // TODO:
    //   Plugins, stream processors.
    //
    //   It should probably be something like
    //
    //   ffmpeg::audio_decoder dec(fr);
    //   // one of which is 'if last frame, truncate buffer'
    //   dec.preprocess(plugins_list);
    //
    //   each {|sample_block|
    //     block_process(sample_block);
    //     observer_process(sample_block);
    //   }
    //
    //   An interesting extension would be to organise a multi-threaded
    //   processing pipeline.  Each stage has a monitored queue for
    //   packets to mess with.
    //
    //   Additionally observer vs. manipulator plugins.  Meh, I think I
    //   can work it out in general.
    //
    // TODO:
    //   What about a crossfade plug?  That would need to somehow take the packets and
    //   store them somewhere until there's enough to fade properly.  It has to work on
    //   the stream data clearly.  Perhaps there is a better way of doing thsi:
    //
    //   chunk_state state;
    //   // another long state class - perhaps the chunk state is
    //   // part of the internals?  That class is way too messy anyway.
    //   processor proc(state, plugins_list_or_something);
    //   ...
    //   each frame {
    //     frame fr(file);
    //     proc.process(frame);
    //
    //
    //     // Or perhaps the chunker appears as part of the processor process?
    //     // where do observer plugins go?  Post_chunks could do it?  Means it
    //     // can't be a plugin of course.
    //     proc.make_chunks(post_chunks());
    //   }
    //
    //   This implies a certain level of abstraction.  There is no need for the processor
    //   to know about the ffmpeg lib or the sdl lib.  That is a good thing, but we'll leave
    //   it until later since it's easier to work with the frame interface.
    //
    //   The internals of the processor are rather difficult because we might need to
    //   defer processing a frame until later.  Regarding crossfade, we are going to need
    //   to know when a frame is the last frame in the file which I'm still having trouble
    //   with (or better the generalised position within the file at any given point).
    //
    //   Having to copy that buffer is really tricky.
    //
    // TODO:
    //   Efficiancy note: if we're dropping frames all the time we're going to end up
    //   calling get_packet() ==  NULL constantly.  It would be better to do the whole
    //   iteration process in one function; we can drop the frame as needed.
    //
    // TODO:
    //   Format conversion:
    //   - libsamplerate - necessary because some sound cards won't support some rates
    //     and also reconfigure request is very hard to manage.
    //   - format (int, uint is really hard to do because we're not dynamic in our
    //     output type at all.  Have to see about that one.
    //
    // TODO:
    //   Upmixer:
    //   - the processor needs to know how many channels to mix up to.
    //   - the sample buffer needs to be configured appropriately so packet_state needs
    //     to know about it somehow.
    //     - Again implies that packet_state should be part of processor
    //     - also implies processor needs to know about the obtained output (although
    //       it still does not need to know about SDL specifically).  Perhaps we can
    //       have a generic audio_spec type later on?  Again - we ignore this stuff for
    //       now because output plugins are not my focus.
    //   - mixing needs to be done before chunking, because the chunks will be a different
    //     size.  We are essentially jsut doing
    //
    //       sample = new int16_t[current_buffer * channels];
    //       each sample as s
    //         for i in 1 to channels
    //           sample[i] = s
    //         end
    //       end
    //
    //
    //   - This could be done in the chunking process easily enough.  No need to duplicate
    //     the entire buffer.
    //   - it does indicate that the decode buffer's size must be able to change sizes
    //     arbitrarily depending on
    //
    // TODO:
    //   Plugin pipeline:
    //
    //   Some will drop, store, reorder frames if they want.  So I need some kind of
    //   pipe processing.  It must be multi-threaded, but we might just have a pool.
    //   Delaying frames is really tricky because you need a fast buffer.
    //
    //   The problem is we are always assuming that each stage will give you something
    //   and we *pull* from every stage.  We need to *push*.  That way there's no
    //   constantly looping around checking that we got given something.
    //
    // TODO:
    //   Gap killing: see ffmpeg.
    //
    // TODO:
    //   On track interrupts (also seeking applies here):
    //   - asuming we are able to have a reasonable limit of prebuffering.
    //   - stop buffering the current track (after we are up to our limit)
    //   - start buffering the next
    //   - either:
    //     - make a brand new buffer for the new track and switch them over
    //     - or delete everything remaining in the old buffer.
    //   - buffer/replace is likely the fastest, but it requires a design
    //     change because we need to alloc the buffer structure itself - easy
    //     to have memory errors if stuff references the old buffer.
    //   - we could have two prebuffers available = no allocs... not a total
    //     solution, though.
    //   - big buffer = lots of work to free prebuffered packets.
    //     - mitigated by an async freeing queue?
    //     - would be a lot mitigated by a memory pool.
    //   - If we do free the packets it has implications for the packet pool - it
    //     wouldn't be a simple stack any more - would need an extra list of ptrs
    //     to give out/take back.
    //   - small buffer = we risk packet queue underflow when loading the new
    //     file/codec/stream.
    //   - buffer should be in units of miliseconds, as should everything else.
    //

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

