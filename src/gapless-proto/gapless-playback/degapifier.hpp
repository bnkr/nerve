#ifndef DEGAPIFY_HPP_4miv4mz8
#define DEGAPIFY_HPP_4miv4mz8

namespace ffmpeg {
  class decoded_audio;
}

class packet_state;

typedef packet_state packet_chunker_type;

class degapifier {
  public:
    degapifier(packet_chunker_type &state) : state_(state) {}

    void degapify(ffmpeg::decoded_audio &aud);

    // copy this from the old ffmpeg audio_decoder
    void *get_packet();

  private:
    packet_chunker_type &state_;

};

#endif
