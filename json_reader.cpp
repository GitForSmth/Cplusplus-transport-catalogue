#include "json_reader.h"
#include "json_builder.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <sstream>

namespace transport {

JsonReader::JsonReader(TransportCatalogue& catalogue)
    : catalogue_(catalogue), document_{nullptr} {}

void JsonReader::Load(std::istream& input) {
    document_ = json::Load(input);
}

void JsonReader::ProcessRequests(std::ostream& output) const {
    const_cast<JsonReader*>(this)->ParseBaseRequests();

    const RequestHandler handler(catalogue_);
    
    // create router
    RoutingSettings routing_settings = ParseRoutingSettings();
    TransportRouter router(catalogue_, routing_settings);

    json::Array responses;
    const auto& stat_requests = document_.GetRoot().AsDict().at("stat_requests").AsArray();
    for (const auto& req_node : stat_requests) {
        const auto& req_data = req_node.AsDict();
        int request_id = req_data.at("id").AsInt();
        const std::string& type = req_data.at("type").AsString();

        if (type == "Bus") {
            responses.push_back(BuildBusResponse(request_id, req_data.at("name").AsString(), handler));
        } else if (type == "Stop") {
            responses.push_back(BuildStopResponse(request_id, req_data.at("name").AsString(), handler));
        } else if (type == "Map") {
            responses.push_back(BuildMapResponse(request_id, handler));
        } else if (type == "Route") {
            responses.push_back(BuildRouteResponse(
                request_id, 
                req_data.at("from").AsString(), 
                req_data.at("to").AsString(),
                router
            ));
        }
    }

    json::Print(json::Document{responses}, output);
}

void JsonReader::ParseBaseRequests() {
    const auto& base_requests = document_.GetRoot().AsDict().at("base_requests").AsArray();

    for (const auto& req : base_requests) {
        if (req.AsDict().at("type").AsString() == "Stop") {
            ParseStop(req.AsDict());
        }
    }
    
    for (const auto& req : base_requests) {
        if (req.AsDict().at("type").AsString() == "Stop") {
             const auto& stop_data = req.AsDict();
             if (stop_data.count("road_distances")) {
                 for (const auto& [neighbour, dist_node] : stop_data.at("road_distances").AsDict()) {
                     catalogue_.SetDistance(stop_data.at("name").AsString(), neighbour, dist_node.AsInt());
                 }
             }
        }
    }

    for (const auto& req : base_requests) {
        if (req.AsDict().at("type").AsString() == "Bus") {
            ParseBus(req.AsDict());
        }
    }
}

void JsonReader::ParseStop(const json::Dict& stop_data) {
    catalogue_.AddStop(stop_data.at("name").AsString(), {stop_data.at("latitude").AsDouble(), stop_data.at("longitude").AsDouble()});
}

void JsonReader::ParseBus(const json::Dict& bus_data) {
    std::vector<std::string> stops;
    stops.reserve(bus_data.at("stops").AsArray().size());
    for (const auto& stop_node : bus_data.at("stops").AsArray()) {
        stops.push_back(stop_node.AsString());
    }
    catalogue_.AddBus(bus_data.at("name").AsString(), stops, bus_data.at("is_roundtrip").AsBool());
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    const auto& settings = document_.GetRoot().AsDict().at("render_settings").AsDict();
    renderer::RenderSettings result;
    result.width = settings.at("width").AsDouble();
    result.height = settings.at("height").AsDouble();
    result.padding = settings.at("padding").AsDouble();
    result.line_width = settings.at("line_width").AsDouble();
    result.stop_radius = settings.at("stop_radius").AsDouble();
    result.bus_label_font_size = settings.at("bus_label_font_size").AsInt();
    const auto& bus_offset = settings.at("bus_label_offset").AsArray();
    result.bus_label_offset = {bus_offset.at(0).AsDouble(), bus_offset.at(1).AsDouble()};
    result.stop_label_font_size = settings.at("stop_label_font_size").AsInt();
    const auto& stop_offset = settings.at("stop_label_offset").AsArray();
    result.stop_label_offset = {stop_offset.at(0).AsDouble(), stop_offset.at(1).AsDouble()};
    result.underlayer_color = ParseColor(settings.at("underlayer_color"));
    result.underlayer_width = settings.at("underlayer_width").AsDouble();
    for (const auto& color_node : settings.at("color_palette").AsArray()) {
        result.color_palette.push_back(ParseColor(color_node));
    }
    return result;
}

RoutingSettings JsonReader::ParseRoutingSettings() const {
    const auto& root = document_.GetRoot().AsDict();
    if (root.count("routing_settings") == 0) {
        return RoutingSettings{};
    }
    const auto& settings = root.at("routing_settings").AsDict();
    RoutingSettings result;
    result.bus_wait_time = settings.at("bus_wait_time").AsInt();
    result.bus_velocity = settings.at("bus_velocity").AsDouble();
    return result;
}

svg::Color JsonReader::ParseColor(const json::Node& node) const {
    if (node.IsString()) return node.AsString();
    const auto& arr = node.AsArray();
    uint8_t r = arr.at(0).AsInt();
    uint8_t g = arr.at(1).AsInt();
    uint8_t b = arr.at(2).AsInt();
    if (arr.size() == 3) return svg::Rgb{r, g, b};
    return svg::Rgba{r, g, b, arr.at(3).AsDouble()};
}

json::Node JsonReader::BuildBusResponse(int request_id, const std::string& bus_name, const RequestHandler& handler) const {
    json::Builder builder;
    builder.StartDict()
        .Key("request_id").Value(request_id);

    auto bus_info = handler.GetBusStat(bus_name);
    if (!bus_info) {
        builder.Key("error_message").Value(std::string("not found"));
    } else {
        builder.Key("curvature").Value(bus_info->curvature)
               .Key("route_length").Value(bus_info->route_length)
               .Key("stop_count").Value(static_cast<int>(bus_info->stop_count))
               .Key("unique_stop_count").Value(static_cast<int>(bus_info->unique_stop_count));
    }

    return builder.EndDict().Build();
}

json::Node JsonReader::BuildStopResponse(int request_id, const std::string& stop_name, const RequestHandler& handler) const {
    json::Builder builder;
    builder.StartDict()
           .Key("request_id").Value(request_id);

    const auto* buses_ptr = handler.GetBusesByStop(stop_name);
    if (!buses_ptr) {
        builder.Key("error_message").Value(std::string("not found"));
    } else {
        builder.Key("buses").StartArray();
        for (const auto& bus_name : *buses_ptr) {
            builder.Value(std::string(bus_name));
        }
        builder.EndArray();
    }
    
    return builder.EndDict().Build();
}

json::Node JsonReader::BuildMapResponse(int request_id, const RequestHandler& handler) const {
    renderer::MapRenderer renderer(ParseRenderSettings());
    std::ostringstream strm;
    renderer.Render(strm, handler.GetSortedBuses());

    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(request_id)
            .Key("map").Value(strm.str())
        .EndDict()
        .Build();
}

json::Node JsonReader::BuildRouteResponse(int request_id, const std::string& from, const std::string& to,
                                           const TransportRouter& router) const {
    auto route_info = router.BuildRoute(from, to);
    
    if (!route_info) {
        return json::Builder{}
            .StartDict()
                .Key("request_id").Value(request_id)
                .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }
    
    json::Builder builder;
    builder.StartDict()
           .Key("request_id").Value(request_id)
           .Key("total_time").Value(route_info->total_time)
           .Key("items").StartArray();
    
    for (const auto& item : route_info->items) {
        std::visit([&builder](const auto& route_item) {
            builder.StartDict();
            
            if constexpr (std::is_same_v<std::decay_t<decltype(route_item)>, WaitRouteItem>) {
                builder.Key("type").Value(std::string("Wait"))
                       .Key("stop_name").Value(std::string(route_item.stop_name))
                       .Key("time").Value(route_item.time);
            } else {
                builder.Key("type").Value(std::string("Bus"))
                       .Key("bus").Value(std::string(route_item.bus_name))
                       .Key("span_count").Value(route_item.span_count)
                       .Key("time").Value(route_item.time);
            }
            
            builder.EndDict();
        }, item);
    }
    
    builder.EndArray();
    
    return builder.EndDict().Build();
}

} // namespace transport