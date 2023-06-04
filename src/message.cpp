#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "message.hpp"

template <typename T> T bytes_to_uint(uint8_t *data) {
   T k = 0;

   for (int i = 0; i < sizeof(T); ++i) {
      k <<= 8;
      k += data[i];
   }

   return k;
}

template <typename T> std::vector<uint8_t> uint_to_bytes(T k, bool big_endian = true) {
   std::vector<uint8_t> v;

   for (int i = 0; i < sizeof(T); ++i) {
      v.push_back(k % (1 << 8));
      k >>= 8;
   }

   if (!big_endian) {
      std::reverse(v.begin(), v.end());
   }

   return v;
}

std::string read_name(uint8_t *data, std::size_t &pos) {
   std::string name;
   bool is_ptr;

   while (data[pos] != 0) {

      // Individual labels are separated with a dot character.
      if (!name.empty()) {
         name += std::string(".");
      }

      is_ptr = data[pos] & 0b11000000;
      if (is_ptr) {
         // The remaining section of the name is at a different address.
         std::size_t ptr = data[pos] & 0b00111111;
         ptr <<= 8;
         ptr += data[pos + 1];
         name += read_name(data, ptr);
         pos += 1;

         break;
      } else {
         // The next label is a literal.
         std::size_t label_len = data[pos];
         pos += 1;

         for (int k = 0; k < label_len; ++k) {
            name.push_back(static_cast<char>(data[pos + k]));
         }
         pos += label_len;
      }
   }
   ++pos;

   return name;
}

std::vector<uint8_t> write_name(std::string &s) {
   std::vector<uint8_t> v;
   std::size_t idx, prev_idx = 0;

   // Encode all but the last label.
   while ((idx = s.find(".", prev_idx)) != std::string::npos) {
      std::string ss = s.substr(prev_idx, idx - prev_idx);
      v.push_back(ss.size());
      for (char c : ss) {
         v.push_back(static_cast<uint8_t>(c));
      }
      prev_idx = idx + 1;
   }

   // Encode the last label.
   std::string ss = s.substr(prev_idx);
   v.push_back(ss.size());
   for (char c : ss) {
      v.push_back(static_cast<uint8_t>(c));
   }

   // Terminate with a zero.
   v.push_back(0);

   return v;
}

Question::Question() {}

Question::Question(std::string name, RecordType type) : name(name), type(type) {}

Question::Question(uint8_t *data, std::size_t &position) {
   this->name = read_name(data, position);

   this->type = static_cast<RecordType>(bytes_to_uint<uint16_t>(data + position));
   position += 2;

   uint16_t class_ = bytes_to_uint<uint16_t>(data + position);
   position += 2;

   if (class_ != 1) {
      std::ostringstream ss;
      ss << "Question class not IN, class id: " << class_;
      throw std::runtime_error(ss.str());
   }
}

std::vector<uint8_t> Question::encode() {
   std::vector<uint8_t> v = write_name(this->name);

   uint16_t type_val = static_cast<uint16_t>(this->type);
   std::vector<uint8_t> to_append = uint_to_bytes(type_val, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t class_val = 1;  // always IN
   to_append = uint_to_bytes(class_val, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   return v;
}

void Question::print(std::ostream &os) {
   // TODO use appropriate labels rather than integers
   os << std::left << std::setw(40) << this->name << std::setw(5)
      << static_cast<uint16_t>(this->type) << std::setw(5) << 1 << std::endl;
}

Record::Record(uint8_t *data, std::size_t &position) {
   this->name = read_name(data, position);

   this->type = static_cast<RecordType>(bytes_to_uint<uint16_t>(data + position));
   position += 2;

   uint16_t class_ = bytes_to_uint<uint16_t>(data + position);
   position += 2;

   this->ttl = bytes_to_uint<uint32_t>(data + position);
   position += 4;

   std::size_t rdlength = static_cast<std::size_t>(bytes_to_uint<uint16_t>(data + position));
   position += 2;

   for (int k = 0; k < rdlength; ++k) {
      this->data.push_back(data[position++]);
   }

   if (class_ != 1) {
      std::ostringstream ss;
      ss << "Question class not IN, class id: " << class_;
      throw std::runtime_error(ss.str());
   }
}

std::vector<uint8_t> Record::encode() {
   std::vector<uint8_t> v = write_name(this->name);

   uint16_t type_val = static_cast<uint16_t>(this->type);
   std::vector<uint8_t> to_append = uint_to_bytes(type_val);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t class_val = 1;  // always IN
   to_append = uint_to_bytes(class_val);
   v.insert(v.end(), to_append.begin(), to_append.end());

   to_append = uint_to_bytes(this->ttl);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t rdlength = static_cast<uint16_t>(this->data.size());
   to_append = uint_to_bytes(rdlength);
   v.insert(v.end(), to_append.begin(), to_append.end());

   v.insert(v.end(), this->data.begin(), this->data.end());

   return v;
}

void Record::print(std::ostream &os) {
   // TODO use appropriate labels rather than integers
   os << std::left << std::setw(40) << this->name << std::setw(5)
      << static_cast<uint16_t>(this->type) << std::setw(6) << 1 << std::setw(5) << this->ttl;

   os << " [";

   if (this->type == RecordType::A) {
      os << std::to_string(this->data[0]) << "." << std::to_string(this->data[1]) << "."
         << std::to_string(this->data[2]) << "." << std::to_string(this->data[3]);
   } else {
      os << "???";
   }

   os << "]" << std::endl;
}

Message::Message(std::vector<uint8_t> bytes) {
   this->id = bytes_to_uint<uint16_t>(bytes.data());
   this->flags = bytes_to_uint<uint16_t>(bytes.data() + 2);

   uint16_t qdcount = bytes_to_uint<uint16_t>(bytes.data() + 4);
   uint16_t ancount = bytes_to_uint<uint16_t>(bytes.data() + 6);
   uint16_t nscount = bytes_to_uint<uint16_t>(bytes.data() + 8);
   uint16_t arcount = bytes_to_uint<uint16_t>(bytes.data() + 10);

   std::size_t position = 12;

   assert(qdcount == 1);
   this->question = Question(bytes.data(), position);

   for (int k = 0; k < ancount; ++k) {
      this->answers.push_back(Record(bytes.data(), position));
   }

   for (int k = 0; k < nscount; ++k) {
      this->authorities.push_back(Record(bytes.data(), position));
   }

   for (int k = 0; k < arcount; ++k) {
      this->additionals.push_back(Record(bytes.data(), position));
   }
};

Message::Message(std::string name, RecordType type, uint16_t flags) {
   std::srand(std::time(0));
   this->id = std::rand();
   this->flags = flags;
   this->question = Question(name, type);
};

std::vector<uint8_t> Message::encode() {
   std::vector<uint8_t> v;

   std::vector<uint8_t> to_append = uint_to_bytes(this->id);
   v.insert(v.end(), to_append.begin(), to_append.end());

   to_append = uint_to_bytes(this->flags);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t qdcount = 1;
   to_append = uint_to_bytes(qdcount, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t ancount = this->answers.size();
   to_append = uint_to_bytes(ancount, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t nscount = this->authorities.size();
   to_append = uint_to_bytes(nscount, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   uint16_t arcount = this->additionals.size();
   to_append = uint_to_bytes(arcount, false);
   v.insert(v.end(), to_append.begin(), to_append.end());

   to_append = this->question.encode();
   v.insert(v.end(), to_append.begin(), to_append.end());

   for (uint16_t k = 0; k < ancount; ++k) {
      to_append = this->answers[k].encode();
      v.insert(v.end(), to_append.begin(), to_append.end());
   }

   for (uint16_t k = 0; k < nscount; ++k) {
      to_append = this->authorities[k].encode();
      v.insert(v.end(), to_append.begin(), to_append.end());
   }

   for (uint16_t k = 0; k < arcount; ++k) {
      to_append = this->additionals[k].encode();
      v.insert(v.end(), to_append.begin(), to_append.end());
   }

   return v;
}

void Message::print(std::ostream &os) {
   os << "Query id: " << this->id << std::endl;

   os << "--- Questions ---" << std::endl;
   os << std::left << std::setw(40) << "hostname" << std::setw(5) << "type" << std::setw(5)
      << "class" << std::endl;
   this->question.print(os);

   if (!this->answers.empty()) {
      os << "--- Answers ---" << std::endl;
      os << std::left << std::setw(40) << "hostname" << std::setw(5) << "type" << std::setw(6)
         << "class" << std::setw(5) << "ttl" << std::endl;
      for (auto record : this->answers) {
         record.print(os);
      }
   }

   if (!this->authorities.empty()) {
      os << "--- Authorities ---" << std::endl;
      os << std::left << std::setw(40) << "hostname" << std::setw(5) << "type" << std::setw(6)
         << "class" << std::setw(5) << "ttl" << std::endl;
      for (auto record : this->authorities) {
         record.print(os);
      }
   }

   if (!this->additionals.empty()) {
      os << "--- Additionals ---" << std::endl;
      os << std::left << std::setw(40) << "hostname" << std::setw(5) << "type" << std::setw(6)
         << "class" << std::setw(5) << "ttl" << std::endl;
      for (auto record : this->additionals) {
         record.print(os);
      }
   }
}
