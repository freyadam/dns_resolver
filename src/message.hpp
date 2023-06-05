#ifndef DNS_RESOLVER_MESSAGE_H
#define DNS_RESOLVER_MESSAGE_H

#include <cstddef>
#include <optional>
#include <ostream>
#include <stdint.h>
#include <string>
#include <vector>

// Enum listing (a subset of) DNS record types.
enum class RecordType {
   A = 1,
   AAAA = 28,
   NS = 2,
   CNAME = 5,
};

// Question part of the query. "What is being asked."
struct Question {
   std::string name;
   RecordType type;

   Question();
   Question(std::string name, RecordType type);
   Question(uint8_t *data, std::size_t &position);
   std::vector<uint8_t> encode();
   void print(std::ostream &os);
};

// A struct representing either an answer, authoritity, or an additional section.
struct Record {
   std::string name;
   RecordType type;
   uint32_t ttl;
   std::vector<uint8_t> data;

   Record(uint8_t *data, std::size_t &position);
   std::vector<uint8_t> encode();
   void print(std::ostream &os);
   std::string data_as_string();
};

// A complete message representing either a query or a response.
struct Message {
   uint16_t id;
   uint16_t flags;
   Question question;
   std::vector<Record> answers;
   std::vector<Record> authorities;
   std::vector<Record> additionals;

   Message(std::vector<uint8_t> bytes);
   Message(std::string name, RecordType type, uint16_t flags = 0);
   std::vector<uint8_t> encode();
   void print(std::ostream &os);

   std::optional<std::string> get_answer_data(RecordType type);
   std::optional<std::string> get_authority_data(RecordType type);
   std::optional<std::string> get_additional_data(RecordType type);

 private:
   std::optional<std::string> get_record(std::vector<Record> &v, RecordType type);
};

#endif  // DNS_RESOLVER_MESSAGE_H
