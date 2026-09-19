#pragma once

#include <cstdint>

enum class NaraInteractionState : uint8_t {
    Idle = 0,
    Listening,
    Thinking,
    Speaking,
    Sleeping,
    Error,
};

enum class NaraEmotion : uint8_t {
    Neutral = 0,
    Happy,
    Shy,
    Sad,
    Annoyed,
    Surprised,
};

struct NaraFaceState {
    NaraInteractionState interaction = NaraInteractionState::Idle;
    NaraEmotion emotion = NaraEmotion::Neutral;
    float emotion_intensity = 0.6f;

    float gaze_x = 0.0f;
    float gaze_y = 0.0f;

    float left_eye_open = 1.0f;
    float right_eye_open = 1.0f;
    float mouth_open = 0.0f;
    float breath = 0.5f;
};

inline float NaraClamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

inline float NaraClampSigned(float value) {
    if (value < -1.0f) return -1.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}
