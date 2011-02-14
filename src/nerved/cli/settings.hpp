// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef CLI_SETTINGS_HPP_udliv6ow
#define CLI_SETTINGS_HPP_udliv6ow

#include "../config/config_parser.hpp"
#include "../util/asserts.hpp"

#include <boost/utility.hpp>

namespace cli {
  struct cli_parser;

  //! \ingroup grp_cli
  //! Container and parser for command-line settings.
  class settings : boost::noncopyable {
    public:

    friend class cli_parser;

    typedef ::config::config_parser::files_type config_files_type;

    settings();

    //! Debugging
    //@{
    bool dump_config() const { return dump_config_; }
    bool trace_lexer() const { return trace_lexer_; }
    bool trace_parser() const { return trace_parser_; }
    //@}

    //! Configuration.
    //@{
    const config_files_type &config_files() const { return config_files_; }
    //@}

    //! Operation
    //@{

    //! File to store player state in.
    const char *state() const { return NERVE_CHECK_PTR(state_); }
    const char *socket() const { return NERVE_CHECK_PTR(socket_); }
    const char *log() const { return NERVE_CHECK_PTR(log_); }

    //@}

    private:
    config_files_type config_files_;
    bool trace_lexer_;
    bool trace_parser_;
    bool dump_config_;
    const char *state_;
    const char *socket_;
    const char *log_;
  };
}
#endif
