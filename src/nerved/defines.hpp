// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * Definitions used by the program decide what features to enable etc.  Also
 * some utility ones.
 *
 * Parameters:
 *
 * - NERVED_CONFIG -- a file to include which might define other stuff.
 *
 * Defines:
 *
 * - NERVED_VERSION -- string representing the version.
 * - NERVED_DEVELOPER -- implies tonnes of debug code
 */

#ifndef CONFIG_DEFINES_HPP_mtq7guzq
#define CONFIG_DEFINES_HPP_mtq7guzq

#ifdef NERVED_CONFIG
#  include NERVED_CONFIG
#endif

#ifndef NERVE_DEVELOPER
#  define NERVE_DEVELOPER 0
#endif

#ifndef NERVED_VERSION
#  error "NERVED_VERSION must be defined"
#endif

#endif
