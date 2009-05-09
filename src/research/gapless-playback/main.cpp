/*!
\file
\brief Initial prototype of gapless media playing of two or more files.

Deals with mixing stuff together, and especially different formats and
sample rates etc.  It also serves as the first prototype to the playlist
manager so we can work out how interrupted songs will work and so on.
*/

#include "playlist.hpp"
#include "play.hpp"

#include <bdbg/trace/static_definitions.hpp>
#include <bdbg/trace/crash_detection.hpp>

bdbg::trace::crash_detector cd;

#include <boost/filesystem.hpp>

#include <cstdlib>
#include <iostream>
#include <list>
#include <algorithm>
#include <queue>

struct list_printer {
  std::size_t i;

  list_printer() : i(1) {}

  void operator()(const char *v) {
    std::cout << "# " << i << ". " << v << std::endl;
    i++;
  }
};

int main(int argc, char **argv) {
  namespace fs = boost::filesystem;

  if (argc < 2) {
    std::cerr << "Error: not enough arguments.  Gimmie some songs to play!" << std::endl;
    return EXIT_FAILURE;
  }
  else if (argc < 3) {
    std::cerr << "Warning: can't test gaplessness with only one file." << std::endl;
  }

  {
    bool argh = false;
    for (int i = 1; i < argc; ++i) {
      char *file = argv[i];
      if (! fs::exists(file)) {
        argh = true;
        std::cerr << "error: file '" << file << "' does not exist." << std::endl;
      }
    }

    if (argh) return EXIT_FAILURE;
  }

  playlist_type playlist(&argv[1], &argv[argc]);

  std::cout << "Playlist:" << std::endl;
  list_printer pr;
  std::for_each(playlist.begin(), playlist.end(), pr);

  return play_from_list(playlist);
}

