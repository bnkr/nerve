// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef STAGES_SDL_HPP_rj23ilz1
#define STAGES_SDL_HPP_rj23ilz1

#include "interfaces.hpp"

namespace stages {
  class sdl : public pipeline::output_stage {
    public:
    void abandon();
    void flush();
    void finish();
    void configure(const char *k, const char *v);
    void output(pipeline::packet *, ::pipeline::outputter *);
    void reconfigure(pipeline::packet *);
  };
}

#endif
