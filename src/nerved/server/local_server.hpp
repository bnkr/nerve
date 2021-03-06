// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef SERVER_SOCKET_SERVER_HPP_eamnxrbl
#define SERVER_SOCKET_SERVER_HPP_eamnxrbl

#include "session.hpp"
#include "../output/logging.hpp"

#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>

#include <string>
#include <iostream>
#include <cstdio>

namespace server {
  //! \ingroup grp_server
  //! Tracks connected clients using the session object.
  class local_server : boost::noncopyable {
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
    local_server(boost::asio::io_service &io_service, const char *file);
    void handle_accept(session_ptr new_session, const boost::system::error_code &error);

    protected:

    //! Binds a session to its socket.  When the socket is connected to, the
    //! session will be started by +handle_accept+.
    void async_accept();

    private:
    boost::asio::io_service &io_service_;
    stream_protocol::acceptor acceptor_;
    ::output::logger log_;
  };
}
#endif
