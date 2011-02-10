// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * The config file parser.  This combines the flex and lemon bits into stuff we
 * actually want to use.
 */

#ifndef CONFIG_CONFIG_PARSER_HPP_999mx4dk
#define CONFIG_CONFIG_PARSER_HPP_999mx4dk

#include "../plugin-proto/asserts.hpp"

#include <vector>
#include <boost/utility.hpp>

struct syntactic_context;

namespace config {
  class pipeline_config;
  class parse_context;

  /*!
   * \ingroup grp_config
   *
   * Main interface to configuration parsing.  This could have been done with a
   * free function but it seems the state changes around a lot so it's better to
   * go with a class.
   */
  class config_parser : boost::noncopyable {
    public:

    struct params {
      params()
        : trace_general_(false),
          trace_parser_(false),
          trace_lexer_(false),
          lexer_only_(false)
      {}

      bool trace_general() const { return trace_general_; }
      bool trace_parser() const { return trace_parser_; }
      bool trace_lexer() const { return trace_lexer_; }
      bool lexer_only() const { return lexer_only_; }

      params &trace_general(bool v) { trace_general_ = v; return *this; }
      params &trace_parser(bool v) { trace_parser_ = v; return *this; }
      params &trace_lexer(bool v) { trace_lexer_ = v; return *this; }
      params &lexer_only(bool v) { lexer_only_ = v; return *this; }

      private:
      // TODO: use bitset and enum
      bool trace_general_;
      bool trace_parser_;
      bool trace_lexer_;
      bool lexer_only_;
    };

    config_parser(const params &p);

    typedef std::vector<const char *> files_type;

    bool parse(pipeline_config &output, const files_type &files);
    bool parse_file(parse_context &, const char *file);

    private:

    void syntactic_pass(struct syntactic_context &);
    void semantic_pass(parse_context &);

    params p_;
  };
}

#endif
