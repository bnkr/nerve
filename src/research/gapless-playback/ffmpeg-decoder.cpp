/*!
\file
\brief Reduce compile time.
*/

#include <nerved_config.hpp>
#include <bdbg/trace/short_macros.hpp>

#include "../../wrappers/ffmpeg.hpp"



//! \brief Truncate the beginning of the bufgfer if it starts with silence
//TODO:
//  I guess this will be a problem if we ever support non-int16_t samples.
//  As far as I can see, ffmpeg only gives you int16...
void ffmpeg::audio_decoder::truncate_pre_silence(const int threshold) {
  trc("started with a buffer of " << buffer_size_ << " bytes");
  std::size_t elts = buffer_size_ / sizeof(int16_t);
  // trc("there are " << elts <<  " of that unit in the array");
  std::size_t num_trimmed = 0;
  int16_t *samples = (int16_t*) buffer_.ptr();
  for (std::size_t i = 0; i < elts; ++i) {
    // trc(i);
    if (samples[i] >= (-threshold) && samples[i] <= threshold) {
    // if (samples[i] == 0) {
      buffer_index_ += sizeof(int16_t);
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

  int16_t first_byte = samples[buffer_index_ / sizeof(int16_t)];
  trc("final byte is: " << first_byte);
  trc("index has become " << buffer_index_ <<  " bytes.");
  trc("truncated to " << buffer_size_ <<  " bytes.");
  trc("trimmed      " << num_trimmed  << " times.");
}

//! \brief Truncate the buffer if it ends with silence.
void ffmpeg::audio_decoder::truncate_silence(const int threshold) {

  // TODO:
  //   aaaarrgh!!!  I've just realised why this doesn't work.  It's *signed*
  //   data so it should be
  //
  //     (samples[i] >= -threshold && samples[i] <= threshhold)
  trc("started with a buffer of " << buffer_size_ << " bytes");
  std::size_t elts = buffer_size_ / sizeof(int16_t);
  // trc("there are " << elts <<  " of that unit in the array");
  std::size_t num_trimmed = 0;
  int16_t *samples = (int16_t*) buffer_.ptr();
  for (int i = elts - 1; i >= 0; --i) {
    // trc(i);
    if (samples[i] >= (-threshold) && samples[i] <= threshold) {
    // if (samples[i] == 0) {
      buffer_size_ -= sizeof(int16_t);
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


  int16_t final_byte = samples[buffer_size_ / sizeof(int16_t) - 1];
  trc("final byte is: " << final_byte);
  trc("truncated to " << buffer_size_ <<  " bytes.");
  trc("trimmed      " << num_trimmed  << " times.");
}

void ffmpeg::audio_decoder::decode(const ffmpeg::frame &fr) {
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
    //
    // TODO:
    //   configuration requirements of a gap killer:
    //
    //   - (int) minimum length of internal gaps to kill
    //     - when 0, internal gaps are never killed
    //     - gaps shorter than minimum are left intact.
    //   - (int) maximum length of gaps to kill at end/start of tracks
    //     - gaps longer than maximum length are left intact
    //     - when 0, all gaps are killed.
    //   - (int) silence threshhold (percentage)
    //

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
    }


    trc("frame: " << codec_context(stream_).frame_number() << " with " << fr.size() << " bytes (data offset = " << fr.file().data_offset() << ").");
    // only problem is that I might drop bytes, so this kind of calcxulation is going to be
    // inaccurate.  It should be alright to get the time from end etc. tho.
    //
    // Add/take fr.file().data_offset() for a value from 0 to.
    trc("frame extents: " << fr.position() << " to " << fr.position() + fr.size() << " / " << fr.file().file_size());
    trc("frame extents: " << std::hex << fr.position() << " to " << fr.position() + fr.size() << " / " << fr.file().file_size() << std::dec);

#define WAV_MP3 1
#define WAV 2
#define WAV_MP3_WAV 3
#define TRIM_NONE 4

#ifndef PATHALOGICAL_GAPLESS
#  define PATHALOGICAL_GAPLESS WAV_MP3_WAV
#endif

#define GENERIC_GAPLESS

#ifdef GENERIC_GAPLESS
    // TODO:
    //   implement plan as written in the main thing

    // Qucik note:
    //
    // - establish a running average.  It seems that the jump periods are more
    //   or less white noise to they *average* to a low value *but* hey don't
    //   change much.  When we do get a big change it should be measured on
    //   average.  The trick is marking the start of the average change.
    //
    //   Basically I think I am saying that I need a tolerance when degapping
    //   crappy streams.

    static bool run_pregap = false;

    // TODO:
    //   doesn't work yet... we have to translate time into byte length.
    //
    //   bytes = t_ms * rate * channels
    const int64_t active_time = 0;
    const int16_t sample_difference = 0;

    // Hack: turn it on on the 0th frame.
    if (codec_context(stream_).frame_number() == 0) {
      run_pregap = true;
    }

    if (run_pregap) {

      // TODO: this is the pathalogical end - we must use the active time.
      if ((fr.position() + fr.size()) > (9048-44)) {
        trc("Ending after frame " << codec_context(stream_).frame_number());
        run_pregap = false;
      }
    }

#elif PATHALOGICAL_GAPLESS == WAV
    // This is the pathalogical test for 220hz-sine-wave-pt*-gap.wav and also
    // nogap.wav.
    //
    // Note: keep this pathalogical case lying around.  Requirement is that
    // frames are 4096 bytes big (unless they are the end) otherwise the frame
    // counts won't match up.
    //
    // This does work perfectly, btw :)
    static int trimmed = 0;
    if (codec_context(stream_).frame_number() >= 28 && trimmed == 0) {
      // TODO: this will be a problem if we're not using signed 16bit I guess.
      truncate_silence(0);
      if (codec_context(stream_).frame_number() == 31) {
        trimmed = 1;
      }
    }
    else if (trimmed == 1 && codec_context(stream_).frame_number() <= 4) {
      truncate_pre_silence(0);
      if (codec_context(stream_).frame_number() == 4) {
        trimmed = 2;
      }
    }
#elif PATHALOGICAL_GAPLESS == WAV_MP3_WAV
    // gapless wav -> mp3 -> wav again.
    // for 220hz-sine-wave-pt*-nogap.wav.mp3.wav


    // Note: properties of gap:
    //
    // Method: copy/paste the gap from sweep.  Could be one or two bytes off.
    //
    // the gap at the end of p1      = 4260 bytes
    // start of gap                  = (size - 44) - 4206 = ???
    // the gap at the start of p2    = 9048
    // end of gap                    = 9048 - 44 = ???

    // TODO:
    //   - finish those properties
    //   - write them in the readme (like for the other sample).
    //   - test the gapkiller using the file dump mode.
    static int trimmed = 0;
    int64_t start = fr.position();
    int64_t end = start + fr.size();
    int64_t start_killing = 0x1d5d0; // start of trailing gap
    int64_t stop_killing  = 331660; // at end of leading gap.
    int16_t threshold = 128;
    // trc("end >= start_killing == " << end << " >= " << start_killing);
    if (trimmed == 0) {
      if (end >= start_killing) {
        truncate_silence(threshold);
        if (fr.position() + fr.size() + fr.file().data_offset() >= fr.file().file_size()) {
          trc("last frame of p1");
          trimmed = 1;
        }
      }
    }
    else if (trimmed == 1) {
      if (start <= stop_killing) {
        truncate_pre_silence(threshold);
        if (fr.position() + fr.size() + fr.file().data_offset() >= fr.file().file_size()) {
          trc("last frame of p2");
          trimmed = 0;
        }
      }
    }

#elif PATHALOGICAL_GAPLESS == WAV_MP3
    // gapless wav -> mp3
    // for 220hz-sine-wave-pt*-nogap.wav.mp3

#error not done yet.
#elif PATHALOGICAL_GAPLESS == TRIM_NONE

#else
#error Wrong value wtf.

#endif


  }
}



