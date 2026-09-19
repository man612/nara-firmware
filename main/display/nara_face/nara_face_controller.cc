#include "nara_face_controller.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

float Approach(float current, float target, float alpha) {
    return current + (target - current) * NaraClamp01(alpha);
}
}  // namespace

NaraFaceController::NaraFaceController(uint32_t seed)
    : rng_state_(seed == 0 ? 0x4e415241u : seed) {
    Reset(0);
}

void NaraFaceController::Reset(uint32_t now_ms) {
    state_ = {};
    last_tick_ms_ = now_ms;
    blink_active_ = false;
    manual_gaze_ = false;
    gaze_target_x_ = 0.0f;
    gaze_target_y_ = 0.0f;
    speech_level_ = 0.0f;
    ScheduleBlink(now_ms);
    ScheduleIdleGaze(now_ms);
}

void NaraFaceController::SetInteraction(NaraInteractionState interaction) {
    state_.interaction = interaction;
}

void NaraFaceController::SetEmotion(NaraEmotion emotion, float intensity) {
    state_.emotion = emotion;
    state_.emotion_intensity = NaraClamp01(intensity);
}

void NaraFaceController::SetGaze(float x, float y) {
    manual_gaze_ = true;
    gaze_target_x_ = NaraClampSigned(x);
    gaze_target_y_ = NaraClampSigned(y);
}

void NaraFaceController::ClearManualGaze() {
    manual_gaze_ = false;
    gaze_target_x_ = 0.0f;
    gaze_target_y_ = 0.0f;
    ScheduleIdleGaze(last_tick_ms_);
}

void NaraFaceController::SetSpeechLevel(float level) {
    speech_level_ = NaraClamp01(level);
}

uint32_t NaraFaceController::NextRandom() {
    uint32_t x = rng_state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state_ = x;
    return x;
}

float NaraFaceController::Random01() {
    return static_cast<float>(NextRandom() & 0x00ffffffu) /
           static_cast<float>(0x01000000u);
}

float NaraFaceController::RandomRange(float min_value, float max_value) {
    return min_value + (max_value - min_value) * Random01();
}

void NaraFaceController::ScheduleBlink(uint32_t now_ms) {
    const uint32_t interval =
        2800u + static_cast<uint32_t>(RandomRange(0.0f, 2400.0f));
    next_blink_ms_ = now_ms + interval;
}

void NaraFaceController::ScheduleIdleGaze(uint32_t now_ms) {
    const uint32_t interval =
        1400u + static_cast<uint32_t>(RandomRange(0.0f, 3200.0f));
    next_idle_gaze_ms_ = now_ms + interval;
}

void NaraFaceController::UpdateGazeTarget(uint32_t now_ms) {
    if (manual_gaze_) return;

    switch (state_.interaction) {
        case NaraInteractionState::Listening:
        case NaraInteractionState::Speaking:
            gaze_target_x_ = 0.0f;
            gaze_target_y_ = 0.0f;
            break;
        case NaraInteractionState::Thinking:
            gaze_target_x_ = 0.34f;
            gaze_target_y_ = -0.28f;
            break;
        case NaraInteractionState::Sleeping:
            gaze_target_x_ = 0.0f;
            gaze_target_y_ = 0.0f;
            break;
        case NaraInteractionState::Error:
            gaze_target_x_ = 0.0f;
            gaze_target_y_ = 0.08f;
            break;
        case NaraInteractionState::Idle:
            if (now_ms >= next_idle_gaze_ms_) {
                gaze_target_x_ = RandomRange(-0.58f, 0.58f);
                gaze_target_y_ = RandomRange(-0.30f, 0.30f);
                ScheduleIdleGaze(now_ms);
            }
            break;
    }
}

void NaraFaceController::Tick(uint32_t now_ms) {
    const uint32_t raw_delta = now_ms - last_tick_ms_;
    const uint32_t delta_ms = std::min<uint32_t>(raw_delta, 100u);
    last_tick_ms_ = now_ms;

    UpdateGazeTarget(now_ms);

    const float gaze_alpha = std::min(1.0f, static_cast<float>(delta_ms) / 140.0f);
    state_.gaze_x = Approach(state_.gaze_x, gaze_target_x_, gaze_alpha);
    state_.gaze_y = Approach(state_.gaze_y, gaze_target_y_, gaze_alpha);

    const bool can_blink = state_.interaction != NaraInteractionState::Sleeping;
    if (can_blink && !blink_active_ && now_ms >= next_blink_ms_) {
        blink_active_ = true;
        blink_started_ms_ = now_ms;
    }

    float blink_open = 1.0f;
    if (blink_active_) {
        const uint32_t elapsed = now_ms - blink_started_ms_;
        if (elapsed >= kBlinkDurationMs) {
            blink_active_ = false;
            ScheduleBlink(now_ms);
        } else {
            const float t = static_cast<float>(elapsed) /
                            static_cast<float>(kBlinkDurationMs);
            blink_open = 1.0f - std::sin(kPi * t);
        }
    }

    float base_eye_open = 1.0f;
    switch (state_.interaction) {
        case NaraInteractionState::Sleeping:
            base_eye_open = 0.06f;
            break;
        case NaraInteractionState::Thinking:
            base_eye_open = 0.86f;
            break;
        case NaraInteractionState::Error:
            base_eye_open = 0.72f;
            break;
        default:
            break;
    }

    const float eye_open = NaraClamp01(base_eye_open * blink_open);
    state_.left_eye_open = eye_open;
    state_.right_eye_open = eye_open;

    float target_mouth = 0.0f;
    if (state_.interaction == NaraInteractionState::Speaking) {
        target_mouth = NaraClamp01(0.08f + speech_level_ * 0.92f);
    } else if (state_.emotion == NaraEmotion::Surprised) {
        target_mouth = 0.24f + state_.emotion_intensity * 0.18f;
    }

    const float mouth_alpha =
        std::min(1.0f, static_cast<float>(delta_ms) / 70.0f);
    state_.mouth_open = Approach(state_.mouth_open, target_mouth, mouth_alpha);

    const float breath_phase =
        static_cast<float>(now_ms % 5200u) / 5200.0f;
    state_.breath = 0.5f + 0.5f * std::sin(breath_phase * 2.0f * kPi);
}
