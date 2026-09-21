#pragma once

#include <cstdint>

enum class NaraTouchGesture {
    None,
    Tap,
    DoubleTap,
    Hold,
    Stroke,
};

struct NaraTouchSample {
    bool pressed = false;
    float x = 0.0f;
    float y = 0.0f;
    uint32_t timestamp_ms = 0;
};

struct NaraTouchClassifierConfig {
    uint32_t tap_max_ms = 320;
    uint32_t double_tap_gap_ms = 360;
    uint32_t hold_min_ms = 650;
    float stationary_radius_px = 22.0f;
    float stroke_distance_px = 58.0f;
    float double_tap_radius_px = 48.0f;
};

class NaraTouchClassifier {
public:
    explicit NaraTouchClassifier(NaraTouchClassifierConfig config = {});

    NaraTouchGesture Update(const NaraTouchSample& sample);
    void Reset();

private:
    NaraTouchClassifierConfig config_;
    bool pressed_ = false;
    bool hold_emitted_ = false;
    bool pending_tap_ = false;
    uint32_t press_started_ms_ = 0;
    uint32_t pending_tap_ms_ = 0;
    float start_x_ = 0.0f;
    float start_y_ = 0.0f;
    float last_x_ = 0.0f;
    float last_y_ = 0.0f;
    float path_distance_px_ = 0.0f;
    float pending_tap_x_ = 0.0f;
    float pending_tap_y_ = 0.0f;

    static float Distance(float ax, float ay, float bx, float by);
};
