// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Sets up all defines used for the project.
 */

#ifndef CONFIG_TRACE_DEFINES_HPP_4dbnc5cx
#define CONFIG_TRACE_DEFINES_HPP_4dbnc5cx

#include "../defines.hpp"

#ifndef NERVE_DEVELOPER
#  error "NERVE_DEVELOPER must be defined to a true or false value"
#endif

// Whether to enable tracing code in the lexer.
#ifndef NERVE_TRACE_LEXER
#  if NERVE_DEVELOPER
#    define NERVE_TRACE_LEXER 1
#  else
#    define NERVE_TRACE_LEXER 0
#  endif
#endif

// Whether to enable tracing code in the parser.
#ifndef NERVE_TRACE_PARSER
#  if NERVE_DEVELOPER
#    define NERVE_TRACE_PARSER 1
#  else
#    define NERVE_TRACE_PARSER 0
#  endif
#endif

#endif
