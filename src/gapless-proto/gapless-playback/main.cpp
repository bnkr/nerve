/*!
\file
\brief Initial prototype of gapless media playing of two or more files.

Deals with mixing stuff together, and especially different formats and
sample rates etc.  It also serves as the first prototype to the playlist
manager so we can work out how interrupted songs will work and so on.
*/

#include "playlist.hpp"
#include "play.hpp"
#include "dump_file.hpp"

#include <boost/filesystem.hpp>

#include <cstdlib>
#include <cstring>
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

  playlist_type playlist;
  {
    bool argh = false;
    for (int i = 1; i < argc; ++i) {
      if (argv[i][0] == '-') {
        const char *const arg = &argv[i][1];
        if (std::strcmp(arg, "dump") == 0) {
          make_file_output = true;
          std::cout << "-dump: will dump to a file." << std::endl;
        }
        else {
          std::cerr << "error: unrecognised argument: " << arg << std::endl;
          return EXIT_FAILURE;
        }

      }
      else {
        char *file = argv[i];
        if (! fs::exists(file)) {
          argh = true;
          std::cerr << "error: file '" << file << "' does not exist." << std::endl;
        }
        playlist.push_back(file);
      }
    }

    if (argh) return EXIT_FAILURE;
  }


  std::cout << "Playlist:" << std::endl;
  list_printer pr;
  std::for_each(playlist.begin(), playlist.end(), pr);

  return play_from_list(playlist);
}

