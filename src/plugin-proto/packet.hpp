// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * Representation of a bit of audio data.  This is what gets passed along the
 * pipeline.
 */
struct packet {
  typedef size_t num_samples_type;
  // Isn't it possible to have other size of samples?
  typedef int16_t samples_type;

  //! So we can tell which song we're on but there's no possiblity for issues of
  //! wraparound and so on.
  struct file_id {
    explicit file_id(int i) : id_(i) {}

    bool operator==(const id_type &rhs) const { this->id_ == rhs.id_; }
    bool operator!=(const id_type &rhs) const { this->id_ != rhs.id_; }

    private:
      int id_;
  };

  struct timestamp {
    int hours();
    int minutes();
    int seconds();
    int miliseconds();

    private:
      native_type stamp_;
  };

  //! Portable timestamp.  Note that this fields's presence here implies that we
  //! can't entirely rely on the timestamp.  Consider a packet being dropped or
  //! another being added.
  timestamp timestamp();
  //! Necessary for various measurements.
  sample_rate_type sample_rate();

  //! Arbitrary number so we can tell what song we're on.
  file_id file_id();

  samples_type *samples_start();
  samples_type *samples_end();

  num_samples_type length();
  size_t size();

  void truncate_start(num_samples_type n);
  void truncate_end(num_samples_type n);

  private:
    // how do we maintain alignment in this arangement?
    samples_t *real_start_;
};
