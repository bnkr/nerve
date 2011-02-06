/* Copyright (C) James Webber, 2009-2011.  Under a 3-clause BSD license.  See
 * COPYING. */

%option noyywrap
/* %option stack */

/* Large size, but fast */
/* %option fast */

%x string

%{

#ifndef TOKENS_FILE
#  error "-D TOKENS_FILE must be a string for the location of the token defs"
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

  "path"   { LEXER_RETURN(PATH); }
  "plugin" { LEXER_RETURN(PLUGIN); }
}

  /****************
   ** Composites **
   ****************/

<INITIAL>{
  \" { LEXER_BEGIN(string); }
}

<string>{
  \.          { LEXER_MSG_DATA("Deal with escape in string"); }
  [^\"]+      { LEXER_MSG_DATA("Chomp string"); }
  \"          {
    BEGIN(INITIAL);
    LEXER_RETURN(STRING_LIT);
  }
}

  /*************
   ** Utility **
   *************/

<INITIAL>{
  \n         { /* set some location stage */ } 
  {blank}+   {}
  .          { /* raise an error */ }
}

%%

// Gets rid of a useless definedness warning.
inline void dont_call_me() {
  char x;
  int y;
  ::yyunput(y,&x);
}

#ifdef NERVE_LEXER_DEBUG
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
#endif
