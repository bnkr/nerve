/*!
\file
\brief Initial prototype of gapless media playing of two files

Deals with mixing stuff together, and especially different formats and
sample rates etc.
*/

#include <boost/filesystem.hpp>

#include <cstdlib>
#include <iostream>

void alsa_prototype(const char *first, const char *second) {

}

int main(int argc, char **argv) {
  namespace fs = boost::filesystem;

  if (argc != 3) {
    std::cerr << "Error: wrong number of arguments." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "#1 " << argv[1] << std::endl;
  std::cout << "#2 " << argv[2] << std::endl;

  {
    bool argh = false;
    for (std::size_t i = 1; i < 3; ++i) {
      char *file = argv[i];
      if (! fs::exists(file)) {
        argv[i];
        argh = true;
        std::cerr << "error: file '" << file << "' does not exist." << std::endl;
      }
    }

    if (argh) return EXIT_FAILURE;
  }

  alsa_prototype(argv[1], argv[2]);
  return EXIT_SUCCESS;
}

