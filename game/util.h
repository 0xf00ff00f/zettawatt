#pragma once

#include <array>
#include <glm/glm.hpp>

namespace GX {

template<typename Point>
struct Box {
    Point min = Point(0);
    Point max = Point(0);

    auto width() const
    {
        return max.x - min.x;
    }

    auto height() const
    {
        return max.y - min.y;
    }

    Point size() const
    {
        return Point(width(), height());
    }

    Point center() const { return 0.5f * (min + max); }

    void moveMin(const Point &p)
    {
        const auto size = max - min;
        min = p;
        max = p + size;
    }

    void moveMax(const Point &p)
    {
        const auto size = max - min;
        max = p;
        min = p - size;
    }

    Box &operator+=(const Point &rhs)
    {
        min += rhs;
        max += rhs;
        return *this;
    }

    Box &operator*=(float scale)
    {
        min *= scale;
        max *= scale;
        return *this;
    }

    Box &operator|=(const Box &rhs)
    {
        min = glm::min(min, rhs.min);
        max = glm::max(max, rhs.max);
        return *this;
    }

    bool contains(const Point &p) const
    {
        return p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y;
    }

    bool contains(const Box &other) const
    {
        return !(other.max.x < min.x || other.min.x > max.x || other.max.y < min.y || other.min.y > max.y);
    }

    operator bool() const
    {
        return min != Point(0) || max != Point(0);
    }
};

template<typename Point>
inline Box<Point> operator+(Box<Point> lhs, const Point &rhs)
{
    lhs += rhs;
    return lhs;
}

template<typename Point>
inline Box<Point> operator*(Box<Point> lhs, float scale)
{
    lhs *= scale;
    return lhs;
}

template<typename Point>
inline Box<Point> operator|(Box<Point> lhs, const Box<Point> &rhs)
{
    lhs |= rhs;
    return lhs;
}

using BoxF = Box<glm::vec2>;
using BoxI = Box<glm::ivec2>;

template<typename Point>
using Quad = std::array<Point, 4>;

using QuadF = Quad<glm::vec2>;
using QuadI = Quad<glm::ivec2>;

} // namespace GX
