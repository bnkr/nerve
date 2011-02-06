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

#define LEXER_RETURN_NDEBUG(tok__) return T_ ## tok__;
#define LEXER_RETURN_DEBUG(tok__) \
  std::cout << "lexer: " << line_num() << ": " << tokname__ << ": ";\
  print_yytext(yytext);\
  LEXER_RETURN_NDEBUG(tok__);

#define LEXER_BEGIN_NDEBUG(state__) BEGIN(state__);
#define LEXER_BEGIN_DEBUG(state__) \
  std::cout << "lexer: " << line_num() << ": " << tokname__ << ": ";\
  print_yytext(yytext);\
  LEXER_BEGIN_NDEBUG(tok__);

/* alias debug macros */
#if NERVE_LEXER_TRACE
#  define LEXER_RETURN(tok__) LEXER_RETURN_DEBUG(tok__)
#  define LEXER_BEGIN(tok__) LEXER_BEGIN_DEBUG(tok__)
#else
#  define LEXER_RETURN(tok__) LEXER_RETURN_NDEBUG(tok__)
#  define LEXER_BEGIN(tok__) LEXER_BEGIN_NDEBUG(tok__)
#  define LEXER_MSG_DATA(...)
#endif

/***************
 * Static data *
 ***************/

config::flex_interface::token_data config::flex_interface::detail::current_token;
static bool enable_trace = false;

inline bool trace() { return enable_trace; }

extern void yyset_in(FILE *);

void config::flex_interface::init(const config::flex_interface::params &p) {
  enable_trace = p.trace();
  ::yyset_in(p.stream());
}

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

