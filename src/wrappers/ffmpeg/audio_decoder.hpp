#ifndef FFMPEG_AUDIO_DECODER_HPP_vjeuxxli
#define FFMPEG_AUDIO_DECODER_HPP_vjeuxxli

namespace ffmpeg {

//! \brief Reads frames into buffers of a given size in a packet_state.
//! Loop on get_packet().  Remember to get any remaining data in packet_state when
//! completely done.
//TODO:
//  this should be split up very much and changes when we have the plugin
//  stage framework.  It should simply return the buffers; therefore the design
//  will be very different.  Including packet_state would not be needed - that
//  would be used in the output plugin.
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
    void decode_frame(const ffmpeg::frame &fr) FF_ATTRIBUTE_DEPRECATED {
      decode(fr);
    }

    //! \brief Set the internal state for this new frame.
    void decode(const ffmpeg::frame &fr); // currently outline to reduce compiler time.
    void truncate_pre_silence(const int);
    void truncate_silence(const int);

    //! \brief Get the next packet_size sized buffer from the frame, or NULL if there isn't one.
    //TODO:
    //  return something like an auto_ptr of course; really it should be done via.
    //  an allocator or even some kind of visitor function?  That would be less
    //  error prone, certainly.  Ideally I want to put it in a queue which is
    //  a memory pool.
    //
    //  Later we will not need to chunk the files because the output plugin will do it.
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

    //! \deprected Use packet_state::get_final() directly.
    void *get_final_packet() {
      return packet_.get_final();
    }


  private:
    //! Decode a packet into output.  Returns also output_size.  Returns number
    //! of bytes of the output buffer which are used up.  Buffer must be larget
    //! than ACVOCED_MAX_AUDIO_FRAME_SIZE.
    int decode(int16_t *output, int *output_size, AVPacket *packet) {
      return avcodec_decode_audio3(&stream_.av_codec_context(), output, output_size, packet);
    }

    void reset_buffer() {
      buffer_index_ = 0;
      buffer_size_ = 0;
    }

    ffmpeg::audio_stream &stream_;

    // work out the aligned buffer type
    // Later we will alloc this from a pool and pass it down to the other plugs.
    // It must be a special type which carries its own dimensions and properties
    // about the audio it carries so we can automatically configure and so on.
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



}

#endif
