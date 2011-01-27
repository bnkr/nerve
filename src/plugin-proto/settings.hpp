/*!
 * \file
 *
 * Run-time configuration and setings.
 */

//! Singleton class
class settings {
  public:
    //! What to do after parsing the cli.
    enum cli_status  {
      exit_ok,
      exit_fail,
      continue_ok
    };

    //! Always called when the program is enered.
    enum cli_status parse_cli(int argc, char **argv);
};
