#include "shared_data.hpp"

synced_type synced_queue;

boost::mutex finish_mut;
boost::condition_variable finish_cond;

std::size_t sdl_buffer_size = 0;
bool finished = false;

