#ifndef DNS_RESOLVER_RESOLVER_H
#define DNS_RESOLVER_RESOLVER_H

#include <optional>
#include <string>

class Resolver {
    public:
        std::optional<std::string> resolve(std::string hostname);
};


#endif // DNS_RESOLVER_RESOLVER_H
