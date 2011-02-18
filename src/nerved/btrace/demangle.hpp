// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef BTRACE_DEMANGLE_HPP_3liirznl
#define BTRACE_DEMANGLE_HPP_3liirznl

#include <string>
#include <typeinfo>

namespace btrace {
  namespace detail {
    /*!
     * Basic name demangler using the cxxabi standard of Intel and others.
     * Returned memory is on the heap and must be freed.
     *
     * This function usually comes undocumented, so here it is from the
     * specification:
     *
     *   extern "C" char*
     *   __cxa_demangle(
     *       const char* mangled_name,
     *       char* buf,
     *       size_t* n,
     *       int* status
     *     );
     *
     * - mangled_name is a pointer to a null-terminated array of characters. It
     *   may be either an external name, i.e. with a "_Z" prefix, or an internal
     *   NTBS mangling, e.g. of a type for type_info.
     * - buf may be null. If it is non-null, then n must also be nonnull, and
     *   buf is a pointer to an array, of at least *n characters, that was
     *   allocated using malloc.
     * - status points to an int that is used as an error indicator. It is
     *   permitted to be null, in which case the user just doesn't get any
     *   detailed error information.
     *
     * Allocation:
     *
     * - If buf is a null pointer, __cxa_demangle allocates a new buffer with
     *   malloc. It stores the size of the buffer in *n, if n is not NULL.
     * - If buf is not a null pointer, it must have been allocated with malloc.
     *   If buf is not big enough to store the resulting demangled name,
     *   __cxa_demangle must either a) call free to deallocate buf and then
     *   allocate a new buffer with malloc, or b) call realloc to increase the
     *   size of the buffer. In either case, the new buffer size will be stored
     *   in *n.
     */
    char *cxxabi_demangle(const char *name);
  }

  //! \ingroup grp_demangle
  //! Demangle the given mangled name.
  std::string demangle_name(const char *mangled);

  //! \ingroup grp_demangle
  //! Uses the name supplied by typeinfo.
  template<class T>
  std::string demangle_type() { return demangle_name(typeid(T).name()); }
}

#endif
