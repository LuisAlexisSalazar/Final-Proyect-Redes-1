#ifndef UDP_SOCKET_HPP_
#define UDP_SOCKET_HPP_
// networking headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// C headers
#include <cstring>
#include <unistd.h>
#include <errno.h>

// C++ stl headers
#include <sstream>
#include <iostream>


namespace net
{
  const int MAX_DGRAM_SIZE = 1000;
  
  class UdpSocket
  {
  private:
    int socket_file_descriptor;
    struct addrinfo hints, *servinfo, *configured_sockaddr;
    struct sockaddr_storage sender_addr;
    socklen_t sender_addr_len;

    std::string IP;
    std::string Port;

  public:
    UdpSocket();
    UdpSocket(const UdpSocket& another_socket);
    UdpSocket(const std::string &_IP, const std::string &_Port);
    void configureSocket(const std::string &_IP, const std::string &_Port);

    int sendAll(u_char *buffer, int &bytes_sent, bool to_sender);
    int simpleRecv(u_char *buffer);

    bool active();
    int getSocketFileDescriptor() const;
    std::string getIP() const;
    uint16_t getPort() const;

    std::string getSenderIP();
    uint16_t getSenderPort();
    
    
    void *get_in_addr(struct sockaddr *sa);
    uint16_t get_in_port(struct sockaddr *sa);
    ~UdpSocket();
  };

}
#endif //UDP_SOCKET_HPP_