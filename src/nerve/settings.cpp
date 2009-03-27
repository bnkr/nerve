#include "settings.hpp"

#include <nerve_config.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <cstdlib>

void settings::print_version() const {
  std::cout <<
    "nerve version " NERVE_VERSION ", compiled on " __DATE__ " for " NERVE_TARGET_SYSTEM ".\n"
    "Copyright (C) James Webber 2009.  Distributed under a 3-clause BSD licence.\n"
    << std::flush;
}

void settings::print_help(boost::program_options::options_description &opts) const {
  std::cout <<
    "Usage: nerve command [option]...\n"
    "Controls the nerve daemon's playlist and running state.\n\n";

  std::cout <<
//  "this string is precisely 80 visible chars long.                                 \n"
    "Common commands:\n"
    "  play [-f]\n"
    "         Unpause or start playing the current song.  If -f if given and a song\n"
    "         is already playing, then it will seek to the start of the song.\n"
    "  stop   Stop playing and seek to position zero.\n"
    "  pause  Stop playing but keep the song state.\n"
    "  add [-s] [-r] FILE...\n"
    "         Append FILEs to the playlist.  If a FILE is a directory then the entire\n"
    "         dir is appended  -s is like inserting the track after the current track\n"
    "         and skipping to it immediately.  -r will replace the current playlist.\n"
    "  next/prev\n"
    "         Skip forwards/backwards in the playlist.\n"
    "  skip [-r] [-d] [-n] NAME\n"
    "         Skip to the song named NAME which is in the playlist.  -r means NAME is\n"
    "         a regexp.  -n means it is a song number in the playlist from 1 to list\n"
    "         size.  -d means NAME refers to the directory of the file.\n"
    << std::flush;

  std::cout << opts << std::endl;
  print_version();
}

void settings::load_options(int argc, char **argv) {
  namespace po = boost::program_options;

  // find the command name.  I think it'd be easiest here.  We strncmp against
  // each possible command name; that way we can abbreviate teh command easier.
  // And also set up the right options_descriptions for the options to the command.
  // Then I think we have a hash map of cmd_id vs. function to call.

#if 0
  // if this has to happen maybe this part should handle all printing out
  // instead of main?

  if (argc < 2) {
    throw insufficiant_arguments_error();
  }

  // Actually it would be faster to only store the unambiguous parts
  // of the commands - can be done at compile time.
  const char * const command = argv[1];
  std::size_t given_length = std::strlen(command);
  command_id_type cmd_id = no_command;
  const char *chosen_command = NULL;
  for (std::size_t commands; i < num_commands; ++i) {
    if (std::strncmp(commands[i], command, given_length) == 0) {
      if (cmd_id != no_command) {
        // never to have a full list of what it could be
        throw ambiguous_command_error(std::string(commands[i]) + " vs. " + chosen_command);
        break;
      }
      chosen_command = commands[i];
      cmd = (command_id_type) i;
    }
  }
#endif

  po::options_description general_opts("General options");
  general_opts.add_options()
    ("help", "Display options description and exit.")
    ("version", "Display version and copyright and exit.")
    ("verbose", "Say more things.")
    ;

  po::options_description all_opts;
  all_opts
    .add(general_opts);

  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc, argv).options(all_opts).run();
  po::store(parsed, vm);

  if (vm.count("help")) {
    exit_status_ = EXIT_SUCCESS;
    print_help(all_opts);
    return;
  }
  else if (vm.count("version")) {
    exit_status_ = EXIT_SUCCESS;
    print_version();
    return;
  }
}

void settings::set_defaults() {
  exit_status_ = -1;
}
