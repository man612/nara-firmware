#pragma once

#include "nara_face_state.h"

#include <cstdint>

class NaraFaceController {
public:
    explicit NaraFaceController(uint32_t seed = 0x4e415241u);

    void Reset(uint32_t now_ms = 0);

    void SetInteraction(NaraInteractionState interaction);
    void SetEmotion(NaraEmotion emotion, float intensity = 0.6f);
    void SetGaze(float x, float y);
    void ClearManualGaze();
    void SetSpeechLevel(float level);

    void Tick(uint32_t now_ms);

    const NaraFaceState& state() const { return state_; }

private:
    static constexpr uint32_t kBlinkDurationMs = 150;

    NaraFaceState state_{};
    uint32_t rng_state_;
    uint32_t last_tick_ms_ = 0;
    uint32_t next_blink_ms_ = 0;
    uint32_t blink_started_ms_ = 0;
    uint32_t next_idle_gaze_ms_ = 0;

    bool blink_active_ = false;
    bool manual_gaze_ = false;

    float gaze_target_x_ = 0.0f;
    float gaze_target_y_ = 0.0f;
    float speech_level_ = 0.0f;

    uint32_t NextRandom();
    float Random01();
    float RandomRange(float min_value, float max_value);

    void ScheduleBlink(uint32_t now_ms);
    void ScheduleIdleGaze(uint32_t now_ms);
    void UpdateGazeTarget(uint32_t now_ms);
};
