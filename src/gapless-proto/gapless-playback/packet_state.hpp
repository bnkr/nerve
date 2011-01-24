#ifndef PACKET_STATE_HPP_31f8zdup
#define PACKET_STATE_HPP_31f8zdup

//! Seperate stateful object which builds packets.
//!
//! It must be seperate so it can exist in a higher scope than a file
//! and its audio stream.  This is to be used by the audio_decoder, and
//! should never need to be touched by the other code.
class packet_state : boost::noncopyable {
  public:
    //! \brief The packet size is the size of the buffer to pass to the output queue.
    //! Silence value is normally 0, but sometimes it's not!
    packet_state(std::size_t packet_size, int silence_value)
    : packet_(NULL), packet_size_(packet_size), packet_index_(0), silence_value_(silence_value) {
      packet_ = std::malloc(packet_size);
      assert(packet_ != NULL);
    }

    ~packet_state() {
      std::free(packet_);
    }

    std::size_t index() const { return packet_index_; }
    std::size_t size() const { return packet_size_; }

    //! \brief Append the most possible; return how many appened.
    std::size_t append_max(void *buffer, std::size_t max_bytes)  {
      const std::size_t bytes_left = packet_size_ - packet_index_;
      const std::size_t copy_len = std::min(max_bytes, bytes_left);

      uint8_t *sample_buffer = ((uint8_t *) packet_) + packet_index_;
      std::memcpy(sample_buffer, buffer, copy_len);
      packet_index_ += copy_len;

      return copy_len;
    }

    //! \brief Set unused bytes to silence and return the ptr on the heap.
    void *get_final() {
      finalise();
      return clear();
    }

    // TODO:
    //   resize() could be necessary later.  It's a problem if the packet size is
    //   reduced though.  Forcing to clear first is a workaround, but not a very
    //   nice one... possibly it's a neceesary one though if you end up having
    //   to restart the audio thread anyway.

    //! \brief Returns old packet (on the heap).
    void *reset() {
      /// TODO:
      ///   need to use some kind of allocator to get these; also an RAII type.
      void *p = clear();
      assert(packet_ == NULL);
      assert(p != NULL);
      packet_ = std::malloc(packet_size_);
      // std::cout << "reset(): release: " << p << std::endl;
      // std::cout << "reset(): alloc:   " << packet_ << std::endl;
      return p;
    }

    //! \brief Like reset_packet(), but don't allocate a new one.
    void *clear() {
      // this shouldn't be called if we didn't finish it off already.
      assert(index() == size());
      void *p = packet_;
      packet_ = NULL;
      packet_index_ = 0;
      return p;
    }

    //! \brief For observation only!  Use the stateful functions.
    const void *ptr() { return packet_; }

  private:
    //! \brief Set the remainder of the buffer to silence
    void finalise() {
      std::memset(((uint8_t*)packet_ + packet_index_), silence_value_, packet_size_ - packet_index_);
      packet_index_ = packet_size_;
    }

    void *packet_;
    const std::size_t packet_size_;
    std::size_t packet_index_;
    const int silence_value_;
};

#endif
