#include "stage_sequence.hpp"

using namespace pipeline;

simple_stage *stage_sequence::create_stage(config::stage_config &cfg) {

  // TODO:
  //   What about if it's a plugin that can't be loaded?  How do we communicate
  //   this?
  //
  //   Type safety.  It seems better to have the stages allocated in the
  //   subclass.  Perhaps it's better to develop it there and then abastract
  //   the common bits.


  simple_stage *ss = alloc_and_cons(cfg);


  // To acomplish:
  //
  // - allocate
  // - configure
  // - store
  //



}
