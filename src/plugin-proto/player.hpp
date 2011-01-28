// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/move.hpp>

//! Main bit of the player.
class player {
  public:
    enum status { status_ok, status_fail };

    //! Sets up all the threads and pipes between them.
    enum status play();
};

/*!
 * Base class for each individual stop on the pipe.  It's designed so the stages
 * don't have to care what thread they're in.
 */
class stage {
  virtual void stage() = 0;
};

class process_stage : public stage {
};

class output_stage : public stage {
};

class input_stage : public stage {
};





/*!
 * The groupings of stages.  Each group (which might only contain a single
 * stage) will become our threads.
 */
struct stage_groups {
  public:
    //! The thread, basically.  This will be more like the old process_thread
    //! thing except we need to do it using pointers which kind of sucks but meh.
    //
    //TODO:
    //  This is totally wrong -- it should be the process_thread thing that we
    //  had originally.
    struct stage_group  {
      public:
        void add(stage *s) { stages.push_back(s); }

        // TODO:
        //  This isn't right.  It's doesn't handle pipelines.
        void run() {
          std::for_each(stages.begin(), stages.end(), boost::bind(&call_stage, this));
        }

        void call_run(stage *s) { s->stage(); }

        std::vector<stage *> stages;
    };

    stage_groups() { next_group(); }

    //! Append a stage to the current goup.
    void add_stage(stage *s) {
      groups.back().add(s);
    }

    //! Create a new group.
    void next_group() {
      groups.push_back(stage_group());
    }

    //! Start threads for the each group which has been made so far.
    void dispatch_threads() {
      std::for_each(groups.begin(), groups.end(), boost::bind(&create_thread, this));
    }

    // private
    void create_thread(stage_group &group) {
      thread.create(boost::bind(&stage_group::run, &stage_group.back()));
    }

    void join() { threads.join_all(); }

    boost::thread_group threads;
    std::vector<stage_group> groups;
};


/*!
 * Store the plugins and the drivers that deal with them.  Input and output
 * plugins have special stage drivers becasue they terminate the pipeline.
 */
struct stage_instaces {
  public:
    stage_instances() : input(in_plug), output_(out_plug) {
      for (each plugins => plug) {
        process.push_back(process_stage(plug));
      }
    }

    input_stage input;
    output_stage output;
    std::vector<process_stage> process;
};

//! Sorts the different stages into groups; each group will be the thread.
void create_groups(stage_instances stages, stage_groups groups) {
  groups.add_stage(&stages.input);

  size_t assigned = 1;
  process_type::iterator process_i = stages.process.begin();
  conf_iter_type conf_i = configs.begin();
  configs_type &configs = settings::thread_configs();
  while (conf_i != configs.end()) {
    const size_t remaining = conf_i->size() - assigned;

    while (remaining > 0) {
      groups.add_stage(&(*process_i));
      ++process_i;

      // Run out of processor plugins -- we must want to put the output plugin
      // in here.
      if (process_i == groups.process.end()) {
        goto add_output_stage;
      }

      ++assigned;
    }

    groups.next_group();
    ++conf_i;
    assigned = 0;
  }

add_output_stage:
  groups.add_stage(&stages.output);
}

void create_plumbing(...) {
  // TODO:
  //   link up each group with the apropriate pipe.  This m eans pointers and
  //   some kind of storage for all the queuses.
}

enum player::status player::play() {
  socket_server server;

  stage_instances stages;
  stage_groups groups;

  create_groups(stages, groups);

  create_plumbing(...);

  groups.dispatch_threads();

  server.run();
  groups.join();
}
