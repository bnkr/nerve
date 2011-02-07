// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Static bits to be included by the lexer so we don't need tonnes of C++ in the
 * .l and so we can use quote includes properly.
 */

#include "flex_interface.hpp"
#include "nerve_config.hpp"
#include "parse_context.hpp"
#include "../plugin-proto/asserts.hpp"

#include <cstring>
#include <iostream>

/***********************
 * Lexer Action Macros *
 ***********************/

static void print_yytext(const char *);
// defined in the lexer because the defs aren't made yet
static const char *state_name(int num);

#define LEXER_MSG_YYTEXT(msg__)\
  if (::trace()) {\
    std::cout << "lexer: " << yyget_lineno() << ": " << msg__ << ": ";\
    print_yytext(yytext);\
  }

#define LEXER_RETURN_NDEBUG(tok__) return T_ ## tok__;
#define LEXER_RETURN_DEBUG(tok__) \
  LEXER_MSG_YYTEXT(#tok__);\
  LEXER_RETURN_NDEBUG(tok__);\

#define LEXER_BEGIN_NDEBUG(state__) BEGIN(state__);
#define LEXER_BEGIN_DEBUG(state__) \
  LEXER_MSG_YYTEXT("switch state to " << state_name(state__));\
  LEXER_BEGIN_NDEBUG(state__);\

/* alias debug macros */
#ifndef NERVE_LEXER_TRACE
#  error "NERVE_LEXER_TRACE should be defined by nerve_config.hpp"
#elif NERVE_LEXER_TRACE
#  define LEXER_RETURN(tok__) LEXER_RETURN_DEBUG(tok__)
#  define LEXER_BEGIN(tok__) LEXER_BEGIN_DEBUG(tok__)
#  define LEXER_MSG(msg__) LEXER_MSG_YYTEXT(msg__)
#else
#  define LEXER_RETURN(tok__) LEXER_RETURN_NDEBUG(tok__)
#  define LEXER_BEGIN(tok__) LEXER_BEGIN_NDEBUG(tok__)
#  define LEXER_MSG(...)
#endif

#define LEXER_NEWLINE() context->reporter().increment_line();

// TODO:
//   - tell the context there was an error
//   - use the error reporter to format the message (separate presentation)
#define LEXER_ERROR(fmt__, ...) context->reporter().report(fmt__, __VA_ARGS__);

/***************
 * Static data *
 ***************/

config::flex_interface::token_type config::flex_interface::detail::current_token;

static ::config::flex_interface::context_type *context;
static bool enable_trace = false;
inline bool trace() { return enable_trace; }

/****************
 * Exported API *
 ****************/

extern void yyset_in(FILE *);
extern void yyset_debug(int);

namespace fi = ::config::flex_interface;

void fi::init(const config::flex_interface::params &p) {
  context = NERVE_CHECK_PTR(p.context());
  enable_trace = p.trace();
  // only necessary for checking the regex themselves
  ::yyset_debug(0);
  ::yyset_in(NERVE_CHECK_PTR(p.stream()));
}

/************************
 * Token Data Managment *
 ************************/

static std::allocator<char> char_alloc;

void config::flex_interface::free_text(char *text) {
  NERVE_ASSERT(text, "attempting to free null pointer");
  // TODO: we should get rid of the need for a strlen, but it's very difficult.
  char_alloc.deallocate(text, std::strlen(text));
}

//! Copy-allocate the string and assign it to the current token text.
static void assign_token_text(const char *copy, size_t length) {
  char *p = char_alloc.allocate(length);
  std::strncpy(p, copy, length);
  ::config::flex_interface::detail::current_token.text = p;
}

/************************************
 * Creating strings and identifiers *
 ************************************/

static std::string buffer;

static void string_append_escape() {
  NERVE_ASSERT(yytext[0] == '\\', "this must only be called when there is an escape char");
  NERVE_ASSERT(yytext[1] != '\0', "there must always be exactly two text chars in an escape");
  NERVE_ASSERT(yytext[2] != '\0', "there must always be exactly two text chars in an escape");

  const char c = yytext[1];

  switch(c) {
  case '"':
  case '\\':
    buffer += c;
    break;
  default:
    LEXER_ERROR("invalid escape in string: %s", yytext);
    buffer += yytext;
  }
}

static void string_append_newline() { buffer += "\n"; }
static void string_append_text() { buffer += yytext; }
static void string_assign_token_text() {
  assign_token_text(buffer.c_str(), buffer.length() + 1);
  buffer.clear();
}

static void identifier_assign_token_text() { assign_token_text(yytext, yyleng + 1); }
