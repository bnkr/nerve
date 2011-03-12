#include "assert.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

using namespace btrace;

void detail::assert_true(bool b) {
  if (b) return;
  std::abort();
}

void detail::assert_true(bool b, cstr_t file, int line) {
  if (b) return;
  std::fprintf(stderr, "assert failure at %s:%d\n", file, line);
  std::abort();
}

void detail::assert_true(bool b, cstr_t file, int line, cstr_t expr, cstr_t doc) {
  if (b) return;

  if (std::strcmp(doc, "") == 0) {
    std::fprintf(stderr, "assert:\n  %s:%d\n  %s\n", file, line, expr);
  }
  else {
    std::fprintf(stderr, "assert:\n  %s:%d\n  %s\n  %s\n", file, line, expr, doc);
  }
  std::abort();
}

void *detail::assert_nn_static(void * p) {
  if (p) return p;
  std::abort();
}

void *detail::assert_nn_static(void *p, cstr_t file, int line) {
  if (p) return p;
  std::fprintf(stderr, "assert failure at %s:%d\n", file, line);
  std::abort();
}

void *detail::assert_nn_static(void *p, cstr_t file, int line, cstr_t expr, cstr_t doc) {
  if (p) return p;

  if (std::strcmp(doc, "") == 0) {
    doc = "expression must not yield a null pointer";
  }
  std::fprintf(stderr, "assert:\n  %s:%d\n  %s\n  %s\n", file, line, expr, doc);
  std::abort();
}
