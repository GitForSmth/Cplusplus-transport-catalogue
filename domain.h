#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    Bus(std::string name, std::vector<const Stop*> stops, bool is_roundtrip)
        : name(std::move(name)), stops(std::move(stops)), is_roundtrip(is_roundtrip) {}

    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

} // namespace domain