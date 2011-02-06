// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Static bits of the grammar.
 */

#include "flex_interface.hpp"
#include "lemon_interface.hpp"
#include "nerve_config.hpp"
#include "parse_context.hpp"
#include "../plugin-proto/asserts.hpp"

#include <iostream>
#include <cstdlib>
#include <cassert>

// The lemon parser only puts its debugging routines in when NDEBUG is not
// defined.
//
// NOTE: remember to define this *after* assert.hpp is included!  So I can
// have a parse trace even with no asserts.
#ifndef NERVE_PARSER_TRACE
#  error "NERVE_LEXER_TRACE should be defined by config.hpp"
#elif NERVE_PARSER_TRACE
#  undef NDEBUG
#else
#  undef NDEBUG
#  define NDEBUG
#endif

#define ERR(...) context->reporter().report(__VA_ARGS__);

// TODO:
//   This only works with NDEBUG defined.
#define ERR_EXPECTED(what__)\
  context->reporter().report("expected %s but got %s", what__, yyTokenName[get_last_error().token]);

template<class T> void use_variable(const T &) {}

typedef ::config::flex_interface::token_type minor_type;

// This data is only remotely useful if the token names are availab.e
struct error_data {
  int token;
  minor_type data;
};

static error_data last_error;

/*!
 * Stores some state so we can report what token caused an error.  This
 * functionality is totally undocumented in lemon.  It looks like the minor data
 * is supposed to be accessed in the error handler.
 */
static void set_last_error(int major, minor_type minor) {
  last_error.token = major;
  last_error.data = minor;
}
const error_data &get_last_error() { return last_error; }
