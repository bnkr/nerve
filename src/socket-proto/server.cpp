// Copyright (C) 2011, James Webber.
// Distributed under a 3-clause BSD license.  See COPYING.

#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#include <string>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <iostream>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

//! An individual communication with a client.  Note the inheritance means we
//! can convert a raw pointer into a shared_ptr (and it has to be public).
class session : public boost::enable_shared_from_this<session> {
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

//! Tracks connected clients using the session object.
class server {
  private:
    typedef boost::asio::local::stream_protocol stream_protocol;

  public:
    server(boost::asio::io_service &io_service, const std::string &file)
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
      session_ptr new_session(new session(io_service_));
      acceptor_.async_accept(
        new_session->socket(),
        boost::bind(
          &server::handle_accept, this, new_session,
          boost::asio::placeholders::error
        )
      );
    }

  private:
    boost::asio::io_service &io_service_;
    stream_protocol::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
  try {
    boost::asio::io_service io_service;

    const char *const file = "/tmp/nerve.socket";

    std::remove(file);
    server s(io_service, file);

    io_service.run();
  }
  catch (std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
  }

  return 0;
}

// http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/reference.html

