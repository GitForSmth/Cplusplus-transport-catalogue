#include "svg.h"

namespace svg {

std::ostream& operator<<(std::ostream& out, const Color& color) {
    std::visit(
        [&out](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                out << "none";
            } else if constexpr (std::is_same_v<T, std::string>) {
                out << value;
            } else if constexpr (std::is_same_v<T, Rgb>) {
                out << "rgb(" << static_cast<int>(value.r) << "," << static_cast<int>(value.g) << "," << static_cast<int>(value.b) << ")";
            } else if constexpr (std::is_same_v<T, Rgba>) {
                out << "rgba(" << static_cast<int>(value.r) << "," << static_cast<int>(value.g) << "," << static_cast<int>(value.b) << "," << value.opacity << ")";
            }
        },
        color);
    return out;
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}
Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}
void Circle::Render(std::ostream& out) const {
    out << "<circle cx=\"" << center_.x << "\" cy=\"" << center_.y << "\" ";
    out << "r=\"" << radius_ << "\"";
    RenderAttrs(out);
    out << "/>";
}

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}
void Polyline::Render(std::ostream& out) const {
    out << "<polyline points=\"";
    bool first = true;
    for (const auto& point : points_) {
        if (!first) {
            out << " ";
        }
        out << point.x << "," << point.y;
        first = false;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>";
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}
Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}
Text& Text::SetFontFamily(const std::string& font_family) {
    font_family_ = font_family;
    return *this;
}
Text& Text::SetFontWeight(const std::string& font_weight) {
    font_weight_ = font_weight;
    return *this;
}
Text& Text::SetData(const std::string& data) {
    data_ = data;
    return *this;
}
void Text::Render(std::ostream& out) const {
    out << "<text";
    RenderAttrs(out);
    out << " x=\"" << pos_.x << "\" y=\"" << pos_.y << "\" ";
    out << "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" ";
    out << "font-size=\"" << font_size_ << "\"";
    if (font_family_) {
        out << " font-family=\"" << *font_family_ << "\"";
    }
    if (font_weight_) {
        out << " font-weight=\"" << *font_weight_ << "\"";
    }
    out << ">" << data_ << "</text>";
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    for (const auto& obj : objects_) {
        out << "  ";
        obj->Render(out);
        out << "\n";
    }
    out << "</svg>";
}

} // namespace svg
