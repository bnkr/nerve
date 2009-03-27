#ifndef SETTINGS_HPP_l3xzroy5
#define SETTINGS_HPP_l3xzroy5

#include <boost/program_options/errors.hpp>
#include <boost/program_options/options_description.hpp>

//! \brief CLI parser and storage of options.
class settings {
  public:
    //! \brief Remember to call load_options later!
    settings() {
      set_defaults();
    }

    settings(int argc, char **argv) {
      set_defaults();
      load_options(argc, argv);
    }

    //! \brief Load from files, and override with CLI.  Also deals with exit options like --help.
    //! Actually CLI is parsed first to get conf file related arguments.
    void load_options(int argc, char **argv);

    //! \brief Should we exit immediately?
    bool exit() const { return exit_status() != -1; }
    int exit_status() const { return exit_status_; }

  protected:
    void set_defaults();
    void print_help(boost::program_options::options_description &) const;
    void print_version() const;

  private:
    int exit_status_;
};

#endif
