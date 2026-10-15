#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <iostream>
#include <memory>

namespace transport {

class JsonReader {
public:
    JsonReader(TransportCatalogue& catalogue);
    void Load(std::istream& input);
    void ProcessRequests(std::ostream& output) const;

private:
    void ParseBaseRequests();
    void ParseStop(const json::Dict& stop_data);
    void ParseBus(const json::Dict& bus_data);
    
    renderer::RenderSettings ParseRenderSettings() const;
    RoutingSettings ParseRoutingSettings() const;
    svg::Color ParseColor(const json::Node& color_node) const;

    json::Node BuildBusResponse(int request_id, const std::string& bus_name, const RequestHandler& handler) const;
    json::Node BuildStopResponse(int request_id, const std::string& stop_name, const RequestHandler& handler) const;
    json::Node BuildMapResponse(int request_id, const RequestHandler& handler) const;
    json::Node BuildRouteResponse(int request_id, const std::string& from, const std::string& to, 
                                   const TransportRouter& router) const;

    TransportCatalogue& catalogue_;
    json::Document document_{nullptr};
};

} // namespace transport