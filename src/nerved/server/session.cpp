// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "session.hpp"

#include "../util/pooled.hpp"

using namespace server;

session::shared_ptr session::create_shared(boost::asio::io_service &sv) {
  return shared_ptr(::pooled::alloc1<session>(sv), pooled::call_free<session>());
}

void session::start() {
  log_.trace("session %p: starting\n", (void*) this);
  socket_.async_read_some(
    boost::asio::buffer(data_),
    boost::bind(
      &session::handle_read,
      shared_from_this(),
      boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred
    )
  );
}

void session::handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
  if (error) {
    log_.error("session %p: read: %s\n", (void*) this, error.message().c_str());
    this->socket().shutdown(socket_type::shutdown_both);
  }
  else {
    if (std::strncmp(data_.begin(), "BYE", bytes_transferred) == 0) {
      this->socket().shutdown(socket_type::shutdown_both);
    }
    else if (std::strncmp(data_.begin(), "MADAGASCAR", bytes_transferred) == 0) {
      // TODO:
      //   How do I inform all clients I am shutting down?
      socket_.get_io_service().stop();
      return;
    }

#if 0
    boost::asio::async_write(
      socket_,
      boost::asio::buffer(data_, bytes_transferred),
      boost::bind(
        &session::handle_write,
        shared_from_this(),
        boost::asio::placeholders::error
      )
    );
#endif
  }
}
