#include <iostream>

#include "message.hpp"
#include "resolver.hpp"

int main(int argc, char **argv) {
   // Get hostname from the list of arguments.
   if (argc < 2) {
      std::cerr << "Hostname not provided" << std::endl;
      return EXIT_FAILURE;
   }
   std::string hostname(argv[1]);

   // Resolve the hostname.
   Resolver r = Resolver();
   std::optional<Message> response = r.resolve(hostname, RecordType::A);

   // Pretty-print the result if possible.
   if (response.has_value()) {
      response.value().print(std::cout);
   } else {
      std::cout << "No result" << std::endl;
   }

   return EXIT_SUCCESS;
}
