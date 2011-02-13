#include "pooled.hpp"
#include "asserts.hpp"

#include <algorithm> // max
#include <cstring>

// pool allocator is better than fast pool allocator for contiguous chunks.
static boost::pool_allocator<char> byte_alloc;

void *pooled::tracked_byte_alloc(size_t bs) {
  NERVE_ASSERT(bs > 0, "nonsense value to allocate");
  const size_t total = bs + sizeof(size_t);
  void *base = byte_alloc.allocate(total);
  size_t *as_size_t = (size_t *) base;
  void *user = as_size_t + 1;
  as_size_t[0] = total;
  return user;
}

void pooled::tracked_byte_free(void *user) {
  NERVE_ASSERT(user, "attempting to free null pointer");
  size_t *as_size_t = (size_t*) user;
  byte_alloc.deallocate((char*) (as_size_t - 1), as_size_t[-1]);
}

void *pooled::tracked_byte_realloc(void *ptr, size_t bytes) {
  void *mem = pooled::tracked_byte_alloc(bytes);
  size_t *as_size_t = (size_t *) mem;
  std::memcpy(mem, ptr, std::max(as_size_t[-1], bytes));
  pooled::tracked_byte_free(ptr);
  return mem;
}
