// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Simple information about stages.
 */

#ifndef STAGES_INFORMATION_HPP_p4g7c2pl
#define STAGES_INFORMATION_HPP_p4g7c2pl

namespace stages {
  //! \ingroup grp_stages
  namespace plug_id {
    // TODO:
    //   This might move around a bit depending on how plugins are eventually
    //   loaded.

    //! \ingroup grp_stages
    enum plugins {
      unset,
      sdl,
      ffmpeg,
      //! Meaning not built in.
      plugin,
      volume
    };
  }

  //! \ingroup grp_stages
  namespace stage_cat {
    //! \ingroup grp_stages
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

  //! \ingroup grp_stages
  //! Categories of plugins (output, input etc.)
  typedef stage_cat::categories category_type;

  //! \ingroup grp_stages
  //! Identifier for known plugins.
  typedef plug_id::plugins plugin_id_type;

  //! \ingroup grp_stages
  //! Name of the enumerator.  Always returns a string.
  const char *get_category_name(category_type c);

  //! \ingroup grp_stages
  //! Name of the enumerator.  Doesn't handle loaded plugins.  Always returns a string.
  const char *get_plugin_enum_name(plugin_id_type);

  //! \ingroup grp_stages
  //! Gets the enum value based on a name.  Doesn't handle loaded plugins, but
  //! does always return a string.
  plugin_id_type get_built_in_plugin_id(const char *);

  //! \ingroup grp_stages
  //! Gets the category type for a known plugin.  Unset if not known or not
  //! built in.
  category_type get_built_in_plugin_category(plugin_id_type);

  //! \ingroup grp_stages
  //! Is the plugin id a built-in one?
  inline bool is_built_in(plugin_id_type p) {
    return p != plug_id::unset && p != plug_id::plugin;
  }
}

#endif
