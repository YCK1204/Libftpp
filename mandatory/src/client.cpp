#include "client.hpp"
#include "message.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>

Client::Client() {
  m_recvBuffer.reserve(65535);
}

Client::~Client() {
  disconnect();
}

void Client::connect(const std::string &address, const std::size_t &port) {
  disconnect();
  m_recvBuffer.clear();
  m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in servAddr;

  if (m_socket < 0) {
    throw std::runtime_error("Client: can not initialize socket");
  }

  std::memset(&servAddr, 0, sizeof(sockaddr_in));

  if (::inet_pton(AF_INET, address.data(), &servAddr.sin_addr) <= 0) {
    disconnect();
    throw std::invalid_argument("Client: address [" + address + "] is not valid");
  }
  servAddr.sin_port = htons(port);
  servAddr.sin_family = AF_INET;

  if (::connect(m_socket, (struct sockaddr *)&servAddr, sizeof(sockaddr_in)) < 0) {
    disconnect();
    throw std::runtime_error("Client: can not connect");
  }

  if (fcntl(m_socket, F_SETFL, fcntl(m_socket, F_GETFL) | O_NONBLOCK) < 0) {
    disconnect();
    throw std::runtime_error("Client: fcntl failed");
  }
}

void Client::disconnect() {
  if (m_socket == -1)
    return;
  close(m_socket);
  m_socket = -1;
}

void Client::defineAction(const Message::Type &messageType, const std::function<void(const Message &msg)> &action) {
  m_actions[messageType] = action;
}

void Client::send(const Message &message) {
  if (m_socket == -1)
    throw std::runtime_error("Client: not connected");
  std::vector<uint8> data = message.serialize();
  std::size_t sent = 0;
  while (sent < data.size()) {
    ssize_t n = ::send(m_socket, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
    if (n > 0) {
      sent += n;
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      pollfd p = {m_socket, POLLOUT, 0};
      ::poll(&p, 1, -1);
      continue;
    }
    disconnect();
    throw std::runtime_error("Client: send failed");
  }
}

void Client::update() {
  if (m_socket == -1)
    return;
  uint8 tmp[4096];
  while (true) {
    ssize_t n = ::recv(m_socket, tmp, sizeof(tmp), 0);
    if (n > 0) {
      m_recvBuffer.insert(m_recvBuffer.end(), tmp, tmp + n);
      continue;
    }
    if (n == 0) {
      disconnect();
      break;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      break;
    if (errno == EINTR)
      continue;
    disconnect();
    throw std::runtime_error("Client: recv failed");
  }
  std::size_t offset = 0, used;
  Message msg(0);
  while ((used = Message::deserialize(m_recvBuffer, offset, msg)) > 0) {
    offset += used;
    processAction(msg);
  }
  m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + offset);
}

void Client::processAction(const Message &msg) {
  Message::Type type = msg.type();

  auto it = m_actions.find(type);
  if (it == m_actions.end())
    return;
  (it->second)(msg);
}
