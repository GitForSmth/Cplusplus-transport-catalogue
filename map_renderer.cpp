#include "map_renderer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

namespace renderer {

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                                  double max_width, double max_height, double padding)
    : padding_(padding) {
    if (points_begin == points_end) {
        return;
    }

    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (max_lon != min_lon_) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (max_lat_ != min_lat) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

// for vector<Coordinates>::iterator
template SphereProjector::SphereProjector(
    std::vector<geo::Coordinates>::iterator,
    std::vector<geo::Coordinates>::iterator,
    double, double, double);

template SphereProjector::SphereProjector(
    std::vector<geo::Coordinates>::const_iterator,
    std::vector<geo::Coordinates>::const_iterator,
    double, double, double);

MapRenderer::MapRenderer(RenderSettings settings)
    : settings_(std::move(settings)) {
}

void MapRenderer::Render(std::ostream& out, const std::map<std::string_view, const domain::Bus*>& buses) const {
    std::vector<geo::Coordinates> all_coords;
    std::map<std::string_view, const domain::Stop*> all_stops;

    for (const auto& [bus_name, bus] : buses) {
        if (bus->stops.empty()) continue;
        for (const auto* stop : bus->stops) {
            all_coords.push_back(stop->coordinates);
            all_stops[stop->name] = stop;
        }
    }

    // create projector
    SphereProjector proj(all_coords.begin(), all_coords.end(),
                         settings_.width, settings_.height, settings_.padding);

    svg::Document doc;

    RenderBusLines(doc, proj, buses);
    RenderBusLabels(doc, proj, buses);
    RenderStopCircles(doc, proj, all_stops);
    RenderStopLabels(doc, proj, all_stops);

    doc.Render(out);
}

void MapRenderer::RenderBusLines(svg::Document& doc, const SphereProjector& proj,
                                  const std::map<std::string_view, const domain::Bus*>& buses) const {
    size_t color_index = 0;
    for (const auto& [bus_name, bus] : buses) {
        if (bus->stops.empty()) continue;

        svg::Polyline line;
        line.SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()])
            .SetFillColor(svg::NoneColor)
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap("round")
            .SetStrokeLineJoin("round");

        for (const auto* stop : bus->stops) {
            line.AddPoint(proj(stop->coordinates));
        }

        if (!bus->is_roundtrip && bus->stops.size() > 1) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                line.AddPoint(proj((*it)->coordinates));
            }
        }

        doc.Add(line);
        ++color_index;
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc, const SphereProjector& proj,
                                   const std::map<std::string_view, const domain::Bus*>& buses) const {
    size_t color_index = 0;
    for (const auto& [bus_name, bus] : buses) {
        if (bus->stops.empty()) continue;

        auto render_label = [&](const domain::Stop* stop) {
            svg::Point pos = proj(stop->coordinates);

            svg::Text underlayer;
            underlayer.SetPosition(pos)
                      .SetOffset(settings_.bus_label_offset)
                      .SetFontSize(settings_.bus_label_font_size)
                      .SetFontFamily("Verdana")
                      .SetFontWeight("bold")
                      .SetData(std::string(bus_name))
                      .SetFillColor(settings_.underlayer_color)
                      .SetStrokeColor(settings_.underlayer_color)
                      .SetStrokeWidth(settings_.underlayer_width)
                      .SetStrokeLineCap("round")
                      .SetStrokeLineJoin("round");
            doc.Add(underlayer);

            svg::Text label;
            label.SetPosition(pos)
                 .SetOffset(settings_.bus_label_offset)
                 .SetFontSize(settings_.bus_label_font_size)
                 .SetFontFamily("Verdana")
                 .SetFontWeight("bold")
                 .SetData(std::string(bus_name))
                 .SetFillColor(settings_.color_palette[color_index % settings_.color_palette.size()]);
            doc.Add(label);
        };

        render_label(bus->stops.front());

        if (!bus->is_roundtrip && bus->stops.back() != bus->stops.front()) {
            render_label(bus->stops.back());
        }

        ++color_index;
    }
}

void MapRenderer::RenderStopCircles(svg::Document& doc, const SphereProjector& proj,
                                     const std::map<std::string_view, const domain::Stop*>& stops) const {
    for (const auto& [stop_name, stop] : stops) {
        svg::Circle circle;
        circle.SetCenter(proj(stop->coordinates))
              .SetRadius(settings_.stop_radius)
              .SetFillColor("white");
        doc.Add(circle);
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc, const SphereProjector& proj,
                                    const std::map<std::string_view, const domain::Stop*>& stops) const {
    for (const auto& [stop_name, stop] : stops) {
        svg::Point pos = proj(stop->coordinates);

        svg::Text underlayer;
        underlayer.SetPosition(pos)
                  .SetOffset(settings_.stop_label_offset)
                  .SetFontSize(settings_.stop_label_font_size)
                  .SetFontFamily("Verdana")
                  .SetData(std::string(stop_name))
                  .SetFillColor(settings_.underlayer_color)
                  .SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap("round")
                  .SetStrokeLineJoin("round");
        doc.Add(underlayer);

        // title
        svg::Text label;
        label.SetPosition(pos)
             .SetOffset(settings_.stop_label_offset)
             .SetFontSize(settings_.stop_label_font_size)
             .SetFontFamily("Verdana")
             .SetData(std::string(stop_name))
             .SetFillColor("black");
        doc.Add(label);
    }
}

} // namespace renderer