#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <memory>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <string>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.hpp"

using std::string;
using std::runtime_error;
using std::strerror;
using std::cerr;
using std::endl;
using std::shared_ptr;
using std::vector;
using std::ostringstream;
using std::make_shared;
using std::ref;

Server::Server(const ServerOptions& options):
  options(options),
  worker_pool(128) {
  
}

int Server::run() {
  ErrorValue err;
  err = start_listening();
  if (!err.success) {
    cerr << "Could not start: " << err.message << endl;
    return 1;
  } else {
    cerr << "Listening on port " << options.listen_port << endl;
    accept_loop();
    return 0;
  }
}

const ErrorValue Server::start_listening() {
  ErrorValue err;
  err = create_listen_socket();
  if (!err.success) {
    return err;
  }
  err = bind_listen_socket();
  if (!err.success) {
    return err;
  }
  err = listen_on_socket();
  if (!err.success) {
    return err;
  }

  return ErrorValue::ok();
}

const ErrorValue Server::create_listen_socket() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    return ErrorValue::error_from_errno("Failed to open socket: ");
  }
  listen_socket.Set(fd);
  // cerr << "Opened socket " << fd << endl;
  return ErrorValue::ok();
}

const ErrorValue Server::bind_listen_socket() {
  {
    int socket_option_value = 1;
    if (setsockopt(listen_socket.GetFd(),
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   reinterpret_cast<void*>(&socket_option_value),
                   sizeof(socket_option_value)) != 0) {
      return ErrorValue::error_from_errno("Failed to set option SO_REUSEADDR on socket: ");
    }
  }
  {
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(options.listen_port);
    if (bind(listen_socket.GetFd(), reinterpret_cast<struct sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
      return ErrorValue::error_from_errno("Failed to bind socket: ");
    }
  }
  return ErrorValue::ok();
}

const ErrorValue Server::listen_on_socket() {
  if (listen(listen_socket.GetFd(), 5) < 0) {
    return ErrorValue::error_from_errno("Failed to listen on server socket: ");
  }
  return ErrorValue::ok();
}

const ErrorValue socket_accept(const Socket& socket, Socket& result_socket, SocketAddress& result_addr) {
  struct sockaddr_in peer_addr;
  socklen_t addr_len = sizeof(peer_addr);
  int fd = accept(socket.GetFd(), reinterpret_cast<struct sockaddr*>(&peer_addr), &addr_len);
  if (fd < 0) {
    return ErrorValue::error_from_errno("Failed to accept: ");
  }
  Socket result(fd);
  if (addr_len != sizeof(peer_addr)) {
    return ErrorValue::error("Socket address length mismatch");
  }
  result_socket.Set(result.Disown());
  result_addr.ip_addr = ntohl(peer_addr.sin_addr.s_addr);
  result_addr.tcp_port = ntohs(peer_addr.sin_port);
  return ErrorValue::ok();
}

const string ip4_to_string(uint32_t addr) {
  unsigned octets[4] = {
    static_cast<unsigned>((addr >> 24) & 0xFF),
    static_cast<unsigned>((addr >> 16) & 0xFF),
    static_cast<unsigned>((addr >> 8) & 0xFF),
    static_cast<unsigned>((addr) & 0xFF)
  };
  ostringstream stream;
  stream << octets[0] << "." << octets[1] << "." << octets[2] << "." << octets[3];
  return stream.str();
}

void Server::accept_loop() {
  while (true) {
    Socket new_client_socket;
    SocketAddress new_client_addr;
    ErrorValue err = socket_accept(listen_socket, new_client_socket, new_client_addr);
    if (!err.success) {
      cerr << "In accept: " << err.message << endl;
    } else {
      // cerr << "Accepted connection from " << ip4_to_string(new_client_addr.ip_addr) << ":" << new_client_addr.tcp_port << endl;
      shared_ptr<ClientHandler> client_handler = make_shared<ClientHandler>(ref(*this), new_client_socket.Disown(), new_client_addr);
      worker_pool.enqueue([=] {
          client_handler->serve();
        });
    }
  }
}
