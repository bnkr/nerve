#include "settings.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv) {
  try {
    settings config(argc, argv);
    if (config.exit()) {
      return config.exit_status();
    }
  }
  catch (boost::program_options::error &e) {
    std::cerr << "Error parsing arguments: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
