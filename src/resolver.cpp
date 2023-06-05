#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.hpp"
#include "resolver.hpp"

Resolver::Resolver() : root_ns_addr(std::string("199.7.83.42")) {}

Resolver::Resolver(std::string ns_addr) : root_ns_addr(ns_addr) {}

int Resolver::get_connected_socket(std::string ns_addr) {
   struct sockaddr_in sa;
   struct addrinfo resolver;

   // Initialize the destination resolver details.
   sa.sin_family = AF_INET;
   sa.sin_port = htons(DNS_PORT);
   if (inet_pton(AF_INET, ns_addr.data(), &(sa.sin_addr)) == -1) {
      throw std::runtime_error("Failed to translate the server address.");
   }
   if (memset(sa.sin_zero, 0, 8) == nullptr) {
      throw std::runtime_error("Failed to zero out `sin_zero`.");
   }

   // Fill in connection details.
   resolver.ai_flags = 0;
   resolver.ai_family = AF_INET;  // IPv4
   resolver.ai_socktype = SOCK_DGRAM;
   resolver.ai_protocol = IPPROTO_UDP;  // UDP connection
   resolver.ai_addrlen = sizeof(sa);
   resolver.ai_addr = reinterpret_cast<sockaddr *>(&sa);
   resolver.ai_canonname = nullptr;
   resolver.ai_next = nullptr;

   // Get a socket which will be re-used for all future resolutions.
   int socket_fd = socket(resolver.ai_family, resolver.ai_socktype, resolver.ai_protocol);
   if (socket_fd == -1) {
      std::ostringstream ss;
      ss << "Failed to get a valid socket (" << strerror(errno) << ")";
      throw std::runtime_error(ss.str());
   }

   // Create a connection.
   if (connect(socket_fd, resolver.ai_addr, resolver.ai_addrlen) != 0) {
      std::ostringstream ss;
      ss << "Failed to connect to the server (" << strerror(errno) << ")";
      throw std::runtime_error(ss.str());
   }

   return socket_fd;
}

Message Resolver::get_response(Message query, std::string ns_addr) {
   int socket_fd = this->get_connected_socket(ns_addr);

   // Send the encoded message.
   int send_len = 0;
   int send_status;
   auto v = query.encode();
   do {
      send_status = send(socket_fd, v.data() + send_len, v.size() - send_len, 0);
      if (send_status == -1) {
         std::ostringstream ss;
         ss << "Failed `send` (" << strerror(errno) << ")";
         throw std::runtime_error(ss.str());
      } else {
         send_len += send_status;
      }
   } while (send_len < v.size());

   // Receive a response.
   int recv_status;
   uint8_t recv_buf[Resolver::INCOMING_MSG_BUF_SIZE];
   std::vector<uint8_t> byte_vector;
   do {
      recv_status = recv(socket_fd, recv_buf, Resolver::INCOMING_MSG_BUF_SIZE, 0);

      if (recv_status == -1) {
         std::ostringstream ss;
         ss << "Failed `recv` (" << strerror(errno) << ")";
         throw std::runtime_error(ss.str());
      } else {
         for (int k = 0; k < recv_status; ++k) {
            byte_vector.push_back(recv_buf[k]);
         }
      }
   } while (recv_status == Resolver::INCOMING_MSG_BUF_SIZE);

   close(socket_fd);

   return Message(byte_vector);
}

std::optional<Message> Resolver::resolve(Message query) {
   std::optional<std::string> d;
   std::string ns_addr = this->root_ns_addr;
   for (;;) {
      Message response = this->get_response(query, ns_addr);

      if ((d = response.get_answer_data(RecordType::A))) {
         // If possible, return the answer immediately.
         return response;
      } else if ((d = response.get_additional_data(RecordType::A))) {
         // Otherwise get the glue record for the next query;
         ns_addr = d.value();
      } else if ((d = response.get_authority_data(RecordType::NS))) {
         // Otherwise try get an IP address of the next possible authority.
         std::string ns_hostname = d.value();
         std::optional<Message> m = this->resolve(Message(ns_hostname, RecordType::A));
         if (!m) {
            return {};
         }

         std::optional<std::string> s = m.value().get_answer_data(RecordType::A);
         if (!s) {
            return {};
         }

         ns_addr = s.value();
      } else {
         return {};
      }
   }
}

std::optional<Message> Resolver::resolve(std::string hostname, RecordType type) {
   // Create the query DNS message to be sent.
   Message query(hostname, type);

   // Decode a DNS message from the response and return it.
   return this->resolve(query);
}
