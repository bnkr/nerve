/*!
\file
\brief The packet class.
*/

#ifndef FFMPEG_PACKET_HPP_kj0hyi3j
#define FFMPEG_PACKET_HPP_kj0hyi3j

namespace ffmpeg {

/*!
\ingroup grp_ffmpeg
Container struct for packet data and accessor wrapper.

This stateful class shouldbe used with a packet_reader.  It is seperate because
packets are not bound to a particular file or stream - you just write the data
into it and have to track where it came from yourself.  Because of this, you
can use the same packet without reallocating.

Some of the fields here are in the context of the stream (mostly the time),
while sizes are in the context of the file.  Therefore it is advisable to use
the decoded_audio facade which binds those structures together.

See:
- http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVPacket.html
*/
class packet {
  public:

    //! \name Constructors/destructors
    //@{
    ~packet() { av_free_packet(&packet_); }
    //@}

    //! \name Packet accessors
    //@{

    //! Data buffer in the frame packet.  Note it is not necessary to use
    //! this now that there is a new function in ffmpeg which decodes audio
    //! from an AVPacket instead of an arbitrary buffer.
    const uint8_t *data() const { return packet_.data; }
    //! Bytes in data().
    int size() const { return packet_.size; }

    //! \brief Byte position in file which *includes* the data offset.
    int64_t position() const { return packet_.pos; }

    //! Stream number this packet is assosciated with.
    int stream_index() const { return packet_.stream_index; }

    //@}

    //! \name Packet time data
    //!
    //! Note that these times are in units of the stream, which this packet
    //! doesn't have access to.  decoded_audio helps here.  Currently it is a
    //! limitiation of the wrapper that we can't get a stream by id.
    //@{

    //! Raw timestamp of when to output this in units of AvStream::time_base.
    int64_t presentation_timestamp() const { return packet_.pts; }
    int64_t decode_timestamp() const { return packet_.dts; }
    //@}

    //! \brief This also shows all the field purposes.
    std::ostream &dump(std::ostream &o, const char *pf = "") {
      std::cout << pf << "presentation ts:  " << packet_.pts << std::endl;
      std::cout << pf << "decompression ts: " << packet_.dts << std::endl;
      std::cout << pf << "data:             " << (void*) packet_.data << std::endl;
      int al;
      std::size_t input_buf = (std::size_t) packet_.data;
      if (input_buf % 16 == 0) {
        al = 16;
      }
      else if (input_buf % 8 == 0) {
        al = 8;
      }
      else if (input_buf % 4 == 0) {
        al = 4;
      }
      else if (input_buf % 2 == 0) {
        al = 2;
      }
      else if (input_buf % 1 == 0) {
        al = 1;
      }
      else {
        al = -1;
      }
      std::cout << pf << "data alignment:   " << al << std::endl;
      std::cout << pf << "size:             " << packet_.size << std::endl;
      std::cout << pf << "stream index:     " << packet_.stream_index << std::endl;
      std::cout << pf << "packet_ flag:     " << packet_.flags << " ("
        << ((packet_.flags == PKT_FLAG_KEY) ? "keyframe" : "non-keyframe") <<  ")" << std::endl;
      std::cout << pf << "presentation dur: " << packet_.duration << std::endl;
      std::cout << pf << "deallocator:      " << (void*) packet_.destruct << std::endl;
      std::cout << pf << "byte offset:      " << packet_.pos << std::endl;

      return o;
    }

    //! \name FFmpeg Accessors
    //@{

    //! \brief Use accessor member functions if possible.
    const AVPacket &av_packet() const { return packet_; }
    AVPacket &av_packet() { return packet_; }

    //@}

  private:
    AVPacket packet_;
};

}
#endif
