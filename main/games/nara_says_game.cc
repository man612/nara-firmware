#include "nara_says_game.h"

#include <algorithm>

namespace {
constexpr NaraGameInput kInputs[] = {
    NaraGameInput::Tap,
    NaraGameInput::DoubleTap,
    NaraGameInput::Hold,
    NaraGameInput::Stroke,
    NaraGameInput::Shake,
};
}

void NaraSaysGame::Start(
    uint32_t seed,
    uint32_t now_ms,
    int target_rounds) {
    rng_ = seed == 0 ? 0x4e415241u : seed;
    snapshot_ = {};
    snapshot_.active = true;
    snapshot_.target_rounds =
        std::clamp(target_rounds, 1, 12);
    snapshot_.lives = 3;
    NextRound(now_ms);
}

NaraGameEvent NaraSaysGame::Stop() {
    if (!snapshot_.active) {
        return NaraGameEvent::None;
    }
    snapshot_.active = false;
    snapshot_.expected = NaraGameInput::None;
    snapshot_.deadline_ms = 0;
    return NaraGameEvent::Stopped;
}

uint32_t NaraSaysGame::NextRandom() {
    // Small deterministic xorshift32: no heap, no network, reproducible tests.
    uint32_t x = rng_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_ = x == 0 ? 0x4e415241u : x;
    return rng_;
}

void NaraSaysGame::NextRound(uint32_t now_ms) {
    ++snapshot_.round;
    snapshot_.expected =
        kInputs[NextRandom() % (sizeof(kInputs) / sizeof(kInputs[0]))];
    snapshot_.deadline_ms = now_ms + kRoundTimeoutMs;
}

NaraGameEvent NaraSaysGame::AdvanceAfterMiss(
    uint32_t now_ms,
    NaraGameEvent miss_event) {
    --snapshot_.lives;
    if (
        snapshot_.lives <= 0 ||
        snapshot_.round >= snapshot_.target_rounds) {
        snapshot_.active = false;
        snapshot_.expected = NaraGameInput::None;
        snapshot_.deadline_ms = 0;
        return NaraGameEvent::Lost;
    }
    NextRound(now_ms);
    return miss_event;
}

NaraGameEvent NaraSaysGame::Input(
    NaraGameInput input,
    uint32_t now_ms) {
    if (!snapshot_.active || input == NaraGameInput::None) {
        return NaraGameEvent::None;
    }
    if (now_ms >= snapshot_.deadline_ms) {
        return AdvanceAfterMiss(now_ms, NaraGameEvent::Timeout);
    }

    if (input != snapshot_.expected) {
        return AdvanceAfterMiss(now_ms, NaraGameEvent::Wrong);
    }

    ++snapshot_.score;
    if (snapshot_.round >= snapshot_.target_rounds) {
        snapshot_.active = false;
        snapshot_.expected = NaraGameInput::None;
        snapshot_.deadline_ms = 0;
        return NaraGameEvent::Won;
    }
    NextRound(now_ms);
    return NaraGameEvent::Correct;
}

NaraGameEvent NaraSaysGame::Tick(uint32_t now_ms) {
    if (!snapshot_.active || now_ms < snapshot_.deadline_ms) {
        return NaraGameEvent::None;
    }
    return AdvanceAfterMiss(now_ms, NaraGameEvent::Timeout);
}

const char* NaraSaysGame::Prompt(NaraGameInput input) {
    switch (input) {
        case NaraGameInput::Tap:
            return "Tap me once";
        case NaraGameInput::DoubleTap:
            return "Double tap me";
        case NaraGameInput::Hold:
            return "Press and hold";
        case NaraGameInput::Stroke:
            return "Pet / stroke me";
        case NaraGameInput::Shake:
            return "Give me a gentle shake";
        case NaraGameInput::None:
        default:
            return "";
    }
}
