#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../storage/store.h"
#include <cstdint>

template <typename K, typename V>
class Server{
private:
  int server_fd;
  int client_fd;
  int port;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t addr_len;
  Storage <K, V> store;

public:
  Server(int port) : port(port), addr_len(sizeof(server_address)){
    server_fd = -1;
    client_fd = -1;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
    store = Storage();
  }

  ~Server() {
    if (server_fd != -1) {
      close(server_fd);
    }
  }

  bool initialize(){
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0){
      std::cerr << "Server socket not initialized" << std::endl;
      return false;
    }

    if(bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
      std::cerr << "Error binding socket" << std::endl;
      return false;

    }

    if (listen(server_fd, 5) < 0) {
      std::cerr << "Error listening on socket" << std::endl;
      return false;
    }

    std::cout << "Server listening on port " << port << std::endl;
    return true;

  }

  bool accept_client(){
    client_fd = accept(server_fd, (struct sockaddr*)&client_address, &addr_len);
    if(client_fd < 0){
      std::cerr << "Error accepting client connection" << std::endl;
      return false;
    }

    std::cout << "Client connected" << std::endl;
    return true;

  }


  bool send_response(std::vector<char> req){
    return true;

  }

  std::vector<uint8_t> receive_request(std::size_t &out_len) {
    if (client_fd < 0) {
      std::cerr << "No client connected" << std::endl;
      out_len = 0;
      return {};
    }

    std::vector<uint8_t> buffer;
    ssize_t num_bytes_received = recv(client_fd, buffer.data(), buffer.size(), 0);
    if (num_bytes_received < 0) {
      std::cerr << "Error receiving request" << std::endl;
      out_len = 0;
      return {};
    }
    if (num_bytes_received == 0) {
      std::cerr << "Client disconnected" << std::endl;
      out_len = 0;
      return {};
    }

    return buffer;
  }


};