#include "transport_router.h"

namespace transport {

static constexpr double KMH_TO_MMIN = 1000.0 / 60.0;

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, RoutingSettings settings)
    : catalogue_(catalogue)
    , settings_(settings) {
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    const auto all_stops = catalogue_.GetAllStops();
    
    // numbering busstops
    size_t vertex_id = 0;
    for (const auto* stop : all_stops) {
        stop_to_vertex_[stop->name] = vertex_id++;
    }
    
    const size_t vertex_count = vertex_id * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);

    for (const auto* stop : all_stops) {
        size_t idx = stop_to_vertex_.at(stop->name);
        
        graph::Edge<double> wait_edge{
            idx * 2,
            idx * 2 + 1,
            static_cast<double>(settings_.bus_wait_time)
        };
        
        size_t edge_id = graph_->AddEdge(wait_edge);
        
        // check edge_id == edges_info_
        while (edges_info_.size() <= edge_id) {
            edges_info_.push_back({});
        }
        
        edges_info_[edge_id].is_wait = true;
        edges_info_[edge_id].name = stop->name;
        edges_info_[edge_id].time = static_cast<double>(settings_.bus_wait_time);
    }

    const auto all_buses = catalogue_.GetAllBuses();
    for (const auto* bus : all_buses) {
        AddBusEdges(bus);
    }
    
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

void TransportRouter::AddBusEdges(const domain::Bus* bus) {
    const auto& stops = bus->stops;
    const size_t stop_count = stops.size();
    
    if (stop_count < 2) {
        return;
    }
    
    auto add_edges_for_direction = [&](const std::vector<const domain::Stop*>& route_stops) {
        const size_t count = route_stops.size();
        for (size_t i = 0; i < count; ++i) {
            double accumulated_time = 0.0;
            
            for (size_t j = i + 1; j < count; ++j) {
                int distance = catalogue_.GetDistance(route_stops[j - 1], route_stops[j]);
                double time = distance / (settings_.bus_velocity * KMH_TO_MMIN);
                accumulated_time += time;
                
                size_t from_vertex = stop_to_vertex_.at(route_stops[i]->name) * 2 + 1;
                size_t to_vertex = stop_to_vertex_.at(route_stops[j]->name) * 2;
                
                size_t edge_id = graph_->AddEdge({from_vertex, to_vertex, accumulated_time});
                
                while (edges_info_.size() <= edge_id) {
                    edges_info_.push_back({});
                }
                
                edges_info_[edge_id].is_wait = false;
                edges_info_[edge_id].name = bus->name;
                edges_info_[edge_id].span_count = static_cast<int>(j - i);
                edges_info_[edge_id].time = accumulated_time;
            }
        }
    };
    
    // direct
    add_edges_for_direction(stops);
    
    // back direct
    if (!bus->is_roundtrip) {
        std::vector<const domain::Stop*> reverse_stops(stops.rbegin(), stops.rend());
        add_edges_for_direction(reverse_stops);
    }
}

std::optional<RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    auto from_it = stop_to_vertex_.find(from);
    auto to_it = stop_to_vertex_.find(to);
    
    if (from_it == stop_to_vertex_.end() || to_it == stop_to_vertex_.end()) {
        return std::nullopt;
    }
    
    if (from == to) {
        return RouteInfo{0.0, {}};
    }
    
    size_t from_vertex = from_it->second * 2;
    size_t to_vertex = to_it->second * 2;
    
    auto route = router_->BuildRoute(from_vertex, to_vertex);
    if (!route) {
        return std::nullopt;
    }
    
    RouteInfo result;
    result.total_time = route->weight;
    
    for (const auto edge_id : route->edges) {
        const auto& edge_info = edges_info_[edge_id];
        
        if (edge_info.is_wait) {
            result.items.push_back(WaitRouteItem{
                edge_info.name,
                edge_info.time
            });
        } else {
            result.items.push_back(BusRouteItem{
                edge_info.name,
                edge_info.time,
                edge_info.span_count
            });
        }
    }
    
    return result;
}

} // namespace transport