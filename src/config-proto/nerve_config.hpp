// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Sets up all defines used for the project.
 */

#ifndef NERVE_CONFIG_HPP_028x8tfz
#define NERVE_CONFIG_HPP_028x8tfz

// Whether to enable tracing code in the lexer.
#if defined(NERVE_LEXER_TRACE) && NERVE_LEXER_TRACE
#  undef NERVE_LEXER_TRACE
#  define NERVE_LEXER_TRACE 1
#else
#  undef NERVE_LEXER_TRACE
#  define NERVE_LEXER_TRACE 0
#endif

// Whether to enable tracing code in the parser.
#if defined(NERVE_PARSER_TRACE) && NERVE_PARSER_TRACE
#  undef NERVE_PARSER_TRACE
#  define NERVE_PARSER_TRACE 1
#else
#  undef NERVE_PARSER_TRACE
#  define NERVE_PARSER_TRACE 0
#endif

#endif
