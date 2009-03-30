#include "shared_data.hpp"

synced_type synced_queue;

// messy - prolly needs a bool callback_finished_printing_data to do a monitor while on.
boost::mutex finish_mut;
boost::condition_variable finish_cond;

// again messy - should be part of the queue data (it is protected by that mutex)
bool finished = false;
bool output_closed = false;


