/*!
\file
\brief Audio decoding.
*/

#ifndef FFMPEG_AUDIO_DECODER_HPP_vbszml7j
#define FFMPEG_AUDIO_DECODER_HPP_vbszml7j

#include <cstring>
#include <cstdlib>

namespace ffmpeg {

/*!
\ingroup grp_ffmpeg
Reads frames into buffers of a given size in a packet_state.

See the function ffmpeg::decode_audio() for a good example of this class.

Note that we deal with bytes, not samples, since the samples can vary in size.
See codec_context for this data.
*/
class audio_decoder {
  public:
    explicit audio_decoder(ffmpeg::audio_stream &stream)
    : stream_(stream), buffer_size_(0) {
    }


    //! Decode a frame into this object's buffer.
    void decode(ffmpeg::packet &packet) {
      // std::cout << "decode at offset " << packet.position() << std::endl;
      buffer_size_ = 0;
      int used_buffer_size = buffer_type::byte_size;
      int bytes_read = decode(buffer_.ptr(), &used_buffer_size, &(packet.av_packet()));
      // std::cout << "read " << bytes_read  << " and used " << used_buffer_size << " of the buffer." << std::endl;

      if (bytes_read < packet.size())  {
        // TODO:
        //   we can keep going, but there will be output errors.
        throw std::runtime_error("less bytes read than were available");
      }

      if (used_buffer_size <= 0) {
        // TODO: more detail.
        std::cerr << "warning: nothing to decode." << std::endl;
      }
      else {
        // don't set this unless we know it's unsigned.
        buffer_size_ = (std::size_t) used_buffer_size;
      }
    }

    //! \name Accessors
    //@{

    //! See also codec_context - it has many useful properties.
    const ffmpeg::audio_stream &audio_stream() const { return stream_; }
    ffmpeg::audio_stream &audio_stream() { return stream_; }

    //! Wrapper of audio_stream::file();
    const ffmpeg::file &file() const { return stream_.file(); }
    ffmpeg::file &file() { return stream_.file(); }

    //! Pointer to decoded samples.
    //!
    //! Note that this is invalid until you decode something (hence why
    //! decoded_audio is a nicer interface.
    //TODO:
    //  I am returning bytes here, but decode audio explicitly uses int16s.
    //  Does that mean ffmpeg always decodes with 16bit samples?  It returns the
    //  bytes read so it's very uncertain!
    const void *samples() const { return buffer_.ptr(); }
   //! \brief Size of the sample buffer in bytes.
    std::size_t samples_size() const { return buffer_size_; }

    //@}

  private:
    //! Decode a packet into output.  Returns also output_size.  Returns number
    //! of bytes of the output buffer which are used up.  Buffer must be larget
    //! than ACVOCED_MAX_AUDIO_FRAME_SIZE.
    int decode(int16_t *output, int *output_size, AVPacket *packet) {
      return avcodec_decode_audio3(&stream_.av_codec_context(), output, output_size, packet);
    }

    ffmpeg::audio_stream &stream_;

    // TODO:
    //   There is a reasonable argument for making this dynamically allocated.
    //   If we don't dynamically allocate then there is always an extra copy
    //   when we take the data from this buffer.  On the other hand, this buffer
    //   is rather big, mostly empty, and doesn't have any properties.  The
    //   alternative would be to write directly into whatever the structure the
    //   rest of the program wants, but that's not very generic.
    static const std::size_t alignment = 16;
    typedef aligned_memory<alignment, AVCODEC_MAX_AUDIO_FRAME_SIZE / sizeof(int16_t), int16_t> buffer_type;
    buffer_type buffer_;

    // the amount of the buffer which was written to by ffmpeg (in bytes);
    std::size_t buffer_size_;
};

/*!
\ingroup grp_ffmpeg
An facade to the decoded data in a decoder.

This decodes the data in a frame into the decoder, then provides an interface
to access the decoded frames.

Essentially, this is a stateless wrapper to the stateful data of the packet
and the decoder.
*/
class decoded_audio {
  public:
    //! \name Constructors/Destructors
    //@{
    decoded_audio(ffmpeg::audio_decoder &dec, ffmpeg::packet &pkt)
    : decoder_(dec), packet_(pkt) {
      decoder_.decode(pkt);
    }
    //@}

    //! \name Property accessors
    //@{
    const ffmpeg::audio_decoder &audio_decoder() const { return decoder_; }
    ffmpeg::audio_decoder &audio_decoder() { return decoder_; }

    const ffmpeg::packet &packet() const { return packet_; }
    ffmpeg::packet &packet() { return packet_; }
    //@}

    //! \name Aliases into packet.
    //@{

    //! The time in the scale of the stream.
    scaled_time presentation_time() const {
      return scaled_time(audio_stream().time_base_q(), packet().presentation_timestamp());
    }
    //@}

    //! \name Aliases into the decoder.
    //@{
    const uint8_t *samples() const { return (uint8_t *) decoder_.samples(); }
    std::size_t samples_size() const { return decoder_.samples_size(); }
    // TODO: accessor for sample size, signedness.

    //! codec_context(stream()).property() is a useful pattern.
    const ffmpeg::audio_stream &audio_stream() const { return decoder_.audio_stream(); }
    ffmpeg::audio_stream &audio_stream() { return decoder_.audio_stream(); }

    const ffmpeg::file &file() const { return decoder_.file(); }
    ffmpeg::file &file() { return decoder_.file(); }
    //@}

  private:
    ffmpeg::audio_decoder &decoder_;
    ffmpeg::packet &packet_;
};

/*!
\brief Calls visit() on all decoded packets read from the particular stream.

Visit receives a decoded_audio object.
*/
template<class PacketVisitor>
void decode_audio(ffmpeg::audio_stream &str, PacketVisitor visit) {
  ffmpeg::packet pkt;
  ffmpeg::packet_reader pr(pkt, str.file());
  ffmpeg::audio_decoder dec(str);

  while (pr.read()) {
    ffmpeg::decoded_audio au(dec, pr.packet());
    visit(au);
  }
}


} // ns ffmpeg

#endif
