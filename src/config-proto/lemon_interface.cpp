#include "lemon_interface.hpp"

#include <memory>
#include <boost/type_traits/alignment_of.hpp>

typedef std::allocator<uint8_t> allocator_type;

void *config::detail::parser_alloc(size_t wanted) {
  const size_t housekeeping = sizeof(size_t);
  const size_t total = wanted + housekeeping;
  allocator_type a;
  uint8_t *const bytes = a.allocate(total);
  size_t *const as_size_t = reinterpret_cast<size_t*>(bytes);
  as_size_t[0] = total;
  uint8_t *ret = reinterpret_cast<uint8_t*>(as_size_t + 1);
  // Note: there will be a std::max_align_t in the future C++ but we just have
  // to assume that this will work for now...
  assert(((size_t) ret) % boost::alignment_of<double>::value == 0);
  return ret;
}

void config::detail::parser_free(void *mem) {
  allocator_type a;
  size_t *const as_size_t = reinterpret_cast<size_t*>(mem);
  const size_t total = as_size_t[-1];
  uint8_t *const bytes = reinterpret_cast<uint8_t*>(as_size_t - 1);
  a.deallocate(bytes, total);
}

