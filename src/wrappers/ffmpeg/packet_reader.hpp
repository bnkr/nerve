/*!
\file
\brief Stateful interface to the ffmpeg::packet structure.
*/

#ifndef FFMPEG_PACKETS_HPP_5qw4rnoe
#define FFMPEG_PACKETS_HPP_5qw4rnoe

namespace ffmpeg {


/*!
\ingroup grp_ffmpeg
Stateful object which reads frames from an ffmeg::file

This class ties an ffmpeg::packet to an ffmpeg::file.  The packet itself is a
parameter so you can read multiple files without reallocating the packet.

Mote: apart from providing an easier stateful interface, this class is here
because it seems likely we'll need to skip back and forth later and it would
be good to have a wrapper instead of manually messing with the audio_stream
and the file.

Note: packet is a parameter instead of a member so it can be re-used for
multiple files.
*/
class packet_reader {
  public:
    packet_reader(ffmpeg::packet &dest, ffmpeg::file &file)
    : packet_(dest), file_(file) {
    }

    //! Read a packet; return false on EOF.
    bool read() {
      int ret = av_read_frame(&file_.av_format_context(), &packet_.av_packet());
      // TODO:
      //   It is not documented in ffmpeg what code is an error, but it always
      //   seems to return -5 for EOF.
      return ret == 0;
    }

    const ffmpeg::packet &packet() const { return packet_; }
    ffmpeg::packet &packet() { return packet_; }

    const ffmpeg::file &file() const { return file_; }
    ffmpeg::file &file() { return file_; }

  private:
    ffmpeg::packet &packet_;
    ffmpeg::file &file_;
};


}

#endif
