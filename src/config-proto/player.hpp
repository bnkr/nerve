// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

/*!
 * \file
 *
 * See class \link plauer \endlink.
 */

//! \ingroup grp_organisation
//! Main bit of the player.
class player {
  public:

  //! Return status for the CLI view to deal with.
  enum status { status_ok, status_fail };

  // TODO:
  //   Might be nicer to separate setup and play so that we can be nicer about
  //   errors.

  //! Sets up all the threads and pipes between them.
  enum status play();
};

#include <vector>

void player::play() {
  typedef std::vector<processor_stage_sequence> processor_sequences_type;
  typedef initial_stage_sequence initial_stage_sequence_type;
  typedef std::vector<observer_stage_sequence> obverver_sequences_type;

  typedef std::vector<processor_stage*> processors_type;
  typedef std::vector<observer_stage*> observers_type;

  typedef std::vector<job> jobs_type;

  observer_sequences_type observer_sequences;
  processor_sequences_type processor_sequences;

  jobs_type jobs;

  observers_type observers;

  initial_stage_sequence_type initial_stage_sequence;
}
