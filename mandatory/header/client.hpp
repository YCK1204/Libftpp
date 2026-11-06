#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "message.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class Client {
public:
  Client();
  Client(const Client &other) = delete;
  Client &operator=(const Client &other) = delete;
  ~Client();
  void connect(const std::string &address, const std::size_t &port);
  void disconnect();
  void defineAction(const Message::Type &messageType, const std::function<void(const Message &msg)> &action);
  void send(const Message &message);
  void update();

private:
  void processAction(const Message &msg);

private:
  std::vector<uint8> m_recvBuffer;
  std::map<Message::Type, std::function<void(const Message& msg)>> m_actions;
  typedef int32 Socket;
  Socket m_socket = -1;
private:
  friend class Server;
};

#endif