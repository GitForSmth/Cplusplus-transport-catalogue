#include "json_reader.h"
#include "transport_catalogue.h"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);

        transport::TransportCatalogue catalogue;

        transport::JsonReader json_reader(catalogue);

        json_reader.Load(std::cin);

        json_reader.ProcessRequests(std::cout);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}