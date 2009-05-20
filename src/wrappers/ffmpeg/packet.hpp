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

Note that the packet is in the context of the file, not the stream, so you
can't use the sizes etc. to tell where you are in the stream.

This object does not have an explicit stream context.  It seems that they are
simply read from the file wherever they appear, so the stream_index() can
change after reading a new packet into this object.

See:
- http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVPacket.html

TODO:
  Unfortunately this makes things a bit awkward to get the times right.  I won't
  attempt to solve this problem until I've done stuff with mutliple streams
  because I'll likely mess it up otherwise.

  I need the ability to get an stream by ID number.  I guess this means I need a
  generic stream class (not so surprising),  Then I need to store the streams in
  the file object.  Otherwise I end up passing loads of these objects around and
  we don't have a proper central location where they're actually stored -- they
  are conceptually `big' objects, so making them all temporaries is pretty
  confusing.

*/
class packet {
  public:

    //! \name Constructors/destructors
    //@{
    explicit packet() : finished_(false) { }

    ~packet() { av_free_packet(&packet_); }

    //@}

    //! \brief Pull another packet from the file.
    void read_from(ffmpeg::file &file) {
      // TODO: formatContext stores an AVPacket.  Does that mean I am doing a useless copy here?
      int ret = av_read_frame(&file.av_format_context(), &packet_);
      finished_ = (ret != 0);
    }

    //! \name Packet accessors
    //@{
    //! Stream finished, ie is this packet the EOF packet.
    bool finished() const { return finished_; }

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
    //! doesn't have access to.  This is a limitation of the wrapper because
    //! it was made under the assumption that you only need one stream (the
    //! audio stream) and consequently there is no method to get the stream
    //! from a packet.  So the code to use times is something like:
    //!
    //!   stream_time st(stream_.time_base_q(), pkt.presentation_timestamp());
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
      std::cout << pf << "packet_ flag:      " << packet_.flags << " ("
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

  protected:
    //! \brief call read_from(file).
    explicit packet(ffmpeg::file &file) {
      read_from(file);
    }

  private:
    AVPacket packet_;
    bool finished_;
};

}
#endif
