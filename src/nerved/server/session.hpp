// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef SERVER_SESSION_HPP_mk6t8fr4
#define SERVER_SESSION_HPP_mk6t8fr4

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

// #include <string>
// #include <cstdlib>
// #include <iostream>
// #include <unistd.h>
// #include <cstdio>

namespace server {
  //! \ingroup grp_server
  //! An individual communication with a client.  Note the inheritance means we
  //! can convert a raw pointer into a shared_ptr (and it has to be public).
  class session : public boost::enable_shared_from_this<session> {
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
    session(boost::asio::io_service &io_service)
      : socket_(io_service)
    { }

    stream_protocol::socket &socket() { return socket_; }

    void start() {
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

    void handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
      if (! error) {
        boost::asio::async_write(
          socket_,
          boost::asio::buffer(data_, bytes_transferred),
          boost::bind(
            &session::handle_write,
            shared_from_this(),
            boost::asio::placeholders::error
          )
        );
      }
    }

    void handle_write(const boost::system::error_code &error) {
      if (! error) {
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
    }

    private:
    stream_protocol::socket socket_;
    boost::array<char, 1024> data_;
  };

  typedef boost::shared_ptr<session> session_ptr;
}

#endif
