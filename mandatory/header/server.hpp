#ifndef SERVER_HPP
#define SERVER_HPP

#include "client.hpp"
#include "message.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

class Server {
public:
  Server();
  Server(const Server&) = delete;
  Server& operator = (const Server&) = delete;
  ~Server();
  void start(const std::size_t &p_port);
  void defineAction(const Message::Type &messageType, const std::function<void(long long &clientID, const Message &msg)> &action);
  void sendTo(const Message &message, long long clientID);
  void sendToArray(const Message &message, std::vector<long long> clientIDs);
  void sendToAll(const Message &message);
  void update();
private:
  void send(const Message&, Client&);
  void processAction(long long& client_id, const Message &msg);
  void accept();
  void recv();

private:
  typedef int32 Socket;
  Socket m_socket = -1;
  long long m_clientId = 0;
  std::map<Message::Type, std::function<void(long long &, const Message &)>> m_actions;
  std::map<long long, Client> m_clients;
};

#endif