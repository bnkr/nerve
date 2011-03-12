// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#ifndef SERVER_SOCKET_SERVER_HPP_eamnxrbl
#define SERVER_SOCKET_SERVER_HPP_eamnxrbl

#include "session.hpp"
#include "../util/pooled.hpp"

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>

#include <string>
#include <iostream>
#include <cstdio>

namespace server {
  //! \ingroup grp_server
  //! Tracks connected clients using the session object.
  class local_server {
    // This class is originally based on example code which is part of the boost
    // ASIO library.
    //
    // Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot
    // com).  Distributed under the Boost Software License, Version 1.0. (See
    // accompanying file LICENSE_1_0.txt or copy at
    // http://www.boost.org/LICENSE_1_0.txt)

    private:
    typedef boost::asio::local::stream_protocol stream_protocol;

    public:
    local_server(boost::asio::io_service &io_service, const char *file)
      : io_service_(io_service),
      acceptor_(io_service, stream_protocol::endpoint(file))
    {
      async_accept();
    }

    void handle_accept(session_ptr new_session, const boost::system::error_code &error) {
      if (! error) {
        new_session->start();
        async_accept();
      }
    }

    protected:

    //! Binds a session to its socket.  When the socket is connected to, the
    //! session will be started by +handle_accept+.
    void async_accept() {
      session_ptr new_session(session::create_shared(io_service_));
      acceptor_.async_accept(
        new_session->socket(),
        boost::bind(
          &local_server::handle_accept, this, new_session,
          boost::asio::placeholders::error
        )
      );
    }

    private:
    boost::asio::io_service &io_service_;
    stream_protocol::acceptor acceptor_;
  };
}

#endif
