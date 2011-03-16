// Copyright (C) 2008-2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef STAGES_AVLIBS_HPP_7ac6duos
#define STAGES_AVLIBS_HPP_7ac6duos

extern "C" {
// ffmpeg can actually be configured to avoid using this, but the particular
// headers I have don't.
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

// These will probably be wrong, but they move around so much between versions
// that it's better to organise it with -I flags (or at worse symlinks).
#include <avcodec.h>
#include <avformat.h>
}

#endif
