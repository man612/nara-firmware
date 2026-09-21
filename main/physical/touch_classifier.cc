#include "touch_classifier.h"

#include <algorithm>
#include <cmath>

NaraTouchClassifier::NaraTouchClassifier(NaraTouchClassifierConfig config)
    : config_(config) {
    config_.tap_max_ms = std::max<uint32_t>(80, config_.tap_max_ms);
    config_.double_tap_gap_ms =
        std::max<uint32_t>(120, config_.double_tap_gap_ms);
    config_.hold_min_ms = std::max<uint32_t>(400, config_.hold_min_ms);
    config_.stationary_radius_px =
        std::max(4.0f, config_.stationary_radius_px);
    config_.stroke_distance_px =
        std::max(config_.stationary_radius_px + 8.0f,
                 config_.stroke_distance_px);
    config_.double_tap_radius_px =
        std::max(config_.stationary_radius_px,
                 config_.double_tap_radius_px);
}

float NaraTouchClassifier::Distance(
    float ax, float ay, float bx, float by) {
    const float dx = ax - bx;
    const float dy = ay - by;
    return std::sqrt((dx * dx) + (dy * dy));
}

NaraTouchGesture NaraTouchClassifier::Update(const NaraTouchSample& sample) {
    if (pending_tap_ && !sample.pressed &&
        static_cast<uint32_t>(sample.timestamp_ms - pending_tap_ms_) >
            config_.double_tap_gap_ms) {
        pending_tap_ = false;
        return NaraTouchGesture::Tap;
    }

    if (sample.pressed) {
        if (!pressed_) {
            pressed_ = true;
            hold_emitted_ = false;
            press_started_ms_ = sample.timestamp_ms;
            start_x_ = sample.x;
            start_y_ = sample.y;
            last_x_ = sample.x;
            last_y_ = sample.y;
            path_distance_px_ = 0.0f;
            return NaraTouchGesture::None;
        }

        path_distance_px_ +=
            Distance(last_x_, last_y_, sample.x, sample.y);
        last_x_ = sample.x;
        last_y_ = sample.y;

        const uint32_t duration =
            static_cast<uint32_t>(sample.timestamp_ms - press_started_ms_);
        const float displacement =
            Distance(start_x_, start_y_, sample.x, sample.y);
        if (!hold_emitted_ &&
            duration >= config_.hold_min_ms &&
            displacement <= config_.stationary_radius_px &&
            path_distance_px_ <= config_.stationary_radius_px * 1.8f) {
            hold_emitted_ = true;
            pending_tap_ = false;
            return NaraTouchGesture::Hold;
        }
        return NaraTouchGesture::None;
    }

    if (!pressed_) {
        return NaraTouchGesture::None;
    }

    pressed_ = false;
    const uint32_t duration =
        static_cast<uint32_t>(sample.timestamp_ms - press_started_ms_);
    const float displacement =
        Distance(start_x_, start_y_, last_x_, last_y_);

    if (hold_emitted_) {
        hold_emitted_ = false;
        pending_tap_ = false;
        return NaraTouchGesture::None;
    }

    if (path_distance_px_ >= config_.stroke_distance_px ||
        displacement >= config_.stroke_distance_px) {
        pending_tap_ = false;
        return NaraTouchGesture::Stroke;
    }

    if (duration <= config_.tap_max_ms &&
        displacement <= config_.stationary_radius_px) {
        if (pending_tap_) {
            const uint32_t gap =
                static_cast<uint32_t>(sample.timestamp_ms - pending_tap_ms_);
            const float tap_distance =
                Distance(start_x_, start_y_, pending_tap_x_, pending_tap_y_);
            if (gap <= config_.double_tap_gap_ms &&
                tap_distance <= config_.double_tap_radius_px) {
                pending_tap_ = false;
                return NaraTouchGesture::DoubleTap;
            }
        }

        pending_tap_ = true;
        pending_tap_ms_ = sample.timestamp_ms;
        pending_tap_x_ = start_x_;
        pending_tap_y_ = start_y_;
    }

    return NaraTouchGesture::None;
}

void NaraTouchClassifier::Reset() {
    pressed_ = false;
    hold_emitted_ = false;
    pending_tap_ = false;
    press_started_ms_ = 0;
    pending_tap_ms_ = 0;
    path_distance_px_ = 0.0f;
}
