// #include <nerved_config.hpp>

#include "play.hpp"
#include "chunkinate.hpp"
#include "../../wrappers/ffmpeg.hpp"
#include "../../wrappers/sdl.hpp"
#include "packet_state.hpp"
#include "output.hpp"
#include "shared_data.hpp"
#include "dump_file.hpp"

#include "portabdbg.hpp"

#include <cstdlib>

int play_from_list(playlist_type &list) {
  trc("Begin.");
  // blah blah should be an observer plugin
  if (make_file_output) {
    trc("dumping to sample-dump.raw");
    dump_output_file = fopen("sample-dump.raw", "w");
    assert(dump_output_file);
  }

  try {
    // TODO:
    //   what happens to this stuff when I do output plugins?  I don't really want to
    //   op new them
    sdl::audio aud;
    sdl::device dev(aud);
    {
      sdl::audio_spec::callback_type cb = sdl_callback;
      sdl::audio_spec desired(cb);
      // std::cout << "Opening audio:" << std::endl;
      // desired.dump_spec(std::cout, "  ") << std::endl;
      dev.reopen(desired);
      std::cout << "SDL Got:" << std::endl;
      dev.obtained().dump_spec(std::cout, "  ") << std::endl;
    }
    ffmpeg::initialiser ff;
    trc("Finished initialising.");

    packet_state state(dev.obtained().buffer_size(), dev.obtained().silence());

    if (! make_file_output) dev.unpause();

    trc("Chunking files.");
    typedef playlist_type::const_iterator iter_type;
    for (iter_type i = list.begin(); i != list.end(); ++i) {
      chunkinate_file(state, *i, make_file_output);
    }
    chunkinate_finish(state, make_file_output);

    // because the sdl callback is not called when this happens.
    if (make_file_output) output_closed = true;

    trc("wait for exit signal");

    // TODO:
    //   wait on something - do a proper monitor this time.  Perhaps make
    //   it a member of the chunkinate object (when that's done?)  Or better
    //   make this a player object which has a condition we that anything
    //   notifies whenever the playlist is updated.
    //
    //   Really this loop is a bit wrong.  foreach playlist up there should be
    //   what we wait on; or at least we have that and inside the playlist ther
    //   is a "quit" var which we get notified of every now and then.
    //
    //   It's iffy to put the quit inside the playlist tho.  They aren't exactly
    //   related.
    boost::unique_lock<boost::mutex> lk(finish_mut);
    while (! output_closed) {
      finish_cond.wait(lk);
    }
  }
  catch (sdl::error &e) {
    std::cerr << "Error: sdl init failed: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (make_file_output) {
#ifndef WIN32
    fsync(fileno(dump_output_file));
#endif
    fclose(dump_output_file);
  }

  trc("Terminate.");

  return EXIT_SUCCESS;
}

