#ifndef SHARED_DATA_HPP_5f1kan7l
#define SHARED_DATA_HPP_5f1kan7l

#include <queue>
#include <boost/thread.hpp>
#include <para/locking.hpp>

typedef para::sync_traits<std::queue<void *>, boost::mutex, boost::unique_lock<boost::mutex>, boost::condition_variable> sync_traits_type;
typedef sync_traits_type::monitor_tuple_type synced_type;
extern synced_type synced_queue;

// messy - prolly needs a bool callback_finished_printing_data.
extern boost::mutex finish_mut;
extern boost::condition_variable finish_cond;

extern bool finished;

#endif
