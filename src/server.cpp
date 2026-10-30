#include "server.hpp"
#include <cerrno>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>

Server::Server() {
}

Server::~Server() {
  if (m_socket == -1)
    return;
  close(m_socket);
  m_socket = -1;
}

void Server::start(const std::size_t &p_port) {
  if (m_socket != -1)
    throw std::runtime_error("Server: multiple server start");
  m_socket = ::socket(AF_INET, SOCK_STREAM, 0);

  if (m_socket < 0) {
    throw std::runtime_error("Server: can not initialize socket");
  }

  int opt = 1;
  if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(m_socket);
    m_socket = -1;
    throw std::runtime_error("Server: setsockopt failed");
  }

  struct sockaddr_in addr;
  std::memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(p_port);

  if (bind(m_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(m_socket);
    m_socket = -1;
    throw std::runtime_error("Server: bind failed");
  }

  if (listen(m_socket, SOMAXCONN) < 0) {
    close(m_socket);
    m_socket = -1;
    throw std::runtime_error("Server: listen failed");
  }

  if (fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK) < 0) {
    close(m_socket);
    m_socket = -1;
    throw std::runtime_error("Server: fcntl failed");
  }
}

void Server::defineAction(const Message::Type &messageType, const std::function<void(long long &clientID, const Message &msg)> &action) {
  m_actions[messageType] = action;
}

void Server::sendTo(const Message &message, long long clientID) {
  auto it = m_clients.find(clientID);

  if (it == m_clients.end())
    return;

  send(message, it->second);
}

void Server::sendToArray(const Message &message, std::vector<long long> clientIDs) {
  for (long long id : clientIDs) {
    auto it = m_clients.find(id);
    if (it == m_clients.end())
      continue;
    send(message, it->second);
  }
}

void Server::sendToAll(const Message &message) {
  for (auto it = m_clients.begin(); it != m_clients.end(); it++) {
    send(message, it->second);
  }
}

void Server::accept() {
  while (true) {
    int fd = ::accept(m_socket, nullptr, nullptr);
    if (fd < 0) {
      break;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    Client &c = m_clients[++m_clientId];
    c.m_socket = fd;
  }
}

void Server::recv() {
  uint8 tmp[4096];
  for (auto it = m_clients.begin(); it != m_clients.end(); it++) {
    long long id = it->first;
    while (true) {
      ssize_t n = ::recv(it->second.m_socket, tmp, sizeof(tmp), 0);
      if (n > 0) {
        it->second.m_recvBuffer.insert(it->second.m_recvBuffer.end(), tmp, tmp + n);
        continue;
      }
      if (n == 0) {
        it->second.disconnect();
        break;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      if (errno == EINTR)
        continue;
      it->second.disconnect();
      break;
    }
    if (it->second.m_socket == -1)
      continue;
    std::size_t offset = 0, used;
    Message msg(0);
    while ((used = Message::deserialize(it->second.m_recvBuffer, offset, msg)) > 0) {
      offset += used;
      processAction(id, msg);
    }
    it->second.m_recvBuffer.erase(it->second.m_recvBuffer.begin(), it->second.m_recvBuffer.begin() + offset);
  }
}

void Server::update() {
  accept();
  recv();

  for (auto it = m_clients.begin(); it != m_clients.end();) {
    if (it->second.m_socket == -1)
      it = m_clients.erase(it);
    else
      ++it;
  }
}

void Server::send(const Message &message, Client &client) {
  if (client.m_socket == -1)
    return;

  try {
    client.send(message);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Server: send failed" << std::endl;
  }
}

void Server::processAction(long long &client_id, const Message &msg) {
  Message::Type type = msg.type();

  auto it = m_actions.find(type);
  if (it == m_actions.end())
    return;
  (it->second)(client_id, msg);
}