#pragma once

#include <cstdint>
#include <optional>

struct NaraVisionBox {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float score = 0.0f;
    int target = -1;
};

struct NaraVisionGaze {
    float x = 0.0f;
    float y = 0.0f;
};

struct NaraVisionTrackerConfig {
    float frame_width = 240.0f;
    float frame_height = 240.0f;
    float min_score = 60.0f;
    float smoothing = 0.30f;
    float deadband = 0.04f;
    uint32_t hold_ms = 900;
};

class NaraVisionTargetTracker {
public:
    explicit NaraVisionTargetTracker(NaraVisionTrackerConfig config = {});

    std::optional<NaraVisionGaze> Update(
        const NaraVisionBox& box,
        uint32_t now_ms);
    std::optional<NaraVisionGaze> Tick(uint32_t now_ms);
    void Clear();

private:
    NaraVisionTrackerConfig config_;
    NaraVisionGaze gaze_{};
    uint32_t last_seen_ms_ = 0;
    bool active_ = false;

    float Normalize(float value, float extent) const;
};
