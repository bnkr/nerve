class pipe_junction {
  public:
    explicit pipe_junction(data_pipe &in, data_pipe &out)
    : in_(in), out_(out) {}

    data_pipe &in() { return in_; }
    data_pipe &out() { return out_; }
};
