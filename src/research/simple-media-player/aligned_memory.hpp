#ifndef ALIGNED_MEMORY_HPP_gidtjkws
#define ALIGNED_MEMORY_HPP_gidtjkws

#include <cstdlib>
#include <boost/utility.hpp>

// How can I statically assert that Alignment is a power of two?
// TODO: unit test this
// TODO:
//   It would be nice to have a variable sized buffer here, like bounded_buffer.  In
//   fact it would be even nicer to write the bounded_* class in terms of this; ie,
//   optionally give an alligned memory or just a standard memory.  And using an
//   alloctor pattern wouldn't be bad.
// TODO:
//   There's not much point in having this kind of storage unless you can make the
//   buffer of a variable size; otherwise just use static_aligned_memory.
template <std::size_t Alignment, class T = uint8_t>
class dynamic_aligned_memory : public boost::noncopyable {
  public:
    typedef T  value_type;
    typedef T* pointer_type;

    static const std::size_t alignment = Alignment;

    // \brief Size in num elements.
    dynamic_aligned_memory(std::size_t size) { alloc(size); }
    ~dynamic_aligned_memory() { std::free(base_); }

    //! \brief Return aligned storage.
    T *ptr() { return aligned_base_; }
    const T *ptr() const { return aligned_base_; }

  private:
    void alloc(std::size_t size) {
      base_ = (T*) std::malloc(size * sizeof(T) + Alignment);
      std::size_t remainder = ((std::size_t) base_) % Alignment;
      aligned_base_ = (T*) (((std::size_t) base_) + remainder);
      assert(((std::size_t) aligned_base_) % Alignment == 0);
    }

    T *base_;
    T *aligned_base_;
};

//! \brief Size is the number of elements.
// TODO: unit test this
template <std::size_t Alignment, std::size_t Size, class T = uint8_t>
class aligned_memory : public boost::noncopyable {
  public:
    typedef T  value_type;
    typedef T* pointer_type;

    static const std::size_t alignment  = Alignment;
    static const std::size_t size       = Size;
    static const std::size_t byte_size  = Size * sizeof(T);

    aligned_memory() {
      const std::size_t byte_offset = ((std::size_t) data_) % Alignment;
      base_ = (T*) (((std::size_t) data_) + byte_offset);
      assert(((std::size_t) base_) % Alignment == 0);
    }

    T *ptr() { return base_; }
    const T *ptr() const { return base_; }

  private:
    uint8_t data_[Size * sizeof(T) + Alignment];
    T * const base_;
};

#endif
