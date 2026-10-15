#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <cstdint>

namespace svg {


struct Rgb {
    Rgb() = default;
    Rgb(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    uint8_t r = 0, g = 0, b = 0;
};

struct Rgba {
    Rgba() = default;
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity) : r(red), g(green), b(blue), opacity(opacity) {}
    uint8_t r = 0, g = 0, b = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor{"none"};

std::ostream& operator<<(std::ostream& out, const Color& color);

struct Point {
    double x = 0, y = 0;
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(const std::string& line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(const std::string& line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }

protected:
    void RenderAttrs(std::ostream& out) const {
        if (fill_color_) out << " fill=\"" << *fill_color_ << "\"";
        if (stroke_color_) out << " stroke=\"" << *stroke_color_ << "\"";
        if (stroke_width_) out << " stroke-width=\"" << *stroke_width_ << "\"";
        if (stroke_line_cap_) out << " stroke-linecap=\"" << *stroke_line_cap_ << "\"";
        if (stroke_line_join_) out << " stroke-linejoin=\"" << *stroke_line_join_ << "\"";
    }

private:
    Owner& AsOwner() { return static_cast<Owner&>(*this); }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<std::string> stroke_line_cap_;
    std::optional<std::string> stroke_line_join_;
};


class Object {
public:
    virtual void Render(std::ostream& out) const = 0;
    virtual ~Object() = default;
};

class Circle : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);
    void Render(std::ostream& out) const override;
private:
    Point center_;
    double radius_ = 1.0;
};


class Polyline : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);
    void Render(std::ostream& out) const override;
private:
    std::vector<Point> points_;
};


class Text : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(const std::string& font_family);
    Text& SetFontWeight(const std::string& font_weight);
    Text& SetData(const std::string& data);
    void Render(std::ostream& out) const override;

private:
    Point pos_;
    Point offset_;
    uint32_t font_size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_;
};


class Document {
public:
    template <typename Obj>
    void Add(Obj obj) {
        objects_.push_back(std::make_unique<Obj>(std::move(obj)));
    }
    void Render(std::ostream& out) const;
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

} // namespace svg
