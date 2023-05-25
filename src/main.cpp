#include <iostream>
#include "resolver.hpp"

int main(int argc, char ** argv) {
    Resolver r = Resolver();
    std::cout << r.resolve("hostname_placeholder").value_or("No result") << std::endl;
    return 1;
}
