namespace ffmpeg {

//! \brief Initialised by pulling a frame from a \link ffmpeg::file \endlink.
//! See:
//! - http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/structAVPacket.html
class frame {
  public:
    frame(ffmpeg::file &file) : file_(file) {
      // TODO: formatContext stores an AVPacket.  Does that mean I am doing a useless copy here?
      int ret = av_read_frame(&file.av_format_context(), &packet_);
      finished_ = (ret != 0);
    }

    ~frame() { av_free_packet(&packet_); }

    //! \name Packet accessors
    //@{
    //! Stream finished?
    bool finished() const { return finished_; }

    //! \brief Data buffer in the frame packet.
    //TODO: since audio2 doesn't exist any more, these aren't necessary.
    const uint8_t *data() const { return packet_.data; }
    int size() const { return packet_.size; }

    //! \brief Timestamp of when to output this in units of time_base.
    //! Note that sometiems the decode timestamp is different from the presentation one, but
    //! this does not happen in audio streams.
    //TODO:
    //  could be better to wrap this time stuff in an object like frame_time and another
    //  object file_time.
    int64_t presentation_time() const { return packet_.pts; }

    //! \brief Presentation time in fractional seconds in the stream's time base.
    double stream_time_double(const ffmpeg::audio_stream &stream) const {
      return presentation_time() * av_q2d(stream.time_base_q());
    }

    //! \brief Time in AV_TIME_BASE units.
    //TODO: this does not seem to be right.  Test for frame number = 0 - maybe it's just an offset?
    // int64_t presentation_time() const { return stream_time_sec() * AV_TIME_BASE; }

    //! \brief Byte position in stream.
    int64_t position() const { return packet_.pos; }
    //@}

    //! \brief The file this frame was pulled from.
    ffmpeg::file &file() { return file_; }
    const ffmpeg::file &file() const { return file_; }

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

    //! \brief Use accessor member functions if possible.
    AVPacket &packet() const { return packet_; }

  private:
    ffmpeg::file &file_;
    // TODO:
    //   this is here as a hack because an ffmpeg function became deprecated.
    //   Since decode_audio3 now takes this packet directly, it could make sense
    //   to have the decoder function in here.
    mutable AVPacket packet_;
    bool finished_;
};


}
