#ifndef SHARED_DATA_HPP_5f1kan7l
#define SHARED_DATA_HPP_5f1kan7l

#include <queue>
#include <boost/thread.hpp>
#include <para/locking.hpp>
// TODO:
//   two problems here.  One is the void*.  I need to use an aligned memory
//   class here.  The second is the queue accessor.  Maybe I use
//

typedef para::sync_traits<std::queue<void *>, boost::mutex, boost::unique_lock<boost::mutex>, boost::condition_variable> sync_traits_type;
typedef sync_traits_type::monitor_tuple_type synced_type;
extern synced_type synced_queue;

// messy - prolly needs a bool callback_finished_printing_data.
extern boost::mutex finish_mut;
extern boost::condition_variable finish_cond;

// TODO: this does not need to be shared - give it to read_packets directly.
extern std::size_t sdl_buffer_size;

extern bool finished;

#endif
