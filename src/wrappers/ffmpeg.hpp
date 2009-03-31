/*!
\file
\brief C++ helpers for ffmpeg.

Note that here we take the conventions of ffmpeg where the term `context' means
informational.

Ffmpeg is a pretty complicated library and this wrapper aims to simplify it as
well as add needed functionality like splitting the stream into packets for sdl
output.  In order to do this, the classes here wrap the ffmpeg structs and expose
a subset of their fields.  These are the ones you actually need, rather than
internal stuff.
*/
#ifndef FFMPEG_HPP_7awlau1z
#define FFMPEG_HPP_7awlau1z

// TODO:
//   this implies that later on, the wrappers should be part of nerved instead
//   of the client.  Perhaps the client should be a completely seperate package?
#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

// FFmpeg developers hate C++, and not without good reason!  But then I have
// always been a masochist...
extern "C" {
#ifdef HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
#  include <ffmpeg/libavcodec/avcodec.h>
// Assume this to get a easier to diagnose error message.
#else /*if defined(HAVE_FFMPEG_AVCODEC_H)*/
#  include <ffmpeg/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
#  include <ffmpeg/libavformat/avformat.h>
#else /*if defined(HAVE_FFMPEG_AVFORMAT_H)*/
#  include <ffmpeg/avformat.h>
#endif
}

#include "ffmpeg/data.hpp"

#include "../include/aligned_memory.hpp"
#include <cstring>
#include <cstdlib>

namespace ffmpeg {

//! \brief Seperate stateful object which builds packets.
//! It must be seperate so it can exist in a higher scope than a file
//! and its audio stream.  This is to be used by the audio_decoder, and
//! should never need to be touched by the other code.
//
//TODO:
//  given that the state is seperate, I could now cause the audio_decoder
//  to immediately read the frame in ctor instead of doing it seperately.
//  This would make things safer to use.  Note: this changes rather a lot
//  wrt frame delaying.
class packet_state : boost::noncopyable {
  public:
    //! \brief The packet size is the size of the buffer to pass to the output queue.
    //! Silence value is normally 0, but sometimes it's not!
    packet_state(std::size_t packet_size, int silence_value)
    : packet_(NULL), packet_size_(packet_size), packet_index_(0), silence_value_(silence_value) {
      packet_ = std::malloc(packet_size);
      assert(packet_ != NULL);
    }

    ~packet_state() {
      std::free(packet_);
    }

    std::size_t index() const { return packet_index_; }
    std::size_t size() const { return packet_size_; }

    //! \brief Append the most possible; return how many appened.
    std::size_t append_max(void *buffer, std::size_t max_bytes)  {
      const std::size_t bytes_left = packet_size_ - packet_index_;
      const std::size_t copy_len = std::min(max_bytes, bytes_left);

      uint8_t *sample_buffer = ((uint8_t *) packet_) + packet_index_;
      std::memcpy(sample_buffer, buffer, copy_len);
      packet_index_ += copy_len;

      return copy_len;
    }

    //! \brief Set unused bytes to silence and return the ptr on the heap.
    void *get_final() {
      finalise();
      return clear();
    }

    // TODO:
    //   resize() could be necessary later.  It's a problem if the packet size is
    //   reduced though.  Forcing to clear first is a workaround, but not a very
    //   nice one... possibly it's a neceesary one though if you end up having
    //   to restart the audio thread anyway.

    //! \brief Returns old packet (on the heap).
    void *reset() {
      /// TODO:
      ///   need to use some kind of allocator to get these; also an RAII type.
      void *p = clear();
      assert(packet_ == NULL);
      assert(p != NULL);
      packet_ = std::malloc(packet_size_);
      // std::cout << "reset(): release: " << p << std::endl;
      // std::cout << "reset(): alloc:   " << packet_ << std::endl;
      return p;
    }

    //! \brief Like reset_packet(), but don't allocate a new one.
    void *clear() {
      // this shouldn't be called if we didn't finish it off already.
      assert(index() == size());
      void *p = packet_;
      packet_ = NULL;
      packet_index_ = 0;
      return p;
    }

    //! \brief For observation only!  Use the stateful functions.
    const void *ptr() { return packet_; }

  private:
    //! \brief Set the remainder of the buffer to silence
    void finalise() {
      std::memset(((uint8_t*)packet_ + packet_index_), silence_value_, packet_size_ - packet_index_);
      packet_index_ = packet_size_;
    }

    void *packet_;
    const std::size_t packet_size_;
    std::size_t packet_index_;
    const int silence_value_;
};

//! \brief Reads frames into buffers of a given size in a packet_state.
//! Loop on get_packet().  Remember to get any remaining data in packet_state when
//! completely done.
//TODO: should probably take the
class audio_decoder {
  public:
    //! \brief Buffer size is what should be given to the audio output.
    audio_decoder(packet_state &packet_state, ffmpeg::audio_stream &stream)
    : stream_(stream), buffer_index_(0), packet_(packet_state) {
      // my assumption is that it needs some N of 16 bit integers, even
      // though we actually just write random stuff to it.
      assert(total_buffer_bytes % sizeof(int16_t) == 0);
    }

    //! \deprecated use decode(fr);
    void decode_frame(const ffmpeg::frame &fr) {
      decode(fr);
    }

    //! \brief Set the internal state for this new frame.
    void decode(const ffmpeg::frame &fr) {
      /*
      From the docs:

      For the input buffer we over allocate by FF_INPUT_BUFFER_PADDING_SIZE
      because optimised readers will read in longer bitlengths.  We never
      actually read data up to that length and the last byte must be zero
      (ffmpeg doesn't always do that).

      Output must be 16-byte alligned because SSE needs it.

      Input must be "at least 4 byte aligned".  Again ffmpeg doesn't always
      do it in fr.data().

      Finally, the output must be at least AVCODEC_MAX_AUDIO_FRAME_SIZE.

      TODO:
        it seems weird that ffmpeg's own data is not OK to put directly
        into the decoder.
      */
      reset_buffer();

      int used_buffer_size = buffer_type::byte_size;
      int used_bytes = decode(buffer_.ptr(), &used_buffer_size, fr.data(), fr.size());

      if (used_bytes < fr.size())  {
        // We can keep going, but there will be output errors.
        // TODO: more detail.
        std::cerr << "warning: less bytes read from the stream than were available." << std::endl;
      }

      if (used_buffer_size <= 0) {
        // TODO: more detail.
        std::cerr <<  "warning: nothing to decode." << std::endl;
      }
      else {
        // don't set this unless we know it's unsigned.
        buffer_size_ = (std::size_t) used_buffer_size;

        //
        // TODO:
        //   Also I should only do it on the last frame of a file.
        //
        // TODO:
        //   Source data/research:
        //
        //   If wikipedia is to be believed:
        //     "Encoder/decoder overall delay is not defined, which means there is no
        //     official provision for gapless playback. However, some encoders such as
        //     LAME can attach additional metadata that will allow players that can handle
        //     it to deliver seamless playback."
        //
        //   It later implies that you can use silence detection to get rid of
        //   silence.  http://en.wikipedia.org/wiki/Gapless_playback
        //
        //   This has stuff about timestamps:
        //   - http://www.dranger.com/ffmpeg/tutorial05.html
        //
        //   By looking at sweep we can see that there are gaps at the end *and* begining of
        //   an mp3.

        // TODO:
        //   the ultimate task is to;
        //   - determine where we are in the *stream*
        //     - we must use a time value because bytes will be a different timespan
        //       when at different bitrates.
        //   - if we are past a certain limit or before a certain limit
        //   - truncate the buffer where the end is lower than a certain threshhold
        //
        //   additionally:
        //   - make it work over multiple frames, eg if the last frame is exteremly
        //     small then we need to kill a longer gap.  Error case:
        //     - the limit dictates that two frames from the end of the song are subject
        //       to gap removal.
        //     - the entire second to last frame is empty
        //     - the last frame is not empty.


        {
          // this seems to be right.
          // double fractional_seconds = fr.presentation_time() * av_q2d(stream_.time_base());
          // int sec = (int) fractional_seconds;
          // int us = (fractional_seconds - sec) * 1000;

          // TODO:
          //   ok this sort of works, but not really.  There's still a click and I can't
          //   work out why.
          //
          //   When I use extreme values it messes the stream completely.  I have no idea
          //   why that happens at all.  Maybe there is some other issue?  Gah, what  I
          //   really need is a file output so I can analyse the waveform I generated.
          //
          //   Argh this is so weird.  If you drop a lot of frames  at the start, it kills
          //   the entire stream!  Is my buffer handling wrong or something?  Does it become
          //   the wrong number of bytes?
          // if (fractional_seconds >= 2 || fractional_seconds <= 0.5 ) {
          //   trc("drop this frame: " << fractional_seconds);
          //   buffer_size_ = 0;
          //   return;
          // }
        }


        trc("frame: " << codec_context(stream_).frame_number() << " with " << fr.size() << " bytes (data offset = " << fr.file().data_offset() << ").");
        // only problem is that I might drop bytes, so this kind of calcxulation is going to be
        // inaccurate.  It should be alright to get the time from end etc. tho.
        //
        // Add/take fr.file().data_offset() for a value from 0 to.
        trc("frame extents: " << fr.position() << " to " << fr.position() + fr.size() << " / " << fr.file().file_size());
        static int trimmed = 0;

        // Note: test properties.
        //
        // This ensures that pathalogical cases where a pure period of silence at the start or
        // end is removed.
        //
        // - p1 nogap last sample               = -19870
        // - p1 gap last sample                 = -19870
        // - p2 nogap first sample should be    = -20313
        // - p2 gap first sample should be      = -20313
        // - bytes of silence at end of p1      = 14688 (b274 to ebd4 (past eof) = 45684 to 60372) = 60372 - 45684
        //                                      = start on frame 28
        // - bytes of silence at start of p2    = 14688 (02c to 398c = 44 to 14732) = 14732 - 44
        //                                      = stop on frame 4
        //
        // Looks like it overlapped by one byte... really those last bytes should be
        // different.  Well... it doesn't matter much, it just means we don't do the
        // short crossfade strategy (or if we do it's the pathalogical case).
        // Crossfade comes later anyway.  We can experiment.
        //
        // acording to the complete file:
        // - the last bytes of the p1    = ???
        // - the initial bytes of the p2 = ???
        //
        // ps: remember endianness, remember stereo files have double samples,
        // remember .wav files have a variable length header.


        // Note: keep this pathalogical case lying around.  Requirement is that
        // frames are 4096 bytes big (unless they are the end) otherwise the frame
        // counts won't match up.
        if (codec_context(stream_).frame_number() >= 28 && trimmed == 0) {
          // TODO: this will be a problem if we're not using signed 16bit I guess.
          truncate_silence<int16_t>(0);
          if (codec_context(stream_).frame_number() == 31) {
            trimmed = 1;
          }
        }
        else if (trimmed == 1 && codec_context(stream_).frame_number() <= 4) {
          // this one must work forward!
          truncate_pre_silence<int16_t>(0);
          if (codec_context(stream_).frame_number() == 4) {
            trimmed = 2;
          }
        }

        // note: optimal algorithm:
        //
        // This is really tricky because we have to hold up the pipeline.  This algorithm
        // is very similar for a internal gap killer.  It migh be better to form this as
        // a special case of the internal gapkiller, but the .
        //
        // Algorithm:
        //
        //   every frame:
        //
        //   if gapkill state = ignore
        //   // -1 so we can have the entire time period availale to us.
        //   else if near enoguh to the end - 1
        //     look for first non-silence from the end
        //
        //     if frame is partial
        //       flush frame buffers (they are not part of the last period of silence)
        //       flag partialnes (store the `real' dimensions of the frame)
        //       delay frame
        //     else if frame is empty && it's the first one we touched
        //       // don't gapkill any more - assume that there was more silence before this.
        //       gapkill state = ignore
        //     else if frame is empty
        //       delay frame
        //     end
        //   // if there was no silence period at the end of the last frame, we assume
        //   // that any silence at the start here is intended.  The always gapkill starts
        //   // option allows us deal with a nogap end -> gap start transition (eg, .wav->mp3)
        //   else if near enough to the the start && (frames are delayed || always gapkill starts )
        //     search for first non-silence from the start
        //
        //     if frame is partial
        //       // we reached the definite end of the silence period
        //       delete empty frames
        //       flush previous partial frame with the correct dimensions
        //       flush this partial frame with the correct dimensions
        //     else frame is empty
        //       // we still delay frames in case the silence period is too long which implies
        //       // that this frame had an intentional gap.
        //       delay frame
        //     end
        //   // We reached the end of the period where silence can be so assume that
        //   // the silence was intentional.
        //   else
        //     if frames are delayed
        //       flush
        //     end
        //     gapkill state = normal
        //   end
        //
        // Implications:
        // - we must be able to delay
        // - we must have a store for one partial frame (could be the same as a
        //   standard frame, not terribly important)
        // - the total pre-buffer time becomes exteremy important.  It must be
        //   enough to account for us delaying the entire near_enough_to_end +
        //   near_enough_to_start time.  This is problematic because of variable
        //   bitrates - we can't be sure exactly how much buffering that buffer
        //   will be...
        //
        // To implement:
        // - we should be checking over global timestamps, not on frame boundaries
        //   We detect silence from the first sample elligable for trimming.
        //
        // To decide:
        // - can this/should this be implemented as a case of the internal gap killer.
        //   Need to design that algorighm.
        //
        // Observations:
        // - flush could do the short crossfade if necesasry.


        // note: algorithm design for generic gapkiller
        //
        // frame:
        //
        // // we are killing a gap
        // if partial frame queued
        //   find first silence from start
        //   if frame is silent
        //     delay frame
        //   else if frame is partial && total duration of silence is long enough
        //     delete delayed silent frames
        //     merge the original partial with this one
        //   else
        //     flush
        //   end
        // else
        //   // we must do the full analysis if a single frame can contain
        //   // a valid silence period.  Otherwise, we can optimise and just
        //   // search for the first silence from the end.
        //   loop frame:
        //     s1 = find next silence
        //     s2 = find next silence or eof
        //     if time between s1 to s2 is long enough
        //       remove chunk - eep how
        //     end
        //   end
        //
        //   if s1 = found && s2 == eof
        //     queue this partial frame
        //   end
        // end
        //
        //
        // Observations:
        // - not sufficiant for end/start gap killing because they can easily be shorter
        //   than the gap period.
        // - Wow that's a whole lot easier than the other one!
        // - quite a lot of work done here
        //
        // Questions:
        // - could we wrte (duration is long enough || near enough to the end|start)?
        //
        // Difficulties:
        // - removing an internal chunk of a frame.


        // note: methods of delaying frames
        //
        // - copy into a giant buffer
        //   - memcpy is slow
        //   - completely dropping frames is free
        //   - might end up doing reallocs for the first few times.
        //   - hard to mempool
        // - allocate this frame buffer (the aligned_memory) and put it
        //   on a queue.
        //   - requires a queue which could be and is definitely using
        //     allocs.
        //   - easier to pool (except the aligned memory bit is rather
        //     wastefull - better to align the base address and dispatch
        //     in units of $alignment - would that be possible for frame
        //     buffers?  They might not all be %16 anyway?
        //   - frame buffers are bloody huge 192000 bytes.
        //
        // Gr... I might be better off trying both and profiling it.
        //
        //
      }
    }


    //! \brief Truncate the beginning of the bufgfer if it starts with silence
    template <class UnitsOf>
    void truncate_pre_silence(const int threshold) {
      trc("started with a buffer of " << buffer_size_ << " bytes");
      std::size_t elts = buffer_size_ / sizeof(UnitsOf);
      // trc("there are " << elts <<  " of that unit in the array");
      std::size_t num_trimmed = 0;
      UnitsOf *samples = (UnitsOf*) buffer_.ptr();
      for (std::size_t i = 0; i < elts; ++i) {
        // trc(i);
        // if (samples[i] >= (-threshold) && samples[i] <= threshold) {
        if (samples[i] == 0) {
          buffer_index_ += sizeof(UnitsOf);
          num_trimmed++;
        }
        else {
          trc("Found a byte " << samples[i]);
          break;
        }
      }

      if (! num_trimmed) {
        trc("nothing truncated");
        return;
      }

      int16_t first_byte = samples[buffer_index_ / sizeof(UnitsOf)];
      trc("final byte is: " << first_byte);
      trc("index has become " << buffer_index_ <<  " bytes.");
      trc("truncated to " << buffer_size_ <<  " bytes.");
      trc("trimmed      " << num_trimmed  << " times.");
    }

    //! \brief Truncate the buffer if it ends with silence.
    template <class UnitsOf>
    void truncate_silence(const int threshold) {

      // TODO:
      //   aaaarrgh!!!  I've just realised why this doesn't work.  It's *signed*
      //   data so it should be
      //
      //     (samples[i] >= -threshold && samples[i] <= threshhold)
      trc("started with a buffer of " << buffer_size_ << " bytes");
      std::size_t elts = buffer_size_ / sizeof(UnitsOf);
      // trc("there are " << elts <<  " of that unit in the array");
      std::size_t num_trimmed = 0;
      UnitsOf *samples = (UnitsOf*) buffer_.ptr();
      for (int i = elts - 1; i >= 0; --i) {
        // trc(i);
        // if (samples[i] >= (-threshold) && samples[i] <= threshold) {
        if (samples[i] == 0) {
          buffer_size_ -= sizeof(UnitsOf);
          num_trimmed++;
        }
        else {
          trc("Found a byte " << samples[i]);
          break;
        }
      }

      if (! num_trimmed) {
        trc("nothing truncated");
        return;
      }


      int16_t final_byte = samples[buffer_size_ / sizeof(UnitsOf) - 1];
      trc("final byte is: " << final_byte);
      trc("truncated to " << buffer_size_ <<  " bytes.");
      trc("trimmed      " << num_trimmed  << " times.");
    }

    //! \brief Get the next packet_size sized buffer from the frame, or NULL if there isn't one.
    //TODO:
    //  return something like an auto_ptr of course; really it should be done via.
    //  an allocator or even some kind of visitor function?  That would be less
    //  error prone, certainly.  Ideally I want to put it in a queue which is
    //  a memory pool.
    void *get_packet() {
      if (buffer_index_ < buffer_size_) {
        const std::size_t stream_available = buffer_size_ - buffer_index_;
        uint8_t *stream_buffer = ((uint8_t *) buffer_.ptr()) + buffer_index_;

        buffer_index_ += packet_.append_max(stream_buffer, stream_available);

        // We filled up a buffer
        if (packet_.index() == packet_.size()) {
          // trc("packet complete");
          void *p = packet_.reset();
          // std::cout << "get_packet(): " << p << std::endl;
          return p;
        }
        else {
          // trc("packet is incomplete");
          // we are not allowed to overrun the buffer.
          assert(packet_.index() < packet_.size());
          return NULL;
        }
      }
      else {
        // we must have actually outputted all the data
        assert(buffer_index_ == buffer_size_);
        // trc("we have reached the end of the buffer: ind = "  << buffer_index_ << " vs. size = " << buffer_size_);
        return NULL;
      }
    }

    //! \brief Get a packet with unfilled bytes set nul.  A new state is NOT allocated.
    // void *get_final_packet() {
    //   void *p = get_packet();
    //   if (p != NULL) {
    //     std::cerr << "get_final_packet(): error: something has gone horribly wrong -- there are packets left to output!" << std::endl;
    //   }

    //   packet_.finalise();
    //   return packet_.clear();
    // }


  private:
    int decode(int16_t *output, int *output_size, const uint8_t *input, int input_size) {
      return avcodec_decode_audio2(&stream_.codec_context(), output, output_size, input, input_size);
    }

    void reset_buffer() {
      buffer_index_ = 0;
      buffer_size_ = 0;
    }

    ffmpeg::audio_stream &stream_;

    // work out the aligned buffer type
    static const std::size_t total_buffer_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    static const std::size_t total_buffer_size = total_buffer_bytes / sizeof(int16_t);
    static const std::size_t alignment = 16;
    typedef aligned_memory<alignment, total_buffer_size, int16_t> buffer_type;

    buffer_type buffer_;

    // the amount of the buffer which was written to by ffmpeg (in bytes);
    std::size_t buffer_size_;
    // stateful data - where have we outputted this frame up to? (In bytes)
    std::size_t buffer_index_;

    // working state
    packet_state &packet_;
};


class decoded_block {

};


} // ns ffmpeg


#endif

