// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Static bits of the grammar.
 */

#include "flex_interface.hpp"
#include "lemon_interface.hpp"
#include "nerve_config.hpp"

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

#include <iostream>
#include <cstdlib>

template<class T> void use_variable(const T &) {}
