#include "vision_target_tracker.h"

#include <cassert>
#include <cmath>

static bool Near(float a, float b, float epsilon = 0.03f) {
    return std::fabs(a - b) <= epsilon;
}

int main() {
    {
        NaraVisionTargetTracker tracker({
            .frame_width = 240,
            .frame_height = 240,
            .min_score = 60,
            .smoothing = 1.0f,
            .deadband = 0.0f,
            .hold_ms = 500,
        });

        auto center = tracker.Update({120, 120, 20, 20, 90, 0}, 100);
        assert(center.has_value());
        assert(Near(center->x, 0));
        assert(Near(center->y, 0));

        auto right = tracker.Update({240, 60, 20, 20, 90, 0}, 200);
        assert(right.has_value());
        assert(Near(right->x, 1));
        assert(Near(right->y, -0.5f));

        assert(tracker.Tick(600).has_value());
        assert(!tracker.Tick(701).has_value());
    }

    {
        NaraVisionTargetTracker tracker({
            .frame_width = 200,
            .frame_height = 200,
            .min_score = 70,
            .smoothing = 0.5f,
            .deadband = 0.1f,
            .hold_ms = 500,
        });
        assert(!tracker.Update({100, 100, 10, 10, 50, 0}, 10).has_value());

        auto first = tracker.Update({150, 100, 10, 10, 90, 0}, 20);
        assert(first.has_value());
        assert(Near(first->x, 0.5f));

        auto smooth = tracker.Update({200, 100, 10, 10, 90, 0}, 40);
        assert(smooth.has_value());
        assert(Near(smooth->x, 0.75f));
    }

    return 0;
}
