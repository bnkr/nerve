#include "ffmpeg.hpp"

#include <iostream>

typedef void (*log_callback_type)(void*, int, const char*, va_list);
void test_loading_log_cb(void *ptr, int level, const char *fmt, va_list args) {
  AVClass *avc = ptr ? *(AVClass**) ptr : NULL;
  // need to sprintf into a string... can I use fprintf of an iostream?  Or do my
  // own parse?
  // ptr is used to get the name of the decode module.
  // last_log.reset();
}


#include <tr1/type_traits> // alignment_of
#include <cstdlib>

void dump_packet(AVPacket &packet) {
  std::cout << "Packet data:" << std::endl;
  std::cout << "  presentation ts:  " << packet.pts << std::endl;
  std::cout << "  decompression ts: " << packet.dts << std::endl;
  std::cout << "  data:             " << (void*) packet.data << std::endl;
  int al;
  std::size_t input_buf = (std::size_t) packet.data;
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
  std::cout << "  data alignment:   " << al << std::endl;
  std::cout << "  size:             " << packet.size << std::endl;
  std::cout << "  stream index:     " << packet.stream_index << std::endl;
  std::cout << "  packet flag:      " << packet.flags << " (" << ((packet.flags == PKT_FLAG_KEY) ? "keyframe" : "non-keyframe") <<  ")" << std::endl;
  std::cout << "  presentation dur: " << packet.duration << std::endl;
  std::cout << "  deallocator:      " << (void*) packet.destruct << std::endl;
  std::cout << "  byte offset:      " << packet.pos << std::endl;
}

#include "aligned_memory.hpp"


// loop through all frames and print out data about them.
void load_file_test(const char *filename) {
  // http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/avformat_8h.html
  // http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/avcodec_8h.html
  // http://www.inb.uni-luebeck.de/~boehme/using_libavcodec.html
  //
  // http://www.dranger.com/ffmpeg/functions.html - abbreviated function list
  // http://www.dranger.com/ffmpeg/data.html - abbreviated data type list

  try {
    ffmpeg::library lib(AV_LOG_DEBUG);
    ffmpeg::file file(filename);
    file.dump_format(filename);

    ffmpeg::audio_stream audio(file);

    // TODO:
    //   obv. this stuff has to be c++alised.

    // Output must be 16-byte alligned because SSE needs it.
    const uint8_t align = 16;
    // use uint8 so the padding will work.
    typedef aligned_memory<align, int16_t> aligned_memory_type;
    const std::size_t buffer_byte_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;
    assert(buffer_byte_size % sizeof(aligned_memory_type::value_type) == 0);
    aligned_memory_type mem(buffer_byte_size / sizeof(aligned_memory_type::value_type));
    int16_t *output_buf = mem.ptr();

    AVPacket packet;
    while(av_read_frame(&file.format_context(), &packet) >= 0) {
      dump_packet(packet);

      // Reset this every time because decode writes back to it.
      int output_buf_size = buffer_byte_size;
      // It looks like this is always 16 bit aligned, so we don't need to mess
      // with it.  Docs say it should be at least 4 byte alligned.
      uint8_t *input_buf = packet.data;
      int input_buf_size = packet.size;

      std::cout << "Buffer properties:" << std::endl;

      // Again, this does not always appear 16-byte aligned.  Perhaps it's to do with
      // different file formats?
      // assert(((std::size_t) input_buf % 16) == 0);
      // docs say it must be 0 in case of corrupt frames but it isn't.
      // assert(input_buf[input_buf_size - 1] == 0);

      // TODO: this is necessary:
      // For the input buffer we over allocate by FF_INPUT_BUFFER_PADDING_SIZE
      // because optimised readers will read in longer bitlengths.  We never
      // actually read the full data.

      int size = avcodec_decode_audio2(&audio.codec_context(), output_buf, &output_buf_size, input_buf, input_buf_size);

      std::cout << "Decode data:" << std::endl;
      std::cout << "  number of bytes used: " << size << std::endl;
      std::cout << "  output_buf_size:      " << output_buf_size  << " (was given " << buffer_byte_size << ")"<< std::endl;

      if (size < 0) {
        std::cerr << "noooooo!" << std::endl;
      }
      else if (size == 0) {
        std::cerr << "no frame could be decompressed" << std::endl;
      }

      // then copy output buf into the audio stream
      av_free_packet(&packet);
    }

    std::cout << "Yay!" << std::endl;
  }
  catch (ffmpeg::ffmpeg_error &e) {
    std::cerr << "Phail: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }

}

