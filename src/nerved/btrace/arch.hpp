// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 * \ingroup grp_arch
 *
 * Architecture detection macros.
 */

#ifndef BTRACE_ARCH_HPP_zmzx7huw
#define BTRACE_ARCH_HPP_zmzx7huw

// TODO:
//   Versions. BTRACE_ARCH_VERSION = value:
//
//   see http://predef.sourceforge.net/prearch.html

#if defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA)
#  define BTRACE_ARCH_ALPHA 1
#elif defined(__amd64__) \
      || defined(__amd64) \
      || defined(__x86_64__) \
      || defined(__x86_64) \
      || defined(_M_X64)
#  define BTRACE_ARCH_AMD64 1
#elif defined(__TARGET_ARCH_ARM)\
      || defined(__TARGET_ARCH_THUMB)\
      || defined(__arm__)\
      || defined(__thumb__)
#  define BTRACE_ARCH_ARM 1
#elif defined(__hppa__) || defined(__hpppa)
#  define BTRACE_ARCH_HPPA 1
#elif defined(i386) \
      || defined(__i386__)\
      || defined(__i486__)\
      || defined(__i586__)\
      || defined(__i686__)\
      || defined(__i386 )\
      || defined(_M_IX86)\
      || defined(__X86__)\
      || defined(_X86_)\
      || defined(__THW_INTEL__)\
      || defined(__I86__)\
      || defined(__INTEL__)
#  define BTRACE_ARCH_X86 1
#elif defined(__ia64__)\
      || defined(_IA64)\
      || defined(__IA64__)\
      || defined(__ia64)\
      || defined(_M_IA64)
#  define BTRACE_ARCH_IA64 1
#elif defined(__m68k__)\
      || defined(M68000)
#  define BTRACE_ARCH_M68K 1
#elif defined(__mips__)\
      || defined(mips)\
      || defined(__mips)\
      || defined(__MIPS__)
#  define BTRACE_ARCH_MIPS 1
#elif defined(__powerpc)\
      || defined(__powerpc__)\
      || defined(__POWERPC__)\
      || defined(__ppc__)\
      || defined(_M_PPC)\
      || defined(_ARCH_PPC)
#  define BTRACE_ARCH_PPC 1
#elif defined(__THW_RS6000)\
      || defined(_IBMR2)\
      || defined(_POWER)\
      || defined(_ARCH_PWR)\
      || defined(_ARCH_PWR2)
#  define BTRACE_ARCH_RS6000 1
#elif defined(__sparc__)\
      || defined(__sparc)
#  define BTRACE_ARCH_SPARC 1
#elif defined(__sh__)
#  define BTRACE_ARCH_SH 1
#elif defined(__370__)\
      || defined(__THW_370__)
#  define BTRACE_ARCH_SYSTEM370 1
#elif defined(__s390__)\
      || defined(__s390x__)
#  define BTRACE_ARCH_SYSTEM390 1
#elif defined(__SYSC_ZARCH__)
#  define BTRACE_ARCH_ZARCH 1
#else
#  error "Can't detect an architecture based on preprocessor defines."
#endif

#endif /* guard */
