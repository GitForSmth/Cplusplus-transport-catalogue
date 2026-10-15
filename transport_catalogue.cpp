#include "transport_catalogue.h"
#include <unordered_set>
#include <numeric>
#include <map>

namespace transport {

void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates& coordinates) {
    stops_.push_back({name, coordinates});
    stop_index_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string>& stop_names, bool is_roundtrip) {
    std::vector<const domain::Stop*> stops;
    stops.reserve(stop_names.size());
    for (const auto& stop_name : stop_names) {
        if (auto* stop_ptr = FindStop(stop_name)) {
            stops.push_back(stop_ptr);
        }
    }
    buses_.emplace_back(name, std::move(stops), is_roundtrip);
    const domain::Bus& new_bus = buses_.back();
    bus_index_[new_bus.name] = &new_bus;
    for (const auto* stop : new_bus.stops) {
        stop_to_buses_[stop].insert(new_bus.name);
    }
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    auto it = stop_index_.find(name);
    if (it != stop_index_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    auto it = bus_index_.find(name);
    if (it != bus_index_.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::SetDistance(std::string_view from_stop, std::string_view to_stop, int distance) {
    const domain::Stop* from = FindStop(from_stop);
    const domain::Stop* to = FindStop(to_stop);
    if (from && to) {
        distances_[{from, to}] = distance;
    }
}

int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return 0;
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus) {
        return std::nullopt;
    }

    const auto& stops = bus->stops;
    if (stops.empty()) {
        return BusInfo{0, 0, 0.0, 0.0};
    }
    
    double geo_length = 0.0;
    int road_length = 0;
    
    for (size_t i = 0; i < stops.size() - 1; ++i) {
        road_length += GetDistance(stops[i], stops[i+1]);
        geo_length += geo::ComputeDistance(stops[i]->coordinates, stops[i+1]->coordinates);
    }
    
    int stop_count = stops.size();
    if (!bus->is_roundtrip) {
        stop_count = stop_count * 2 - 1;
        for (size_t i = stops.size() - 1; i > 0; --i) {
            road_length += GetDistance(stops[i], stops[i-1]);
            geo_length += geo::ComputeDistance(stops[i]->coordinates, stops[i-1]->coordinates);
        }
    }

    std::unordered_set<const domain::Stop*> unique_stops(stops.begin(), stops.end());

    return BusInfo{
        stop_count,
        static_cast<int>(unique_stops.size()),
        static_cast<double>(road_length),
        (geo_length > 0) ? (static_cast<double>(road_length) / geo_length) : 0.0
    };
}

const std::set<std::string_view>* TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    const domain::Stop* stop = FindStop(stop_name);
    if (!stop) {
        return nullptr;
    }
    
    auto it = stop_to_buses_.find(stop);
    if (it != stop_to_buses_.end()) {
        return &it->second;
    }
    
    static const std::set<std::string_view> empty_set;
    return &empty_set;
}

std::map<std::string_view, const domain::Bus*> TransportCatalogue::GetSortedBuses() const {
    return {bus_index_.begin(), bus_index_.end()};
}

std::vector<const domain::Stop*> TransportCatalogue::GetAllStops() const {
    std::vector<const domain::Stop*> result;
    result.reserve(stops_.size());
    for (const auto& stop : stops_) {
        result.push_back(&stop);
    }
    return result;
}

std::vector<const domain::Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const domain::Bus*> result;
    result.reserve(buses_.size());
    for (const auto& bus : buses_) {
        result.push_back(&bus);
    }
    return result;
}
} // namespace transport