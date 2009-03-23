#include <nerved_config.hpp>

#ifdef HAVE_FFMPEG_LIBAVCODEC_AVCODEC_H
#  include <ffmpeg/libavcodec/avcodec.h>
// Assume this to get a easier to diagnose error message.
#else /*if defined(HAVE_FFMPEG_AVCODEC_H)*/
#  include <ffmpeg/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_LIBAVFORMAT_AVFORMAT_H
#  include <ffmpeg/libavformat/avformat.h>
#else /*if defined(HAVE_FFMPEG_AVFORMAT_H)*/
#  include <ffmpeg/avformat.h>
#endif

#include "play.hpp"

#include "sdl.hpp"
#include <iostream>
#include <fstream>

#include <pthread.h>
#include <cstdlib>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::ifstream in;


int amount_read = 0;
std::size_t file_buffer_size = 0;
uint8_t *file_buffer = NULL;

int buffer_file() {
  in.read((char*) file_buffer, file_buffer_size);
  amount_read = in.gcount();
}

void callback(void *userdata, uint8_t *stream, int length) {
  // TODO:
  //   Other libs do not involve their own audio thread.  I will need to account for that
  //   somehow.

  // also need to check that I have played the entire buffer here when buffering is
  // done in the  other thread.
  if (in.eof()) {
    std::cout << "notify eof" << std::endl;
    int r = pthread_cond_signal(&cond);
    assert(r == 0);
    // just in case the callback is called again before the shutdown happens
    amount_read = 0;
    return;
  }

  int len = std::min(amount_read, length);
  // TODO:
  //   problem here: if the file ends with a partial buffer, sdl still wants to
  //   play the entire thing.  Therefore, I'll need to memset 0 the rest of the
  //   buffer.
  //
  //   In the gapless version, I'll need to mix the two buffers together.  This
  //   indicates that I will absolutely have to organise the buffering in another
  //   thread.
  //
  //   Still... it needs to be synchronus anyway... I absolutely must avoid any
  //   kind of stutter; if that means doing all the loading in this thread then
  //   that's fine.
  std::memcpy(stream, file_buffer, len);
  // SDL_MixAudio(stream, file_buffer, len, SDL_MIX_MAXVOLUME);

  buffer_file();
}


void play(const char * const file) {
  assert(file != NULL);
  try {
    std::cout << "Begin." << std::endl;
    sdl::audio aud;
    std::cout << "Audio status is: " << aud.status_name() << std::endl;
    sdl::device dev(aud);
    in.open(file);
    assert(in.good());
    {
      sdl::audio_spec desired(callback);
      std::cout << "Opening audio." << std::endl;
      std::cout << "wanted:\n" << desired << std::endl;
      dev.reopen(desired);
      std::cout << "got:\n" << dev.obtained() << std::endl;
    }

    file_buffer_size = dev.obtained().buffer_size();
    file_buffer = (uint8_t *) std::malloc(sizeof(uint8_t) * file_buffer_size);
    // Discard the 44 byte .wav header.
    in.read((char *) file_buffer, 44);
    std::memset(file_buffer, 0, file_buffer_size);
    // prebuffer
    buffer_file();
    std::cout << "playing: " << file << std::endl;

    dev.unpause();
    std::cout << "unpaused; now waiting" << std::endl;
    int r = pthread_mutex_lock(&mutex);
    assert(r == 0);
    std::cout << "locked" << std::endl;
    r = pthread_cond_wait(&cond, &mutex);
    sdl::audio_lock_guard lock_for_shutdown(dev);
    assert(r == 0);
    std::cout << "got notified" << std::endl;
    r = pthread_mutex_unlock(&mutex);
    assert(r == 0);
    free(file_buffer);

    std::cout << "shutting down" << std::endl;
  }
  catch (sdl::init_error &e) {
    std::cerr << "Error: could not initialise sdl: " << e.what() << std::endl;
    exit(1);
  }
  catch (sdl::open_error &e) {
    std::cerr << "Error: could not open the sound card: " << e.what() << std::endl;
    exit(1);
  }

  std::cout << "finished!" << std::endl;

}
