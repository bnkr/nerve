// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.
#ifndef SERVER_SESSION_HPP_mk6t8fr4
#define SERVER_SESSION_HPP_mk6t8fr4

#include "protocol.hpp"
#include "../output/logging.hpp"

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include <boost/array.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

namespace server {
  //! \ingroup grp_server
  //! An individual communication with a client.  Note the inheritance means we
  //! can convert a raw pointer into a shared_ptr (and it has to be public).
  class session : public boost::enable_shared_from_this<session>, boost::noncopyable {
    // This class is originally based on example code which is part of the boost
    // ASIO library.
    //
    // Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot
    // com).  Distributed under the Boost Software License, Version 1.0. (See
    // accompanying file LICENSE_1_0.txt or copy at
    // http://www.boost.org/LICENSE_1_0.txt)

    private:
    typedef boost::asio::local::stream_protocol stream_protocol;
    typedef stream_protocol::socket socket_type;

    public:
    typedef boost::shared_ptr<session> shared_ptr;

    explicit session(boost::asio::io_service &io_service)
      : socket_(io_service),
        log_(output::source::server)
    { }

    ~session() {
      log_.trace("session %p closed\n", (void*) this);
    }

    static shared_ptr create_shared(boost::asio::io_service &sv);

    socket_type &socket() { return socket_; }

    //! Start this session listening.
    void start();

    //! Deal with some data coming in.
    void handle_read(const boost::system::error_code &error, size_t bytes_transferred);

#if 0
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
#endif

    private:
    stream_protocol::socket socket_;
    boost::array<char, 1024> data_;
    output::logger log_;
  };

  typedef session::shared_ptr session_ptr;
}

#endif
