/*!
\file
\brief main()
*/

#include "../../wrappers/sdl.hpp"
// #include "settings.hpp"
// #include "data.hpp"

#include <iostream>

#include <cstdlib>
#include <cmath>

#include <boost/thread.hpp>

// TODO: monitor properly
bool finished = false;
boost::mutex finished_mutex;
boost::condition_variable finished_cond;

// really the periods should be generated elsewhere and this func
// just pulls them.
void sine_callback(void *, uint8_t *stream, int len) {
  static double sine_pos = 0;

  // TODO:
  //   this should be got from the sdl obj ofc.
  const double sample_frequency = 44100;
  const int channels = 1;

  // TODO:
  //   should be a cmdline param.
  const double note_frequency = 220.0;
  const double amp = 0.75;

  const double sine_speed = 2 * M_PI * note_frequency / sample_frequency;

  // Interpret the buffer as 16 bit samples.
  // TODO: make this work with an arbitrary size sample (easy with templates)
  std::size_t samples_len = len / sizeof(int16_t);
  int16_t *samples = (int16_t *) stream;
  std::size_t i = 0;
  while (i < samples_len) {
    double y = amp * std::sin(sine_pos);
    int16_t normalised = y * std::numeric_limits<int16_t>::max();
    for (int c = 0; c < channels; ++c) {
      samples[i++] = normalised;
    }
    sine_pos += sine_speed;
  }

  finished = true;
  // finish_cond.notify_all();
  boost::thread::yield();
}

void make_sine(uint8_t *stream, int length) {
  sine_callback(NULL, stream, length);
}

#include <cstdio>

void sine_file(const char *filename, std::size_t length) {
  // should be able to specify size etc.
  FILE *fh = fopen(filename, "w");
  uint16_t *samples = (uint16_t *) std::malloc(length * sizeof(int16_t));
  make_sine((uint8_t*)samples, length * sizeof(int16_t) / sizeof(uint8_t));
  fwrite(samples, sizeof(int16_t), length, fh);
  std::free(samples);
  fclose(fh);
}


// http://code.google.com/p/freemat/wiki/TutorialMakeASinewave

int main(int argc, const char **argv) {
  try {
    // settings set(arc, argv);
    // if (set.bad()) {
      // return set.exit_code();
    // }

    // all this should be dynamic
    std::size_t miliseconds = 5 * 10;
    std::size_t seconds = miliseconds / 10;
    std::size_t frequency = 44100;
    std::size_t channels = 1;

    std::size_t samples = frequency * seconds;
    std::size_t buffer_size = channels * samples * sizeof(int16_t);

    // TODO:
    //   allow stdout if !isatty so you can pipe it to
    //
    //     aplay -c 1 -f S16_LE -r 44100
    sine_file("sinewave.raw", samples);


    return 0;

    sdl::audio aud;
    sdl::audio_spec out_spec(sine_callback);
    out_spec.channels(1);
    sdl::device dev(aud, out_spec);
    // if (set.verbose()) {
    std::cout << "Audio spec:" << std::endl;
    dev.obtained().dump(std::cout, "  ") << std::endl;
    // }

    dev.unpause();
    // TODO:
    //   somehow set up a list of notes to iterate
    //
    //   and call something like play_notes(container, duration);
    //   also play_single_note(note_id);
    //

    // So how do I switch notes?
    // - Switch it every T ms by timer or something?  Nice because I can
    //   use ctrl-c to change then or even arrow keys to go back and forth.
    // - use any key for next note
    // - also have a time between note mode - could use SDL timer?  Or just
    //   thread sleep easy enough.

    boost::unique_lock<boost::mutex> lk(finished_mutex);
    while (! finished) {
      finished_cond.wait(lk);
    }

    return EXIT_SUCCESS;
  }
  // TODO: be more specific
  catch (std::exception &e) {
    std::cerr << "Error parsing arguments: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}
