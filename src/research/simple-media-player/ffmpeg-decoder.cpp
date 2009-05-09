/*!
\file
\brief Reduce compile time.
*/

#include "../../wrappers/ffmpeg.hpp"

// hacked together backport from the gapless tests
void ffmpeg::audio_decoder::decode(const ffmpeg::frame &fr) {
  reset_buffer();

  int used_buffer_size = buffer_type::byte_size;
  int used_bytes = decode(buffer_.ptr(), &used_buffer_size, &(fr.packet()));

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
  }
}



