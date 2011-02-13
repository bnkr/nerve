#include "../config/config_parser.hpp"
#include <boost/utility.hpp>

namespace cli {
  struct cli_parser;

  //! \ingroup grp_cli
  //! Container and parser for command-line settings.
  class settings : boost::noncopyable {
    public:

    friend class cli_parser;

    typedef ::config::config_parser::files_type config_files_type;

    //! Debugging
    //@{
    bool dump_config() const;
    bool trace_lexer() const;
    bool trace_parser() const;
    //@}

    //! Configuration.
    //@{
    const config_files_type &config_files() const { return config_files_; }
    //@}

    private:
    config_files_type config_files_;
  };
}
