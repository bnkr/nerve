// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * Classes which store the configuration of the pipeline.
 */

#ifndef CONFIG_PIPELINE_CONFIGS_HPP_n8dsnrmy
#define CONFIG_PIPELINE_CONFIGS_HPP_n8dsnrmy
#include <vector>

namespace config {

//! \ingroup grp_settings
class stage_config {
  public:

};

//! \ingroup grp_settings
class sequence_config {
  public:

  private:
  std::vector<stage_config> stages_;
};

//! \ingroup grp_settings
class section_config {
  public:

  private:
  std::vector<sequence_config> sequences_;
};

//! \ingroup grp_settings
class job_config {
  public:

  private:
  std::vector<section_config> sections_;
};

//! \ingroup grp_settings
class pipeline_config {
  public:

  private:
  std::vector<job_config> jobs_;
};




}

#endif
