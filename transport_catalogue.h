#pragma once

#include "domain.h"
#include "geo.h"

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <set>
#include <optional>
#include <functional>
#include <map>

namespace transport {

struct BusInfo {
    int stop_count = 0;
    int unique_stop_count = 0;
    double route_length = 0.0;
    double curvature = 0.0;
};

struct PairHasher {
    size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const {
        std::hash<const void*> hasher;
        return hasher(p.first) ^ (hasher(p.second) << 1);
    }
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, const geo::Coordinates& coordinates);
    void AddBus(const std::string& name, const std::vector<std::string>& stop_names, bool is_roundtrip);

    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus* FindBus(std::string_view name) const;

    void SetDistance(std::string_view from_stop, std::string_view to_stop, int distance);
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>* GetBusesForStop(std::string_view stop_name) const;
    
    std::map<std::string_view, const domain::Bus*> GetSortedBuses() const;
    
    std::vector<const domain::Stop*> GetAllStops() const;
    std::vector<const domain::Bus*> GetAllBuses() const;

private:
    std::deque<domain::Stop> stops_;
    std::unordered_map<std::string_view, domain::Stop*> stop_index_;

    std::deque<domain::Bus> buses_;
    std::unordered_map<std::string_view, const domain::Bus*> bus_index_;

    std::unordered_map<const domain::Stop*, std::set<std::string_view>> stop_to_buses_;
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, PairHasher> distances_;
};

} // namespace transport