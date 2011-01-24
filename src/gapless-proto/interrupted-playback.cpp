/*!
\file
\brief Given an interrupt, this will switch songs to another and still be gapless.

The last experiment!
*/

#include <iostream>
#include <cstdlib>
#include <boost/filesystem.hpp>

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "error: wrong args.  Need three files." << std::endl;
    return EXIT_FAILURE;
  }

  for (std::size_t i = 1; i < 4; ++i) {
    if (! boost::filesystem::exists(argv[i])) {
      std::cerr << "error: " << argv[i] << ": doesn't exist." << std::endl;
      return EXIT_FAILURE;
    }
  }

  // TODO: start here :)

  return EXIT_SUCCESS;
}

