// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include "local_server.hpp"

using namespace server;

local_server::local_server(boost::asio::io_service &io_service, const char *file)
: io_service_(io_service),
  acceptor_(io_service, stream_protocol::endpoint(file)),
  log_(output::source::server)
{
  async_accept();
}

void local_server::handle_accept(session_ptr new_session, const boost::system::error_code &error) {
  if (error) {
    log_.error("accept: %s\n", error.message().c_str());
  }
  else {
    new_session->start();
  }
  async_accept();
}

void local_server::async_accept() {
  session_ptr new_session(session::create_shared(io_service_));
  acceptor_.async_accept(
    new_session->socket(),
    boost::bind(
      &local_server::handle_accept, this, new_session,
      boost::asio::placeholders::error
    )
  );
}
