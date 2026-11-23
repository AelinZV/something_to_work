#pragma once
#include <algorithm>

// Geometry-independent layout policy: easy to test without a desktop session.
namespace border::ui {
struct ResponsivePolicy {
    bool compact;
    int coverHeight;
    int mapMinimumHeight;
};

inline ResponsivePolicy policyFor(int width, int height) {
    const int w = std::max(500, width);
    const int h = std::max(400, height);
    return {w < 1120,
            std::clamp(h / 5, 108, 205),
            std::clamp(h / 4, 150, 270)};
}
} // namespace border::ui
