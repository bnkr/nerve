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
#include "pipeline_configs.hpp"
#include "../plugin-proto/asserts.hpp"

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstring>

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

#include TOKENS_FILE

// token name needs to be in the macro because it's declared static (yeah yeah I
// shouldn't really be using it anyway).
#define ERR_EXPECTED(what__) err_expected(what__, context, ::yyTokenName[get_last_error().token]);

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

static void err_expected(const char *exp, config::parse_context *pc, const char *tok_name) {
  const char *val = 0;
  switch (get_last_error().token) {
  case T_IDENTIFIER_LIT:
  case T_STRING_LIT:
    val = get_last_error().data.text;
    break;
  default:
    val = tok_name;
  }

  NERVE_CHECK_PTR(pc)->reporter().report("expected %s but got %s", exp, val);
}

/*******************
 * Creating stages *
 *******************/

using config::flex_interface::make_text_ptr;
using config::stage_config;
