#pragma once

#include "svg.h"
#include "domain.h"
#include "geo.h"
#include <vector>
#include <map>
#include <algorithm>

namespace renderer {

struct RenderSettings {
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    int bus_label_font_size;
    svg::Point bus_label_offset;
    int stop_label_font_size;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    svg::Point operator()(geo::Coordinates coords) const;
private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(RenderSettings settings);
    void Render(std::ostream& out, const std::map<std::string_view, const domain::Bus*>& buses) const;

private:
    void RenderBusLines(svg::Document& doc, const SphereProjector& proj, const std::map<std::string_view, const domain::Bus*>& buses) const;
    void RenderBusLabels(svg::Document& doc, const SphereProjector& proj, const std::map<std::string_view, const domain::Bus*>& buses) const;
    void RenderStopCircles(svg::Document& doc, const SphereProjector& proj, const std::map<std::string_view, const domain::Stop*>& stops) const;
    void RenderStopLabels(svg::Document& doc, const SphereProjector& proj, const std::map<std::string_view, const domain::Stop*>& stops) const;

    RenderSettings settings_;
};

} // namespace renderer