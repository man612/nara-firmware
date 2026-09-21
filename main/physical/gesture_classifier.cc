#include "gesture_classifier.h"

#include <algorithm>
#include <cmath>

namespace {
float AccMagnitude(const NaraMotionSample& sample) {
    return std::sqrt(sample.ax_g * sample.ax_g + sample.ay_g * sample.ay_g +
                     sample.az_g * sample.az_g);
}

float GyroMagnitude(const NaraMotionSample& sample) {
    return std::sqrt(sample.gx_dps * sample.gx_dps + sample.gy_dps * sample.gy_dps +
                     sample.gz_dps * sample.gz_dps);
}

bool IsOpposite(int a, int b) {
    return a != 0 && b != 0 && a == -b;
}
}  // namespace

NaraGestureClassifier::NaraGestureClassifier(NaraGestureConfig config) : config_(config) {}

void NaraGestureClassifier::Reset() {
    orientation_ = 0;
    orientation_changed_ms_ = 0;
    last_emit_ms_ = 0;
    shake_window_started_ms_ = 0;
    shake_peaks_ = 0;
    shake_peak_active_ = false;
    spin_started_ms_ = 0;
}

bool NaraGestureClassifier::CanEmit(uint32_t now_ms) const {
    return last_emit_ms_ == 0 ||
           static_cast<uint32_t>(now_ms - last_emit_ms_) >= config_.cooldown_ms;
}

NaraMotionGesture NaraGestureClassifier::Emit(NaraMotionGesture gesture, uint32_t now_ms) {
    last_emit_ms_ = now_ms;
    shake_peaks_ = 0;
    shake_peak_active_ = false;
    spin_started_ms_ = 0;
    return gesture;
}

int NaraGestureClassifier::ClassifyOrientation(const NaraMotionSample& sample) const {
    const float magnitude = AccMagnitude(sample);
    if (magnitude < config_.orientation_min_g || magnitude > config_.orientation_max_g) {
        return 0;
    }

    const float ax = std::fabs(sample.ax_g);
    const float ay = std::fabs(sample.ay_g);
    const float az = std::fabs(sample.az_g);
    if (ax >= ay && ax >= az) return sample.ax_g >= 0.0f ? 1 : -1;
    if (ay >= ax && ay >= az) return sample.ay_g >= 0.0f ? 2 : -2;
    return sample.az_g >= 0.0f ? 3 : -3;
}

NaraMotionGesture NaraGestureClassifier::Update(const NaraMotionSample& sample) {
    const uint32_t now = sample.timestamp_ms;

    const int new_orientation = ClassifyOrientation(sample);
    if (new_orientation != 0 && new_orientation != orientation_) {
        const int previous = orientation_;
        const uint32_t previous_changed = orientation_changed_ms_;
        orientation_ = new_orientation;
        orientation_changed_ms_ = now;

        if (IsOpposite(previous, new_orientation) &&
            previous_changed != 0 &&
            static_cast<uint32_t>(now - previous_changed) <= config_.flip_window_ms &&
            CanEmit(now)) {
            return Emit(NaraMotionGesture::Flip, now);
        }
    }

    const float acceleration = AccMagnitude(sample);
    const bool high_peak = acceleration >= config_.shake_peak_g;
    if (shake_window_started_ms_ == 0 ||
        static_cast<uint32_t>(now - shake_window_started_ms_) > config_.shake_window_ms) {
        shake_window_started_ms_ = now;
        shake_peaks_ = 0;
        shake_peak_active_ = false;
    }
    if (high_peak && !shake_peak_active_) {
        shake_peak_active_ = true;
        ++shake_peaks_;
    } else if (!high_peak && acceleration < config_.shake_peak_g * 0.72f) {
        shake_peak_active_ = false;
    }
    if (shake_peaks_ >= 3 && CanEmit(now)) {
        return Emit(NaraMotionGesture::Shake, now);
    }

    const float gyro = GyroMagnitude(sample);
    if (gyro >= config_.spin_rate_dps) {
        if (spin_started_ms_ == 0) {
            spin_started_ms_ = now;
        } else if (static_cast<uint32_t>(now - spin_started_ms_) >= config_.spin_hold_ms &&
                   CanEmit(now)) {
            return Emit(NaraMotionGesture::Spin, now);
        }
    } else if (gyro < config_.spin_rate_dps * 0.60f) {
        spin_started_ms_ = 0;
    }

    return NaraMotionGesture::None;
}
