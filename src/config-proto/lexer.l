/* Copyright (C) James Webber, 2009-2011.  Under a 3-clause BSD license.  See
 * COPYING. */

%option noyylineno
%option noyywrap
%option nodefault

%x STRING

%{

#ifndef TOKENS_FILE
#  error "-D TOKENS_FILE must be a quoted string of the location of lemon's token defs header"
#  error "If you are compiling using the CMake build system then this is a bug."
#endif

// Defined by the build system courtesy bbuild_parser.
#include TOKENS_FILE
// We can't use quote includes for compiler reasons so we need an -I.  Then the
// inc file can use quote paths and dependencies are all correct.
#include <lexer_header.inc.hpp>

%}

id [a-zA-Z_][a-zA-Z0-9_]*

int_lit  [0-9][0-9]*(\.[0-9]+)?
hex_lit  0x{int_lit}
oct_lit  0{int_lit}
number   {hex_lit}|{int_lit}|{oct_lit}

/* Ws except newline */
blank [\t \r]
ws    [\t \n\r]

%%

  /*******************
   * Simple Keywords *
   *******************/

<INITIAL>{
  "="      { LEXER_RETURN(EQ); }
  "{"      { LEXER_RETURN(LBRACE); }
  "}"      { LEXER_RETURN(RBRACE); }

  "thread"  { LEXER_RETURN(THREAD); }
  "section" { LEXER_RETURN(SECTION); }
  "stage"   { LEXER_RETURN(STAGE); }
  "input"   { LEXER_RETURN(INPUT); }
  "output"  { LEXER_RETURN(OUTPUT); }

  "path"    { LEXER_RETURN(PATH); }
  "type"    { LEXER_RETURN(TYPE); }
  "after"   { LEXER_RETURN(AFTER); }
  "name"    { LEXER_RETURN(NAME); }
}

  /****************
   ** Composites **
   ****************/

<INITIAL>{
  \"           { LEXER_BEGIN(STRING); }
  [a-zA-Z0-9]+ { identifier_assign_token_text(); LEXER_RETURN(IDENTIFIER_LIT); }
}

<STRING>{
  \\.         { string_append_escape();  }
  \n          { string_append_newline(); LEXER_NEWLINE(); }
  [^\"]+      { string_append_text(); }
  \"          {
    string_assign_token_text();
    LEXER_BEGIN(INITIAL);
    LEXER_RETURN(STRING_LIT);
  }
}

  /*************
   ** Utility **
   *************/

<INITIAL>{
  \n         { LEXER_NEWLINE(); }
  {blank}+   {}
  .          { LEXER_ERROR("unmatched character: %c", yytext[0]); }
}

%%

// Gets rid of a useless definedness warning.
inline void dont_call_me() {
  char x;
  int y;
  ::yyunput(y,&x);
}

#ifndef NERVE_LEXER_TRACE
#  error "NERVE_LEXER_TRACE should be defined by nerve_config.hpp"
#endif

#if NERVE_LEXER_TRACE
static void print_yytext(const char *text) {
  size_t len = std::strlen(text), i = 0;
  while (i < len) {
    char c = text[i++];
    if (c == '\n') {
      std::cout << "\\n";
    }
    else {
      std::cout << c;
    }
  }
  std::cout << std::endl;
}

static const char *state_name(int num) {
  switch(num) {
  case INITIAL:
    return "INITIAL";
  case STRING:
    return "string";
  default:
    return "<unknown state>";
  }
}
#endif
