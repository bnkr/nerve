// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef PIPELINE_STAGE_DATA_HPP_cxziwt7m
#define PIPELINE_STAGE_DATA_HPP_cxziwt7m

namespace pipeline {
  //! \ingroup grp_pipeline
  namespace built_stages {
    // TODO:
    //   This might move around a bit depending on how plugins are eventually
    //   loaded.

    //! \ingroup grp_pipeline
    enum stage_ids {
      unset,
      sdl,
      ffmpeg,
      //! Meaning not built in.
      plugin,
      volume
    };
  }

  //! \ingroup grp_pipeline
  namespace stage_cat {
    //! \ingroup grp_pipeline
    enum categories {
      // Note that a separate type is necessary for the output despite it
      // essentially being an observer because we need to know that one exists in
      // the pipeline.
      output,
      input,
      process,
      observe,
      unset
    };
  }

  //! \ingroup grp_pipeline
  //! Categories of plugins (output, input etc.)
  typedef stage_cat::categories stage_category_type;

  //! \ingroup grp_pipeline
  //! Identifier for known plugins.
  typedef built_stages::stage_ids built_stage_id_type;

  //! \ingroup grp_pipeline
  //! Name of the enumerator.
  const char *get_category_name(stage_category_type c);
}
#endif
