/*!
\file
\brief Global data
*/
#ifndef SHARED_DATA_HPP_v13oue0m
#define SHARED_DATA_HPP_v13oue0m

#include <para/locking.hpp>
#include <boost/thread.hpp>

#include <queue>

// TODO:
//   we can almost certainly get rid of a lot of this out of the global space.
//   The packet queue can't really because it's needed by the sdl callback.
//
// TODO:
//   packet queue should have the finished boolean in it, and perhaps also the
//   condition that we wait on for being finished...

typedef para::sync_traits<std::queue<void *>, boost::mutex, boost::unique_lock<boost::mutex>, boost::condition_variable> sync_traits_type;
typedef sync_traits_type::monitor_tuple_type synced_type;
extern synced_type synced_queue;

// TODO:
//   Prolly get rid of these.  We'll block forever on the playlist queue.
extern boost::mutex finish_mut;
extern boost::condition_variable finish_cond;

// again messy - should be part of the queue data (it is protected by that mutex)
extern bool finished;
extern bool output_closed;

inline bool continue_predicate() {
  return finished || ! synced_queue.data().empty();
}
#endif
