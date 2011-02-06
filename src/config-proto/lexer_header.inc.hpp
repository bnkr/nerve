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

// TODO:
//   - tell the context there was an error
//   - use the error reporter to format the message (separate presentation)
#define LEXER_ERROR(msg__)\
  std::cerr << msg__ << ": " << yytext << std::endl;

/***************
 * Static data *
 ***************/

config::flex_interface::token_data config::flex_interface::detail::current_token;

static bool enable_trace = false;
inline bool trace() { return enable_trace; }

static bool error = false;
static void set_error_state() { error = true; }

/****************
 * Exported API *
 ****************/

extern void yyset_in(FILE *);
extern void yyset_debug(int);

namespace fi = ::config::flex_interface;

void fi::init(const config::flex_interface::params &p) {
  enable_trace = p.trace();
  // it's too much debugging otherwise!
  ::yyset_debug(0);
  ::yyset_in(p.stream());
}

//! Error at end of document.
bool fi::error() { }
//! Stop immediately.
bool fi::fatal_error() { }

/************************
 * Token Data Managment *
 ************************/

static std::allocator<char> char_alloc;

void config::flex_interface::free_text(char *text) {
  /* NERVE_ASSERT(text, "attempting to free null pointer"); */
  // TODO: we should get rid of the need for a strlen, but it's very difficult.
  char_alloc.deallocate(text, std::strlen(text));
}

//! Set string which represents the current token based on yytext.
void set_current_token_text() {
  char *p = char_alloc.allocate(yyleng + 1);
  std::strcpy(p, yytext);
  ::config::flex_interface::detail::current_token.text = p;
}

