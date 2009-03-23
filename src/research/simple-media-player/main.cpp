/*!
\file
\brief Basic playing of a single file.

This experiment gets us to the point where we can decode files and put them to
stdout.  It's also explores places to add memory pooling and ways that audio
filtering/observing can be done.
*/

#include <cstdlib>
#include <iostream>
#include <boost/filesystem.hpp>
#include "play.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "error: wrong number of arguments.  Need one file to play." << std::endl;
    return EXIT_FAILURE;
  }

  const char * const file = argv[1];
  if (! (boost::filesystem::exists(file) && boost::filesystem::is_regular(file))) {
    std::cerr << "error: not a regular file: '" << file << "'" << std::endl;
    return EXIT_FAILURE;
  }

  play(file);

  return EXIT_SUCCESS;
}
