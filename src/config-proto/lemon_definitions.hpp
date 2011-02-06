// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Static bits to be included by the lexer so we don't need tonnes of C++ in the
 * .l and so we can use quote includes properly.  This has to be given to the
 * lexer as a -D.
 */

#include "flex_interface.hpp"
#include "lemon_interface.hpp"

#include <cassert>

// The lemon parser only puts its debugging routines in when NDEBUG is not
// defined.
//
// NOTE: remember to define this *after* assert.hpp is included!  So I can
// have a parse trace even with no asserts.
#if defined(NERVE_ENABLE_PARSER_TRACE) && NERVE_ENABLE_PARSER_TRACE
#  undef NDEBUG
#else
#  define NDEBUG
#endif

#include <iostream>
#include <cstdlib>

template<class T> void use_variable(const T &) {}

