#ifndef DNS_RESOLVER_RESOLVER_H
#define DNS_RESOLVER_RESOLVER_H

#include <optional>
#include <string>

#include "message.hpp"

static const uint16_t DNS_PORT = 53;

class Resolver {
 public:
   Resolver();
   Resolver(std::string ns_addr);
   std::optional<Message> resolve(Message query);
   std::optional<Message> resolve(std::string hostname, RecordType type);

 private:
   static const uint32_t INCOMING_MSG_BUF_SIZE = 4096;
   std::string root_ns_addr;
   int get_connected_socket(std::string ns_addr);
   Message get_response(Message query, std::string ns_addr);
};

#endif  // DNS_RESOLVER_RESOLVER_H
