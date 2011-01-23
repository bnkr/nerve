#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::local::stream_protocol;

enum { max_length = 1024 };

int main(int argc, char* argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: stream_client <file>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    stream_protocol::socket s(io_service);
    s.connect(stream_protocol::endpoint(argv[1]));

    char request[max_length];
    // if (isatty(fileno(stdin)) && isatty(fileno(stdout))) {
    //   std::cout << "Enter message: ";
    //   std::cin.getline(request, max_length);
    // }
    // else {
      std::strcpy(request, "uber pantsness\n");
    // }

    size_t request_length = std::strlen(request);
    boost::asio::write(s, boost::asio::buffer(request, request_length));

    char reply[max_length];
    size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply, request_length));
    std::cout << "Reply is: ";
    std::cout.write(reply, reply_length);
    std::cout << "\n";
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

