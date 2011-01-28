// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Deals with the cli and calling the player.  Just simple stuff to keep
 * everything nice and separate.
 */

#include "asserts.hpp"
#include "settings.hpp"
#include "player.hpp"

int main() {
  settings::cli_status cs = settings::parse_cli(argc, argv);
  switch (cs) {
    case settings::cli_exit_ok:
      return EXIT_SUCCESS;
    case settings::cli_exit_fail:
      return EXIT_FAILURE;
    case settings::cli_ok:
      break;
    default:
      NERVE_ABORT("wrong status from settings::parse_cli: " << cs);
  }

  player::play_status ps = player::play();
  switch (ps) {
    case player::status_error:
      return EXIT_FAILURE;
    case player::status_ok:
      return EXIT_SUCCESS;
    default:
      NERVE_ABORT("wrong status from player::play: " << ps);
  }

   NERVE_ABORT("oh dear.  This should not have been reached.");
}
