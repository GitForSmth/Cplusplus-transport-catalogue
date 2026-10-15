#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace transport {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

struct WaitRouteItem {
    std::string_view stop_name;
    double time = 0.0;
};

struct BusRouteItem {
    std::string_view bus_name;
    double time = 0.0;
    int span_count = 0;
};

// variant
using RouteItem = std::variant<WaitRouteItem, BusRouteItem>;

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    TransportRouter(const TransportCatalogue& catalogue, RoutingSettings settings);

    std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

private:
    struct EdgeInfo {
        std::string_view name; // name (Wait) & (Bus)
        int span_count = 0;
        double time = 0.0;
        bool is_wait = false;
    };

    void BuildGraph();
    void AddBusEdges(const domain::Bus* bus);

    const TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    
    std::unordered_map<std::string_view, size_t> stop_to_vertex_;
    
    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    
    std::vector<EdgeInfo> edges_info_;
};

} // namespace transport