#pragma once

#include <cstdint>
#include <string>

enum class NaraGameInput : uint8_t {
    None,
    Tap,
    DoubleTap,
    Hold,
    Stroke,
    Shake,
};

enum class NaraGameEvent : uint8_t {
    None,
    Started,
    Correct,
    Wrong,
    Timeout,
    Won,
    Lost,
    Stopped,
};

struct NaraGameSnapshot {
    bool active = false;
    int round = 0;
    int target_rounds = 5;
    int score = 0;
    int lives = 3;
    NaraGameInput expected = NaraGameInput::None;
    uint32_t deadline_ms = 0;
};

class NaraSaysGame {
public:
    void Start(uint32_t seed, uint32_t now_ms, int target_rounds = 5);
    NaraGameEvent Stop();
    NaraGameEvent Input(NaraGameInput input, uint32_t now_ms);
    NaraGameEvent Tick(uint32_t now_ms);

    const NaraGameSnapshot& snapshot() const { return snapshot_; }
    static const char* Prompt(NaraGameInput input);

private:
    static constexpr uint32_t kRoundTimeoutMs = 5000;

    NaraGameSnapshot snapshot_;
    uint32_t rng_ = 1;

    uint32_t NextRandom();
    void NextRound(uint32_t now_ms);
    NaraGameEvent AdvanceAfterMiss(
        uint32_t now_ms,
        NaraGameEvent miss_event);
};
