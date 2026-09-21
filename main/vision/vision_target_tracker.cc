#include "vision_target_tracker.h"

#include <algorithm>
#include <cmath>

namespace {
float ClampSigned(float value) {
    return std::max(-1.0f, std::min(1.0f, value));
}

float ApplyDeadband(float value, float deadband) {
    return std::fabs(value) < deadband ? 0.0f : value;
}
}  // namespace

NaraVisionTargetTracker::NaraVisionTargetTracker(NaraVisionTrackerConfig config)
    : config_(config) {
    config_.frame_width = std::max(1.0f, config_.frame_width);
    config_.frame_height = std::max(1.0f, config_.frame_height);
    config_.smoothing = std::max(0.01f, std::min(1.0f, config_.smoothing));
    config_.deadband = std::max(0.0f, std::min(0.5f, config_.deadband));
}

float NaraVisionTargetTracker::Normalize(float value, float extent) const {
    return ClampSigned((value / (extent * 0.5f)) - 1.0f);
}

std::optional<NaraVisionGaze> NaraVisionTargetTracker::Update(
    const NaraVisionBox& box,
    uint32_t now_ms) {
    if (box.score < config_.min_score) {
        return Tick(now_ms);
    }

    const float target_x = ApplyDeadband(
        Normalize(box.x, config_.frame_width),
        config_.deadband);
    const float target_y = ApplyDeadband(
        Normalize(box.y, config_.frame_height),
        config_.deadband);

    if (!active_) {
        gaze_.x = target_x;
        gaze_.y = target_y;
        active_ = true;
    } else {
        gaze_.x += (target_x - gaze_.x) * config_.smoothing;
        gaze_.y += (target_y - gaze_.y) * config_.smoothing;
    }

    last_seen_ms_ = now_ms;
    return gaze_;
}

std::optional<NaraVisionGaze> NaraVisionTargetTracker::Tick(uint32_t now_ms) {
    if (!active_) {
        return std::nullopt;
    }
    if (static_cast<uint32_t>(now_ms - last_seen_ms_) > config_.hold_ms) {
        Clear();
        return std::nullopt;
    }
    return gaze_;
}

void NaraVisionTargetTracker::Clear() {
    active_ = false;
    last_seen_ms_ = 0;
    gaze_ = {};
}
