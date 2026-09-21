#pragma once

#include <cstdint>

enum class NaraMotionGesture {
    None,
    Flip,
    Shake,
    Spin,
};

struct NaraMotionSample {
    float ax_g = 0.0f;
    float ay_g = 0.0f;
    float az_g = 0.0f;
    float gx_dps = 0.0f;
    float gy_dps = 0.0f;
    float gz_dps = 0.0f;
    uint32_t timestamp_ms = 0;
};

struct NaraGestureConfig {
    float orientation_min_g = 0.72f;
    float orientation_max_g = 1.30f;
    float shake_peak_g = 1.85f;
    float spin_rate_dps = 260.0f;
    uint32_t flip_window_ms = 1600;
    uint32_t shake_window_ms = 650;
    uint32_t spin_hold_ms = 260;
    uint32_t cooldown_ms = 1200;
};

class NaraGestureClassifier {
public:
    explicit NaraGestureClassifier(NaraGestureConfig config = {});

    NaraMotionGesture Update(const NaraMotionSample& sample);
    void Reset();

private:
    NaraGestureConfig config_;
    int orientation_ = 0;
    uint32_t orientation_changed_ms_ = 0;
    uint32_t last_emit_ms_ = 0;

    uint32_t shake_window_started_ms_ = 0;
    int shake_peaks_ = 0;
    bool shake_peak_active_ = false;

    uint32_t spin_started_ms_ = 0;

    int ClassifyOrientation(const NaraMotionSample& sample) const;
    bool CanEmit(uint32_t now_ms) const;
    NaraMotionGesture Emit(NaraMotionGesture gesture, uint32_t now_ms);
};
